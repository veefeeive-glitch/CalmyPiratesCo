#include "Testing/CalmyTutorialProbe.h"
#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyMissionBoard.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"

// Process-local verification memory only: non-seamless travel destroys world subsystems.
static bool SawTraining=false,SawDeparture=false;
bool UCalmyTutorialProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value;const auto* World=Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyTutorialProbe="),Value);
#endif
}
void UCalmyTutorialProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);
    GLog->Flush();Age=-10000;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyTutorialProbe::Capture(const FString& Name)
{
    if(Role!=TEXT("Visual")) return;
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Dir);
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/(Name+TEXT(".png")),true,false);
}
void UCalmyTutorialProbe::Tick(float DT)
{
    if(Role.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("CalmyTutorialProbe="),Role);
    if(Age<0 || !GetWorld()->HasBegunPlay()) return;
    const bool Solo=Role==TEXT("Solo") || Role==TEXT("Visual"),Host=Role==TEXT("Host"),Late=Role==TEXT("LateJoin");
    if(!Solo && GetWorld()->GetNetMode()!=(Host?NM_ListenServer:NM_Client)) return;
    Age+=DT; Retry+=DT;
    if(Age>115) {Finish(false,FString::Printf(TEXT("timeout map=%s step=%d"),*GetWorld()->GetMapName(),LastStep));return;}
    ACalmyPlayerController* PC=nullptr;
    for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(It->Get()->IsLocalController()) PC=Cast<ACalmyPlayerController>(It->Get());
    auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    if(!P || Age<2) return;
    // Voyage G4: completed training departs to the port (mission board), not straight onto the ship.
    if(GetWorld()->GetMapName().Contains(TEXT("PortHub")))
    {
        if(!SawTraining || !SawDeparture || ACalmyTutorial::Find(GetWorld())) {Finish(false,TEXT("port_without_completed_tutorial"));return;}
        if(!PortArrived){PortArrived=true;PortAge=Age;Previous=P->GetActorLocation();if(PC->IsPauseMenuOpen())PC->ResumeFromMenu();
            UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s ARRIVED_PORT pawn=%s"),*Role,*Previous.ToCompactString());GLog->Flush();Capture(TEXT("04-port"));}
        if(Age-PortAge>3 && Age-PortAge<4) P->MoveIntent(FVector2D(.3,0));
        if(Age-PortAge>(Host?20:8))
        {
            int32 Boards=0;for(TActorIterator<ACalmyMissionBoard> It(GetWorld());It;++It) ++Boards;
            const float Walk=FVector::Dist2D(Previous,P->GetActorLocation());
            Finish(!PC->IsPauseMenuOpen() && P->IsControlled() && !P->HeldCargo && P->ResetCount==0 && Boards==1 && Walk>15,
                FString::Printf(TEXT("tutorial_to_port=1 mission_board=%d walked_cm=%.1f resets=%d"),Boards,Walk,P->ResetCount));
        }
        return;
    }
    if(GetWorld()->GetMapName().Contains(TEXT("OceanFreighterR7")))
    {
        if(!SawTraining || !SawDeparture || ACalmyTutorial::Find(GetWorld())) {Finish(false,TEXT("ship_without_completed_tutorial"));return;}
        if(!MenuSeen)
        {
            if(!PC->IsPauseMenuOpen() || !PC->GetBridgeTerminal()) return;
            MenuSeen=true;Previous=P->GetActorLocation();
            UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s ARRIVED pawn=%s"),*Role,*Previous.ToCompactString());
            Capture(TEXT("04-ship-menu"));
        }
        if(Age>5 && PC->IsPauseMenuOpen()) PC->ResumeFromMenu();
        if(Age>6 && Age<7) P->MoveIntent(FVector2D(.3,0));
        if(Age>7 && Shot!=5) {Shot=5;Capture(TEXT("05-ship-play"));ShipWalk=FVector::Dist2D(Previous,P->GetActorLocation());}
        if(Age>(Host?28:12))
        {
            int32 CargoCount=0,HelperCount=0;for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It) ++CargoCount;
            for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It) ++HelperCount;
            Finish(!PC->IsPauseMenuOpen() && ShipWalk>15 && P->IsControlled() && !P->HeldCargo && P->ResetCount==0 && CargoCount==4 && HelperCount==1,
                FString::Printf(TEXT("tutorial_to_ship=1 menu_resume=1 walked_cm=%.1f fresh_cargo=%d helper=%d resets=%d"),ShipWalk,CargoCount,HelperCount,P->ResetCount));
        }
        return;
    }
    auto* Tutorial=ACalmyTutorial::Find(GetWorld());
    if(!Tutorial) return;
    SawTraining=true;
    if(!Started) {Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s START"),*Role);GLog->Flush();}
    if(LastStep!=int32(Tutorial->Step))
    {
        LastStep=int32(Tutorial->Step);
        UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s SNAPSHOT step=%d remaining=%d"),*Role,LastStep,Tutorial->DepartureSeconds);GLog->Flush();
        Capture(FString::Printf(TEXT("0%d-training"),FMath::Min(3,LastStep)));
    }
    if(Tutorial->Step==ECalmyTutorialStep::Depart) {SawDeparture=true;return;}
    if(Tutorial->Step==ECalmyTutorialStep::TravelFailed) {Finish(false,TEXT("travel_failed"));return;}
    if(P->ResetCount!=0) {Finish(false,TEXT("unexpected_rescue"));return;}
    if(Host)
    {
        HostDiagAge+=DT;
        if(HostDiagAge>3){HostDiagAge=0;if(auto* HC=Tutorial->TrainingCargo.Get()){auto* HS=ACalmyShift::Find(GetWorld());
            UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST Host CARGO state=%d holder=%s delivered=%d pos=%s shift_delivered=%d required=%d phase=%d"),int32(HC->Carry.State),*GetNameSafe(HC->Carry.Holder),int32(HC->Carry.bDelivered),
                *HC->GetActorLocation().ToCompactString(),HS?HS->State.Delivered:-1,HS?HS->State.Required:-1,HS?int32(HS->State.Phase):-1);GLog->Flush();}}
    }
    // The owning client completes the team lesson; host and late join only observe.
    if(Host || Late) return;
    auto* C=Tutorial->TrainingCargo.Get();auto* Shift=ACalmyShift::Find(GetWorld());
    if(!C || !Shift) return;
    if(Shift->State.SecondsRemaining<300) {Finish(false,TEXT("tutorial_timer_running"));return;}
    if(LastHeld.IsValid() && !P->HeldCargo)
    {
        const ACalmyCargo* Lost=LastHeld.Get();
        UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s DROP_EVENT held_age=%.1f pawn=%s cargo=%s state=%d holder=%s in_ocean=%d lifecycle=%d resets=%d health=%.0f retry=%.1f"),*Role,HeldAge,*P->GetActorLocation().ToCompactString(),
            *Lost->GetActorLocation().ToCompactString(),int32(Lost->Carry.State),*GetNameSafe(Lost->Carry.Holder),int32(P->IsInOcean()),int32(P->Lifecycle),P->ResetCount,P->Health,Retry);GLog->Flush();
    }
    LastHeld=P->HeldCargo.Get();
    auto WalkTo=[&](FVector Goal,float Stop)
    {
        const FVector2D Delta=FVector2D(Goal-P->GetActorLocation());
        if(Delta.Size()<Stop) return true;
        P->MoveIntent(Delta.GetSafeNormal()*FMath::Min(1.,Delta.Size()/100.));return false;
    };
    if(!P->HeldCargo)
    {
        PC->SetControlRotation((C->GetActorLocation()-P->GetInteractionOrigin()).Rotation());
        if(WalkTo(C->GetActorLocation()-FVector(140,0,0),25) && Retry>1)
        {P->RequestGrab();Retry=0;}
    }
    else
    {
        if(!Ready) {Ready=true;UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s READY"),*Role);GLog->Flush();}
        HeldAge+=DT;
        DiagAge+=DT;
        if(DiagAge>2){DiagAge=0;const FVector G=ACalmyShift::Find(GetWorld())?ACalmyShift::Find(GetWorld())->DeliveryZone->GetComponentLocation():FVector::ZeroVector;
            UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL_TEST %s DIAG held_age=%.1f pawn=%s goal=%s dist2d=%.0f held=%s cargo=%s retry=%.1f vel=%.0f"),*Role,HeldAge,*P->GetActorLocation().ToCompactString(),*(G+FVector(78,0,0)).ToCompactString(),
                FVector::Dist2D(P->GetActorLocation(),G+FVector(78,0,0)),*GetNameSafe(P->HeldCargo),*GetNameSafe(Tutorial->TrainingCargo.Get()),Retry,P->GetVelocity().Size());GLog->Flush();}
        if(!Solo && HeldAge<18) return; // Let the runner join a third player mid-lesson.
        const FVector Goal=Shift->DeliveryZone->GetComponentLocation();
        PC->SetControlRotation(FRotator(0,180,0));
        if(WalkTo(Goal+FVector(78,0,0),20) && Retry>1) {P->RequestDrop();Retry=0;}
    }
}
