#include "Testing/CalmySettingsProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmySettingsWidget.h"
#include "Gameplay/CalmyPreferences.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
bool UCalmySettingsProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V;const auto* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmySettingsProbe="),V);
#endif
}
void UCalmySettingsProbe::Finish(bool OK,const FString& Detail)
{UE_LOG(LogTemp,Display,TEXT("CALMY_SETTINGS_TEST %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);}
void UCalmySettingsProbe::Press(ACalmyPlayerController* PC,FName Action)
{
    auto W=PC->GetSettingsWidget();auto Button=W.IsValid()?W->GetActionWidget(Action):nullptr;
    if(!Button.IsValid()){Finish(false,TEXT("missing_widget_")+Action.ToString());return;}
    auto& Slate=FSlateApplication::Get();Slate.SetKeyboardFocus(Button,EFocusCause::SetDirectly);
    const FKeyEvent Key(EKeys::Enter,FModifierKeysState(),0,false,0,0);
    Slate.ProcessKeyDownEvent(Key);Slate.ProcessKeyUpEvent(Key);
}
void UCalmySettingsProbe::Tick(float Dt)
{
    if(Stage==999||!GetWorld()->HasBegunPlay())return;
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;if(!P)return;
    const double Now=FPlatformTime::Seconds();if(!Start){Start=Since=Now;FParse::Value(FCommandLine::Get(),TEXT("CalmySettingsProbe="),Role);}
    if(Now-Start>150){Finish(false,FString::Printf(TEXT("timeout stage=%d"),Stage));return;}
    if(Now-Since<1)return;
    if(Stage==0)
    {
        FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Dir);
        if(Dir.IsEmpty())Dir=TEXT("Settings-S1-")+Role+TEXT("-")+FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Dir);IFileManager::Get().MakeDirectory(*Folder,true);
        // All settings writes (including engine SaveSettings) stay in this process's isolated file.
        GGameUserSettingsIni=FPaths::ConvertRelativePathToFull(Folder/(Role+TEXT("-GameUserSettings.ini")));
        UCalmyPreferences::Get()->Bindings.Reset();
        PC->OpenSettings();PawnPosition=P->GetActorLocation();
        UE_LOG(LogTemp,Display,TEXT("CALMY_SETTINGS_TEST %s START"),*Role);GLog->Flush();
        Stage=1;Since=Now;return;
    }
    auto W=PC->GetSettingsWidget();
    auto Photo=[&](const TCHAR* Name){if(Role==TEXT("Visual"))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/Name,true,false);};
    if(Stage==1)
    {
        if(!W.IsValid()||!PC->IsMoveInputIgnored()||!PC->IsLookInputIgnored()||GetWorld()->IsPaused()!=(GetWorld()->GetNetMode()==NM_Standalone))
        {Finish(false,TEXT("settings_input_or_pause_lease"));return;}
        Photo(TEXT("01-display.png"));Stage=101;
    }
    else if(Stage==101){Press(PC,TEXT("Tab1"));Stage=2;}
    else if(Stage==2)
    {
        if(W->GetPage()!=1){Finish(false,TEXT("slate_tab_navigation"));return;}
        Photo(TEXT("02-graphics.png"));Stage=102;
    }
    else if(Stage==102){Press(PC,TEXT("Tab2"));Stage=103;}
    else if(Stage==103)
    {if(W->GetPage()!=2){Finish(false,TEXT("audio_tab_navigation"));return;}Photo(TEXT("06-audio.png"));Stage=104;}
    else if(Stage==104){Press(PC,TEXT("Tab4"));Stage=105;}
    else if(Stage==105)
    {if(W->GetPage()!=4){Finish(false,TEXT("camera_tab_navigation"));return;}Photo(TEXT("07-camera.png"));Stage=106;}
    else if(Stage==106){Press(PC,TEXT("Tab3"));Stage=3;}
    else if(Stage==3)
    {
        Press(PC,TEXT("E"));const FKeyEvent Key(EKeys::Z,FModifierKeysState(),0,false,0,0);
        FSlateApplication::Get().ProcessKeyDownEvent(Key);FSlateApplication::Get().ProcessKeyUpEvent(Key);Stage=4;
    }
    else if(Stage==4)
    {
        if(W->GetDraft()->KeyFor(EKeys::E)!=EKeys::Z){Finish(false,TEXT("slate_key_capture"));return;}
        Photo(TEXT("03-controls.png"));
        W->GetDraft()->Sensitivity=1.75;W->GetDraft()->InvertY=true;W->GetDraft()->MasterVolume=.35;W->GetDraft()->AmbienceVolume=.45;
        W->GetDraft()->ShowFPS=true;W->GetDraft()->MotionBlur=false;W->GetDraft()->FieldOfView=100;
        W->GetVideoDraft().Quality[2]=1;W->GetVideoDraft().Limit=45;Press(PC,TEXT("Apply"));Stage=5;
    }
    else if(Stage==5)
    {
        auto* Loaded=NewObject<UCalmyPreferences>();Loaded->LoadConfig(UCalmyPreferences::StaticClass(),*GGameUserSettingsIni);
        if(!FMath::IsNearlyEqual(P->GetLookSensitivity(),1.75f)||Loaded->KeyFor(EKeys::E)!=EKeys::Z||!FMath::IsNearlyEqual(Loaded->MasterVolume,.35f)||GEngine->GetGameUserSettings()->GetShadowQuality()!=1
            ||IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality"))->GetInt()!=0)
        {Finish(false,TEXT("applied_controls_graphics_or_saved_preferences"));return;}
        if(W->IsConfirmingVideo())W->KeepVideo();
        OldResolution=W->GetVideoDraft().Resolution;OldMode=int32(W->GetVideoDraft().Mode);
        W->GetVideoDraft().Resolution=FIntPoint(1024,768);W->GetVideoDraft().Mode=EWindowMode::Windowed;
        if(OldResolution==FIntPoint(1024,768))W->GetVideoDraft().Resolution=FIntPoint(1152,768);
        Press(PC,TEXT("Apply"));Stage=6;
    }
    else if(Stage==6)
    {if(!W->IsConfirmingVideo()){Finish(false,TEXT("missing_video_confirmation"));return;}Photo(TEXT("04-video-confirmation.png"));Stage=7;}
    else if(Stage==7)
    {
        if(W->IsConfirmingVideo())return;
        if(GEngine->GetGameUserSettings()->GetScreenResolution()!=OldResolution||int32(GEngine->GetGameUserSettings()->GetFullscreenMode())!=OldMode)
        {Finish(false,TEXT("video_timeout_did_not_restore"));return;}
        W->GetVideoDraft().Resolution=FIntPoint(1280,800);W->GetVideoDraft().Mode=EWindowMode::Windowed;
        // Let Slate refresh the Apply button's enabled state after the timeout.
        Stage=107;
    }
    else if(Stage==107){Press(PC,TEXT("Apply"));Stage=8;}
    else if(Stage==8)
    {if(!W->IsConfirmingVideo()){Finish(false,TEXT("second_apply_did_not_open_confirmation"));return;}Press(PC,TEXT("Keep"));Stage=108;}
    else if(Stage==108)
    {
        const auto* VideoSettings=GEngine->GetGameUserSettings();
        if(W->IsConfirmingVideo()||VideoSettings->GetLastConfirmedScreenResolution()!=FIntPoint(1280,800)||VideoSettings->GetLastConfirmedFullscreenMode()!=EWindowMode::Windowed)
        {Finish(false,TEXT("keep_did_not_confirm_video"));return;}
        Photo(TEXT("05-saved.png"));Stage=109;
    }
    else if(Stage==109){Press(PC,TEXT("Back"));Stage=9;}
    else if(Stage==9)
    {
        if(PC->IsSettingsOpen()||!PC->IsPauseMenuOpen()){Finish(false,TEXT("settings_back_closed_wrong_menu"));return;}
        PC->ResumeFromMenu();Stage=10;
    }
    else if(Stage==10)
    {
        if(PC->IsMoveInputIgnored()||PC->IsLookInputIgnored()||GetWorld()->IsPaused()) {Finish(false,TEXT("resume_input_lease_leaked"));return;}
        PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64()));Stage=11;
    }
    else if(Stage==11)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64()));
        if(FVector::Dist(P->GetActorLocation(),PawnPosition)<30){Finish(false,TEXT("walking_after_remapping_failed"));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_SETTINGS_TEST %s READY"),*Role);GLog->Flush();Stage=12;
    }
    else if(Stage==12&&(Role!=TEXT("Host")||Now-Start>120))
        Finish(true,TEXT("five_tabs slate_keyboard_capture apply graphics controls disk_roundtrip video_timeout keep_video resume_walk local_pause=1"));
    Since=Now;
}
