#include "Testing/CalmyPauseProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/OutputDeviceRedirector.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"

bool UCalmyPauseProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value;
    const auto* W = Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyPauseProbe="), Value);
#endif
}

void UCalmyPauseProbe::Finish(bool bPass, const FString& Detail)
{
    UE_LOG(LogTemp, Display, TEXT("CALMY_PAUSE_TEST %s %s %s"), *Role, bPass?TEXT("PASS"):TEXT("FAIL"), *Detail);
    GLog->Flush(); Stage=999;
    FPlatformMisc::RequestExitWithStatus(false, bPass?0:1);
}

void UCalmyPauseProbe::Tick(float)
{
    if (Stage==999 || !GetWorld()->HasBegunPlay()) return;
    if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyPauseProbe="), Role);
    const auto Mode=GetWorld()->GetNetMode();
    const bool Preferences=Role==TEXT("PrefsWrite") || Role==TEXT("PrefsRead");
    const bool bSolo=Role==TEXT("Solo") || Role==TEXT("Visual") || Preferences;
    if ((bSolo && Mode!=NM_Standalone) || (Role==TEXT("Host") && Mode!=NM_ListenServer)
        || (!bSolo && Role!=TEXT("Host") && Mode!=NM_Client)) return;
    const double Now=FPlatformTime::Seconds();
    if (!Start) Start=Now;
    if (Now-Start>100) { Finish(false,TEXT("timeout")); return; }
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    if (!PC || !PC->IsLocalController() || !P || Now-Start<2) return;
    if(Preferences)
    {
        FString UserDir;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
        // UE 5.8's GGameUserSettingsIni is a config key, not necessarily a filesystem path.
        const FConfigBranch* Branch=GConfig->FindBranch(FName(*GGameUserSettingsIni),GGameUserSettingsIni);
        const FString UserFull=FPaths::ConvertRelativePathToFull(UserDir),IniFull=Branch?FPaths::ConvertRelativePathToFull(Branch->IniPath):FString();
        const FString TestRoot=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("Saved/Verification"));
        if(FApp::IsUnattended() || UserDir.IsEmpty() || !Branch || IniFull.IsEmpty() || !FPaths::IsUnderDirectory(UserFull,TestRoot) || !FPaths::IsUnderDirectory(IniFull,UserFull))
        {Finish(false,TEXT("preferences_test_requires_isolated_user_dir"));return;}
        if(Role==TEXT("PrefsWrite"))
        {
            if(!P->IsSteadyCamera()) P->ToggleSteadyCamera();
            for(int32 I=0;I<3 && !FMath::IsNearlyEqual(P->GetCameraMotionScale(),.3f);++I) P->CycleCameraMotion();
            P->AdjustLookSensitivity(1.75f-P->GetLookSensitivity());
        }
        const bool OK=P->IsSteadyCamera() && FMath::IsNearlyEqual(P->GetCameraMotionScale(),.3f) && FMath::IsNearlyEqual(P->GetLookSensitivity(),1.75f);
        UE_LOG(LogTemp,Display,TEXT("G1_PREFERENCES %s steady=%d motion=%.2f sensitivity=%.2f ini=%s"),*Role,P->IsSteadyCamera(),P->GetCameraMotionScale(),P->GetLookSensitivity(),*IniFull);
        Finish(OK,Role==TEXT("PrefsWrite")?TEXT("isolated_profile_saved=1"):TEXT("fresh_process_loaded_preferences=1"));return;
    }
    if (Stage==0)
    {
        UE_LOG(LogTemp,Display,TEXT("CALMY_PAUSE_TEST %s START"),*Role); GLog->Flush();
        if (FParse::Param(FCommandLine::Get(),TEXT("CalmyStartPaused")))
        {
            if(!PC->IsPauseMenuOpen() || !GetWorld()->IsPaused()) {Finish(false,TEXT("startup_pause_failed"));return;}
            PC->ResumeFromMenu();
        }
        P->BeginJumpInput();
        PC->SetPauseMenuOpen(true); PC->SetPauseMenuOpen(true); // Idempotent: no stacked input locks.
        if (!PC->IsPauseMenuOpen() || !PC->IsMoveInputIgnored() || !PC->IsLookInputIgnored() || !PC->bShowMouseCursor)
        { Finish(false,TEXT("menu_did_not_block_input"));return; }
        P->MoveIntent(FVector2D(1,0));
        if (!P->GetPendingMovementInputVector().IsNearlyZero() || P->bPressedJump)
        { Finish(false,TEXT("movement_or_jump_leaked_into_menu"));return; }
        if (GetWorld()->IsPaused()!=bSolo) { Finish(false,TEXT("incorrect_world_pause_policy"));return; }
        WorldTimeAtOpen=GetWorld()->GetTimeSeconds(); StageStart=Now; Stage=1;
    }
    else if (Stage==1 && Now-StageStart>1)
    {
        if (Role==TEXT("Visual"))
        {
            FString Label=TEXT("PauseReview"); FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Label);
            const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));
            IFileManager::Get().MakeDirectory(*Dir,true);
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("pause-menu.png"),true,false);
        }
        Stage=2;
    }
    else if (Stage==2 && Now-StageStart>3)
    {
        const float ElapsedWorld=GetWorld()->GetTimeSeconds()-WorldTimeAtOpen;
        if ((bSolo && ElapsedWorld>.001f) || (!bSolo && ElapsedWorld<1))
        { Finish(false,FString::Printf(TEXT("incorrect_clock delta=%.3f"),ElapsedWorld));return; }
        const bool Steady=P->IsSteadyCamera(); const float Motion=P->GetCameraMotionScale();
        PC->MenuCamera();
        if (P->IsSteadyCamera()==Steady) { Finish(false,TEXT("paused_camera_button_failed"));return; }
        PC->MenuCamera();PC->MenuMotion();PC->MenuMotion();PC->MenuMotion();
        const float Sensitivity=P->GetLookSensitivity();
        PC->MenuLookUp();
        if(P->GetLookSensitivity()<=Sensitivity) {Finish(false,TEXT("paused_sensitivity_failed"));return;}
        PC->MenuLookDown();
        if(!FMath::IsNearlyEqual(Sensitivity,P->GetLookSensitivity())) {Finish(false,TEXT("sensitivity_restore_failed"));return;}
        if (!FMath::IsNearlyEqual(Motion,P->GetCameraMotionScale())) { Finish(false,TEXT("paused_motion_cycle_failed"));return; }
        PC->ResumeFromMenu();PC->ResumeFromMenu();
        if (PC->IsPauseMenuOpen() || GetWorld()->IsPaused() || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored() || PC->bShowMouseCursor)
        { Finish(false,TEXT("resume_left_pause_or_input_lock"));return; }
        P->MoveIntent(FVector2D(1,0));
        if (P->GetPendingMovementInputVector().IsNearlyZero()) { Finish(false,TEXT("movement_not_restored"));return; }
        P->ConsumeMovementInputVector();
        for(int32 I=0;I<3;++I) { PC->TogglePauseMenu();PC->TogglePauseMenu(); }
        if (PC->IsMoveInputIgnored() || PC->IsLookInputIgnored() || GetWorld()->IsPaused())
        { Finish(false,TEXT("repeated_pause_left_lock"));return; }
        UE_LOG(LogTemp,Display,TEXT("CALMY_PAUSE_TEST %s READY world_delta=%.3f"),*Role,ElapsedWorld);GLog->Flush();
        WorldTimeAtOpen=GetWorld()->GetTimeSeconds();StageStart=Now;Stage=3;
    }
    else if (Stage==3 && Now-StageStart>3)
    {
        if(GetWorld()->GetTimeSeconds()-WorldTimeAtOpen<1) {Finish(false,TEXT("clock_not_resumed"));return;}
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Escape, IE_Pressed, FPlatformTime::Cycles64()));
        StageStart=Now;Stage=4;
    }
    else if (Stage==4 && Now-StageStart>1)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Escape, IE_Released, FPlatformTime::Cycles64()));
        if (!PC->IsPauseMenuOpen()) { Finish(false,TEXT("escape_binding_failed"));return; }
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Enter, IE_Pressed, FPlatformTime::Cycles64()));
        StageStart=Now;Stage=5;
    }
    else if (Stage==5 && Now-StageStart>1)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Enter, IE_Released, FPlatformTime::Cycles64()));
        if (PC->IsPauseMenuOpen() || GetWorld()->IsPaused()) { Finish(false,TEXT("enter_resume_binding_failed"));return; }
        if (Role==TEXT("Host") && Now-Start<75) return; // Keep listen server alive for cold late-join startup.
        if (Role==TEXT("Visual")) { PC->SetPauseMenuOpen(true); StageStart=Now; Stage=6; return; }
        Finish(true,TEXT("input_restore=1 repeat=1 camera=1 world_clock_policy=1 escape_enter=1"));
    }
    else if (Stage==6 && Now-StageStart>1)
    {
        // Exercise the rendered HUD hit-box path, not just the controller's resume method.
        int32 W=0,H=0; PC->GetViewportSize(W,H);
        auto* HUD=PC->GetHUD();
        const FVector2D Click(FMath::Max(16.f,(W-620.f)*.5f)+50,FMath::Max(16.f,(H-450.f)*.5f)+130);
        if(!HUD || !HUD->GetHitBoxAtCoordinates(Click,true)) {Finish(false,TEXT("resume_hitbox_missing"));return;}
        HUD->UpdateAndDispatchHitBoxClickEvents(Click,IE_Pressed);
        if(PC->IsPauseMenuOpen() || GetWorld()->IsPaused()) {Finish(false,TEXT("mouse_resume_failed"));return;}
        Finish(true,TEXT("input_restore=1 repeat=1 camera=1 world_clock_policy=1 escape_enter=1 mouse_hitbox=1"));
    }
}
