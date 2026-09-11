#include "Testing/CalmyInteractionProbe.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
bool UCalmyInteractionProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString S;const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyInteractionProbe="),S);
#endif
}
void UCalmyInteractionProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_INTERACTION %s %s %s gap_cm=%.3f walked_cm=%.1f soak_seconds=%.1f"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail,MaxGap,Walked,SoakAge);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyInteractionProbe::Tick(float DT)
{
    if(Role.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("CalmyInteractionProbe="),Role);
    if(Stage==999 || !GetWorld()->HasBegunPlay()) return;
    const bool Host=Role==TEXT("Host"),Late=Role==TEXT("LateJoin");
    if((Host && GetWorld()->GetNetMode()!=NM_ListenServer) || (!Host && GetWorld()->GetNetMode()!=NM_Client)) return;
    int32 Duration=0;FParse::Value(FCommandLine::Get(),TEXT("CalmyG1Soak="),Duration);Duration=FMath::Clamp(Duration,0,1200);
    Age+=DT;if(!WallStart) WallStart=FPlatformTime::Seconds();
    if(FPlatformTime::Seconds()-WallStart>Duration+170) {Finish(false,FString::Printf(TEXT("timeout stage=%d"),Stage));return;}
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Explicit isolated fixture above WaterLab, never saved to the map. Only initial placement teleports.
    if(!FloorCreated)
    {
        auto* Floor=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0,0,990),FRotator::ZeroRotator,S);
        Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        // Identical local static fixture on each process; no editor asset or runtime mesh replication assumption.
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorScale3D(FVector(32,20,.2));Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));FloorCreated=true;
    }
    ACalmyCharacter* P=nullptr;bool RemotePresent=false;
    for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)
    {
        auto* PC=Cast<APlayerController>(It->GetController());
        if(Cast<ACalmyCrewHelper>(*It)) continue;
        if(Host && PC && !PC->IsLocalController()) RemotePresent=true;
        if(PC && PC->IsLocalController()) P=*It;
        if(Host && PC && It->GetPlayerState() && !It->ActorHasTag(TEXT("G1Fixture")))
        {
            It->Tags.Add(TEXT("G1Fixture"));
            const int32 ID=It->GetPlayerState()->GetPlayerId();
            int32 AlreadyPlaced=0;for(TActorIterator<ACalmyCharacter> Other(GetWorld());Other;++Other)
                if(*Other!=*It && Other->ActorHasTag(TEXT("G1Fixture")) && !Other->IsLocallyControlled()) ++AlreadyPlaced;
            const float Lane=PC->IsLocalController()?-500.f:AlreadyPlaced*500.f;
            It->SetActorLocation(FVector(-1000,Lane,1090),false,nullptr,ETeleportType::TeleportPhysics);
            It->GetCharacterMovement()->StopMovementImmediately();It->ForceNetUpdate();
            auto* C=GetWorld()->SpawnActor<ACalmyCargo>(FVector(-820,Lane,1035),FRotator::ZeroRotator,S);C->CargoId=100+ID;C->ForceNetUpdate();
            auto* Far=GetWorld()->SpawnActor<ACalmyCargo>(FVector(-400,Lane+160,1035),FRotator::ZeroRotator,S);Far->CargoId=10000+ID;Far->ForceNetUpdate();
        }
    }
    if(!P || !P->GetPlayerState() || Age<2) return;
    if(!Started) {Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_INTERACTION %s START"),*Role);GLog->Flush();}
    if(Late)
    {
        for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)
            if(It->Carry.Holder && It->Carry.Holder!=P && It->Carry.Holder->HeldCargo==*It && It->CargoId>=100)
            {Finish(true,TEXT("late_current_holder=1 short_range_config=1"));return;}
        return;
    }
    ACalmyCargo* Cargo=nullptr;ACalmyCargo* Far=nullptr;
    for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)
    {if(It->CargoId==100+P->GetPlayerState()->GetPlayerId()) Cargo=*It;if(It->CargoId==10000+P->GetPlayerState()->GetPlayerId()) Far=*It;}
    if(!Cargo || !Far || P->GetActorLocation().Z<1050) return;
    auto* PC=Cast<APlayerController>(P->GetController());
    auto Advance=[&](int32 Next){Stage=Next;StageAge=Age;UE_LOG(LogTemp,Display,TEXT("CALMY_INTERACTION %s STAGE %d"),*Role,Stage);GLog->Flush();};
    auto Aim=[&](ACalmyCargo* C){PC->SetControlRotation((C->GetActorLocation()-P->GetInteractionOrigin()).Rotation());};
    auto* Rig=P->GetCrewPresentation();
    if(!Rig || !Rig->HasFiniteBodies() || Rig->SimulatedPartCount()!=9) {Finish(false,TEXT("invalid_rig"));return;}
    MaxGap=FMath::Max(MaxGap,Rig->MaximumJointSeparation());
    if(MaxGap>8 || P->GetActorLocation().ContainsNaN()) {Finish(false,TEXT("joint_or_transform_invalid"));return;}
    if(Stage==0)
    { PC->SetControlRotation(FRotator(0,180,0));RejectedBefore=P->RejectedCommands;Advance(1); }
    else if(Stage==1 && Age-StageAge>.7)
    { P->ServerGrab(Cargo,++P->LastIssuedSequence);Advance(2); }
    else if(Stage==2 && Age-StageAge>.7)
    { if(P->HeldCargo || P->RejectedCommands<=RejectedBefore) {Finish(false,TEXT("off_aim_grab_accepted"));return;}Aim(Far);RejectedBefore=P->RejectedCommands;Advance(3); }
    else if(Stage==3)
    { Aim(Far);if(Age-StageAge>.7) {P->ServerGrab(Far,++P->LastIssuedSequence);Advance(4);} }
    else if(Stage==4 && Age-StageAge>.7)
    { if(P->HeldCargo || P->RejectedCommands<=RejectedBefore) {Finish(false,TEXT("far_grab_accepted"));return;}Aim(Cargo);Advance(5); }
    else if(Stage==5)
    { Aim(Cargo);if(Age-StageAge>.7) {const uint32 Seq=++P->LastIssuedSequence;P->ServerGrab(Cargo,Seq);P->ServerGrab(Cargo,Seq);Advance(6);} }
    else if(Stage==6 && Age-StageAge>1)
    {
        if(P->HeldCargo!=Cargo || Cargo->Carry.Holder!=P || P->RejectedCommands<3) {Finish(false,TEXT("aimed_pickup_or_replay_failed"));return;}
        SawHeld=true;UE_LOG(LogTemp,Display,TEXT("CALMY_INTERACTION %s READY"),*Role);GLog->Flush();Advance(7);
    }
    else if(Stage==7 && Age-StageAge>25)
    {P->RequestDrop();Advance(8);Previous=P->GetActorLocation();}
    else if(Stage==8)
    {
        SoakAge+=DT;
        if(Duration>0)
        {
            // Walk a clear rectangle through CharacterMovement, without position correction/rescue.
            const float Lane=Host?-500.f:0.f;
            const FVector2D Goals[]={FVector2D(-1200,Lane-120),FVector2D(-600,Lane-120),FVector2D(-600,Lane+120),FVector2D(-1200,Lane+120)};
            const FVector2D Delta=Goals[Waypoint]-FVector2D(P->GetActorLocation());
            if(Delta.Size()<24) Waypoint=(Waypoint+1)%4;
            else P->MoveIntent(Delta.GetSafeNormal()*FMath::Min(1.,Delta.Size()/100.));
            const int32 Cycle=FMath::FloorToInt(SoakAge/10);
            if(SoakAge<Duration && Cycle!=LastCycle && P->IsControlled() && P->GetCharacterMovement()->IsMovingOnGround())
            {LastCycle=Cycle;P->RequestStumble();}
            Walked+=FVector::Dist2D(Previous,P->GetActorLocation());Previous=P->GetActorLocation();
            if(P->ResetCount>0) {Finish(false,TEXT("unexpected_rescue"));return;}
            if(FMath::FloorToInt(SoakAge/60)!=FMath::FloorToInt((SoakAge-DT)/60))
            {UE_LOG(LogTemp,Display,TEXT("G1_SOAK %s seconds=%.1f knock=%d recovery=%d gap=%.3f walked=%.1f"),*Role,SoakAge,P->KnockdownCount,P->RecoveryCount,MaxGap,Walked);GLog->Flush();}
        }
        // Client starts later. Host must retain the session until the owning client finishes/exits.
        if(SoakAge>=Duration && P->IsControlled() && Age>(Host?85.f:45.f) && (!Host || !RemotePresent))
            Finish(SawHeld && !P->HeldCargo && Cargo->Carry.State==ECalmyCarryState::Free && (Duration==0 || (Walked>Duration*80 && P->KnockdownCount>=Duration/12 && P->RecoveryCount==P->KnockdownCount)),
                TEXT("aimed_pickup=1 off_aim_denied=1 range_denied=1 replay_denied=1 drop=1 controlled_at_end=1"));
    }
}
