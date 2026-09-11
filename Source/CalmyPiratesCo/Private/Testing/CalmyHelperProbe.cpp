#include "Testing/CalmyHelperProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyCargo.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
bool UCalmyHelperProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString S;const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyHelperProbe="),S);
#endif
}
void UCalmyHelperProbe::Finish(bool OK,const FString& Why)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_HELPER %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Why);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyHelperProbe::Tick(float DT)
{
    if(Role.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("CalmyHelperProbe="),Role);
    if(Stage==999 || !GetWorld()->HasBegunPlay()) return;
    if(Role==TEXT("Host") && GetWorld()->GetNetMode()!=NM_ListenServer) return;
    if((Role==TEXT("Client")||Role==TEXT("LateJoin")) && GetWorld()->GetNetMode()!=NM_Client) return;
    Age+=DT;ACalmyCharacter* P=nullptr;ACalmyCargo* Heavy=nullptr;int32 Helpers=0;bool Remote=false;
    for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)
    {
        if(Cast<ACalmyCrewHelper>(*It)) {++Helpers;continue;}
        if(It->IsLocallyControlled() && Cast<APlayerController>(It->GetController())) P=*It;
        else Remote=true;
    }
    for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It) if(It->Kind==ECalmyCargoKind::Bulky) Heavy=*It;
    if(!P || !Heavy) return;
    if(Heavy->Carry.State==ECalmyCarryState::Held)
    {
        LowestTeamPace=FMath::Min(LowestTeamPace,Heavy->Condition.TeamPace);
        if(Heavy->Condition.TeamPace<55 || Heavy->Condition.TeamPace>100 || Heavy->GetCarrySpeed()>260)
        {Finish(false,TEXT("invalid_team_pace"));return;}
    }
    if(!Started) {Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_HELPER %s START"),*Role);GLog->Flush();}
    if(Age>150) {Finish(false,FString::Printf(TEXT("timeout stage=%d helper=%d pos=%s"),Stage,Helpers,*P->GetActorLocation().ToString()));return;}
    if(Helpers>1) {Finish(false,TEXT("duplicate_helper_spawn"));return;}
    auto MoveTo=[P](FVector2D Goal) {const FVector2D D=Goal-FVector2D(P->GetActorLocation());if(D.Size()<22)return true;P->MoveIntent(D.GetSafeNormal()*FMath::Min(1.,D.Size()/100.));return false;};
    if(Role==TEXT("LateJoin"))
    {
        for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It)
            if(IsValid(It->Leader) && It->Leader->CrewHelper==*It && It->Leader!=P && Heavy->Carry.Holder==It->Leader)
                {Finish(true,TEXT("helper_leader_snapshot=1 held_cargo_snapshot=1 physical_npc=1"));return;}
        return;
    }
    if(Role==TEXT("Host"))
    {
        MoveTo(FVector2D(-1300,-750)); // Stay out of helper range: AI must be sufficient.
        SawHelper|=Helpers==1;SawHeld|=Heavy->Carry.State==ECalmyCarryState::Held;
        bool FreeCrew=false;for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It) FreeCrew|=!IsValid(It->Leader);
        if(SawHelper && SawHeld && Heavy->Carry.bDelivered && !Remote && Helpers==1 && FreeCrew)
            Finish(true,TEXT("npc_only_assistance=1 delivery=1 disconnect_releases_persistent_crew=1 no_duplicate=1"));
        return;
    }
    auto Advance=[this](int32 Next){Stage=Next;StageTime=Age;UE_LOG(LogTemp,Display,TEXT("CALMY_HELPER %s STAGE %d"),*Role,Stage);GLog->Flush();};
    if(Stage==1) FCalmyPickupFixture::Aim(P,Heavy);
    else if(Stage>=3) FCalmyPickupFixture::Forward(P);
    if(Stage==0 && MoveTo(FVector2D(850,-250)))
    {
        const uint32 Seq=++P->LastIssuedSequence;P->ServerHelper(Seq);P->ServerHelper(Seq);Advance(1);
    }
    else if(Stage==1 && P->CrewHelper && Heavy->HasNearbyHelper(P) && Age-StageTime>1 && FCalmyPickupFixture::Approach(P,Heavy))
    {P->ServerGrab(Heavy,++P->LastIssuedSequence);Advance(2);}
    else if(Stage==2 && Heavy->Carry.Holder==P)
    {UE_LOG(LogTemp,Display,TEXT("CALMY_HELPER %s READY"),*Role);GLog->Flush();Advance(3);}
    else if(Stage==3 && Age-StageTime>10 && MoveTo(FVector2D(850,0))) Advance(4);
    else if(Stage==4 && MoveTo(FVector2D(-900,0))) Advance(5);
    else if(Stage==5 && MoveTo(FVector2D(-900,450))) {P->RequestDrop();Advance(6);}
    else if(Stage==6 && Heavy->Carry.bDelivered)
    {P->RequestHelper();Advance(7);}
    else if(Stage==7 && !P->CrewHelper && Age-StageTime>2.5)
    {
        // Opposite-face carry may leave Misha beyond the 2.2m recruitment range.
        // Exercise the real approach interaction rather than widening that rule.
        for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It)
            if(FVector::Distance(P->GetActorLocation(),It->GetActorLocation())>190)
            {MoveTo(FVector2D(It->GetActorLocation()));return;}
        P->RequestHelper();Advance(8);
    }
    else if(Stage==8 && P->CrewHelper && Age-StageTime>3)
    {Finish(P->RejectedCommands>=1 && Heavy->Condition.ForcedDrops==0 && LowestTeamPace<100,FString::Printf(TEXT("request_replay_rejected=1 carry_route=1 heavy_delivery=1 dismiss_rehire=1 lowest_team_pace=%d"),LowestTeamPace));}
    if(Stage>=3 && Stage<=5 && Heavy->Carry.Holder!=P) Finish(false,TEXT("npc_lost_during_carry"));
}
