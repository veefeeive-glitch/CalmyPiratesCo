#include "Testing/CalmyCharacterProbe.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyPreferences.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/OutputDeviceRedirector.h"
#include "UnrealClient.h"
bool UCalmyCharacterProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V;const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyCharacterProbe="),V);
#endif
}
void UCalmyCharacterProbe::Finish(bool OK,const FString& Why)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_CHARACTER %s %s %s max_joint_gap_cm=%.2f"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Why,MaxJointGap);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyCharacterProbe::Tick(float DT)
{
    if(Role.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("CalmyCharacterProbe="),Role);
    if(Stage==999 || !GetWorld()->HasBegunPlay()) return;
    if(Role!=TEXT("Visual") && ((Role==TEXT("Host") && GetWorld()->GetNetMode()!=NM_ListenServer) || (Role!=TEXT("Host") && GetWorld()->GetNetMode()!=NM_Client))) return;
    Age+=DT;
    ACalmyCharacter* P=nullptr;for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It) if(It->IsLocallyControlled() && Cast<APlayerController>(It->GetController())) P=*It;
    if(Age>110) { Finish(false,FString::Printf(TEXT("timeout stage=%d down=%d recovering=%d pos=%s knock=%d recovery=%d rejected=%d"),Stage,SawDown,SawRecovery,P?*P->GetActorLocation().ToString():TEXT("none"),P?P->KnockdownCount:-1,P?P->RecoveryCount:-1,P?P->RejectedCommands:-1));return; }
    if(!P || Age<1) return;
    if(Role!=TEXT("Visual") && FMath::FloorToInt(Age/5)!=FMath::FloorToInt((Age-DT)/5))
        UE_LOG(LogTemp,Display,TEXT("CREW_PROBE_STATE %s stage=%d pos=%s life=%d knock=%d recovered=%d rejected=%d ground=%d"),*Role,Stage,*P->GetActorLocation().ToString(),static_cast<int32>(P->Lifecycle),P->KnockdownCount,P->RecoveryCount,P->RejectedCommands,P->GetCharacterMovement()->IsMovingOnGround());
    auto* Rig=P->GetCrewPresentation();
    if(!Started)
    {
        Started=true;Initial=P->GetActorLocation();
        UE_LOG(LogTemp,Display,TEXT("CALMY_CHARACTER %s START"),*Role);GLog->Flush();
    }
    const int32 ExpectedBodies=Rig && Rig->IsPhysicalFall() && P->HasAuthority()?10:9;
    if(!Rig || Rig->SimulatedPartCount()!=ExpectedBodies || !Rig->HasFiniteBodies()) { Finish(false,TEXT("missing_or_invalid_physics_rig"));return; }
    MaxJointGap=FMath::Max(MaxJointGap,Rig->MaximumJointSeparation());
    if(P->Lifecycle==ECalmyPlayerLifecycleState::KnockedDown && Rig->IsPhysicalFall())
    {
        LowestRoot=FMath::Min(LowestRoot,static_cast<float>(Rig->GetChestTransform().GetLocation().Z));
        SawFreeRoot|=P->GetCapsuleComponent()->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
        FMinimalViewInfo DownView;P->CalcCamera(DT,DownView);
        if(DownView.Location.ContainsNaN() || DownView.Rotation.ContainsNaN()) {Finish(false,TEXT("invalid_falling_bodycam"));return;}
    }
    if(MaxJointGap>8) { Finish(false,TEXT("articulated_body_separated_over_8cm"));return; }
    if(Role==TEXT("Visual"))
    {
        auto* PC=Cast<APlayerController>(P->GetController());
        if(!ReviewCamera)
        {
            ReviewCamera=GetWorld()->SpawnActor<ACameraActor>();ReviewCamera->GetCameraComponent()->SetFieldOfView(55);
            PC->SetViewTarget(ReviewCamera);StageTime=Age;
            if(PC->GetHUD()) PC->GetHUD()->bShowHUD=false;
        }
        ReviewCamera->SetActorLocation(P->GetActorLocation()+FVector(300,-230,70));
        ReviewCamera->SetActorRotation((P->GetActorLocation()-ReviewCamera->GetActorLocation()).Rotation());
        auto Shot=[&](const TCHAR* Name)
        {
            FString Label=TEXT("Deckhand-Visual");FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Label);
            const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));IFileManager::Get().MakeDirectory(*Dir,true);
            FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),true,false);
            UE_LOG(LogTemp,Display,TEXT("CREW_PHOTO %s gap=%.2f velocity=%s"),Name,Rig->MaximumJointSeparation(),*P->GetVelocity().ToString());
            TArray<UStaticMeshComponent*> Pieces;P->GetComponents(Pieces);
            for(auto* Piece:Pieces) if(Piece->GetName().StartsWith(TEXT("Deckhand_")))
                UE_LOG(LogTemp,Display,TEXT("CREW_POSE %s %s"),*Piece->GetName(),*Piece->GetComponentRotation().ToString());
        };
        if(Stage==0 && Age-StageTime>4) { Shot(TEXT("01-standing"));Stage=1;StageTime=Age; }
        else if(Stage==1)
        {
            P->MoveIntent(FVector2D(1,0));
            if(Age-StageTime>1.3) { Shot(TEXT("02-walk"));Stage=2;StageTime=Age; }
        }
        else if(Stage==2 && Age-StageTime>1) { P->RequestStumble();Stage=3;StageTime=Age; }
        else if(Stage==3 && Age-StageTime>.7) { Shot(TEXT("03-physical-fall"));Stage=4;StageTime=Age; }
        else if(Stage==4 && Age-StageTime>2.5) { Shot(TEXT("04-recovered"));Stage=5;StageTime=Age; }
        else if(Stage==5 && Age-StageTime>1)
        { PC->SetViewTarget(P);PC->SetControlRotation(FRotator(-82,0,0));Stage=6;StageTime=Age; }
        else if(Stage==6 && Age-StageTime>2) { Shot(TEXT("05-bodycam-lookdown"));Stage=7;StageTime=Age; }
        else if(Stage==7 && Age-StageTime>1)
        {
            if(!FParse::Param(FCommandLine::Get(),TEXT("CalmyFallWaterReview"))) {Finish(true,TEXT("five_rendered_views"));return;}
            const auto* Ocean=ACalmyOcean::Find(GetWorld());
            if(!Ocean){Finish(false,TEXT("missing_ocean_fixture"));return;}
            // Explicit review fixture setup, not evidence of walking over a rail.
            P->SetActorLocation(FVector(3100,0,Ocean->HeightAt(FVector(3100,0,0))+350));
            Rig->ResetBodies(); // Move all detached bodies with this explicit fixture placement.
            P->GetCharacterMovement()->DisableMovement();PC->SetViewTarget(ReviewCamera);
            Stage=71;StageTime=Age;
        }
        // Let the physics scene consume the setup teleport before changing the
        // kinematic body to dynamic; never simulate a multi-kilometre target sweep.
        else if(Stage==71 && Age-StageTime>1)
        {P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);P->ReceiveCargoImpact(650,FVector(1,0,0));Stage=8;StageTime=Age;}
        else if(Stage==8 && Age-StageTime>.8){Shot(TEXT("06-fall-into-water"));Stage=9;StageTime=Age;}
        else if(Stage==9 && Age-StageTime>3 && P->IsControlled() && P->IsInOcean())
        {
            if(P->ResetCount!=0 || P->KnockdownCount!=2 || P->RecoveryCount!=2){Finish(false,TEXT("water_handoff_used_rescue_or_bad_lifecycle"));return;}
            Finish(true,TEXT("six_rendered_views physical_fall_to_swimming=1 rescue=0"));
        }
        return;
    }
    if(Role==TEXT("LateJoin"))
    {
        int32 Others=0;
        for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)
            if(*It!=P && It->KnockdownCount>=1 && It->RecoveryCount>=1 && It->Health<100.f && It->Health>95.f && It->GetCrewPresentation()->SimulatedPartCount()==9) ++Others;
        if(Others>=2 && Age>4) Finish(true,TEXT("late_lifecycle_counters=1 articulated_crew=1"));
        return;
    }
    SawDown|=P->Lifecycle==ECalmyPlayerLifecycleState::KnockedDown;
    SawRecovery|=P->Lifecycle==ECalmyPlayerLifecycleState::Recovering;
    if(Stage==0)
    {
        // Host has moved off P0, so the client can reuse that spawn. Give the
        // owning client a separate walking lane instead of walking into the host.
        if(Role==TEXT("Client") && FMath::Abs(P->GetActorLocation().Y)>18) P->MoveIntent(FVector2D(0,P->GetActorLocation().Y<0?1:-1));
        else if(P->GetActorLocation().X-Initial.X<220) P->MoveIntent(FVector2D(1,0));
        else
        {
            FMinimalViewInfo View;P->CalcCamera(DT,View);
            const float ExpectedFov=UCalmyPreferences::Get()->FieldOfView;
            // FOV comes from preferences (default 92); steady mode does not change FOV.
            if(!FMath::IsNearlyEqual(View.FOV,ExpectedFov) || View.Location.ContainsNaN()) { Finish(false,TEXT("bodycam_invalid"));return; }
            P->ToggleSteadyCamera();P->CalcCamera(DT,View);
            if(!P->IsSteadyCamera() || !FMath::IsNearlyEqual(View.FOV,ExpectedFov)) { Finish(false,TEXT("comfort_mode_invalid"));return; }
            P->ToggleSteadyCamera();
            const float OriginalMotion=P->GetCameraMotionScale();
            P->CycleCameraMotion();P->CycleCameraMotion();P->CycleCameraMotion();
            if(!FMath::IsNearlyEqual(OriginalMotion,P->GetCameraMotionScale())) {Finish(false,TEXT("motion_cycle_invalid"));return;}
            const uint32 Seq=++P->LastIssuedSequence;P->ServerStumble(Seq);P->ServerStumble(Seq);
            Stage=1;StageTime=Age;
        }
    }
    else if(Stage==1 && SawDown && SawRecovery && P->IsControlled() && P->RecoveryCount==1)
    {
        if(!SawFreeRoot || LowestRoot>Initial.Z-35.f || P->ResetCount!=0)
        { Finish(false,TEXT("root_did_not_fall_physically_or_used_rescue"));return; }
        if(P->KnockdownCount!=1 || P->RejectedCommands<1) { Finish(false,TEXT("duplicate_stumble_accepted"));return; }
        if(P->Health>=100.f || P->Health<=95.f) { Finish(false,TEXT("stumble_health_not_replicated_or_duplicate_damage"));return; }
        UE_LOG(LogTemp,Display,TEXT("CALMY_CHARACTER %s READY"),*Role);GLog->Flush();Stage=2;
    }
    else if(Stage==2 && Age>(Role==TEXT("Host")?75.f:35.f))
        Finish(true,FString::Printf(TEXT("free_server_torso=1 root_drop_cm=%.1f walked=1 stumble=1 powered_recovery=1 replay_rejected=1 bodycam=1 comfort_mode=1 rescue=0"),Initial.Z-LowestRoot));
}
