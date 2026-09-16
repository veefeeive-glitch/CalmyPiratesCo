#include "Testing/CalmyBridgeMenuProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyShipMotion.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"
bool UCalmyBridgeMenuProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString R;const auto* W=Cast<UWorld>(Outer);
    return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyBridgeMenuProbe="),R);
#endif
}
void UCalmyBridgeMenuProbe::Finish(bool OK,const FString& Detail)
{UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);}
int32 UCalmyBridgeMenuProbe::CrewCount() const
{const auto* GS=GetWorld()->GetGameState();return GS?GS->PlayerArray.Num():0;}
// Riders heave and drift with the hull while the world is live; judge menu stillness in the ship frame.
FVector UCalmyBridgeMenuProbe::ShipLocal(const AActor* Actor) const
{const auto* Ship=ACalmyShipMotion::Find(GetWorld());return Ship?Ship->GetActorTransform().InverseTransformPosition(Actor->GetActorLocation()):Actor->GetActorLocation();}
void UCalmyBridgeMenuProbe::Photo(const FString& Name)
{
    if(Role!=TEXT("Visual"))return;
    FString Label=TEXT("BridgeMenu-R1");FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Label);
    FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));
    IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),true,false);
}
void UCalmyBridgeMenuProbe::Tick(float DT)
{
    if(Stage==999||!GetWorld()->HasBegunPlay())return;
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyBridgeMenuProbe="),Role);
    const double Now=FPlatformTime::Seconds();if(!Start)Start=Now;
    {const int32 CrewNow=CrewCount();if(CrewNow!=LastCrew){LastCrew=CrewNow;LastCrewChange=Now;}}
    if(Now-Start>95){Finish(false,TEXT("timeout"));return;}
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    if(!PC||!P||!P->IsLocallyControlled()||Now-Start<5)return;
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,IE_Pressed,FPlatformTime::Cycles64()));PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,IE_Released,FPlatformTime::Cycles64()));};
    auto* T=PC->GetBridgeTerminal();const bool Solo=GetWorld()->GetNetMode()==NM_Standalone;
    if(Stage==0)
    {
        if(!PC->IsPauseMenuOpen()||!T||PC->GetViewTarget()!=T->MenuCamera||GetWorld()->IsPaused()!=Solo)
        {Finish(false,FString::Printf(TEXT("entry_menu_or_local_pause_failed open=%d terminal=%s view=%s camera=%s paused=%d solo=%d map=%s"),PC->IsPauseMenuOpen(),*GetNameSafe(T),*GetNameSafe(PC->GetViewTarget()),*GetNameSafe(T?T->MenuCamera.Get():nullptr),GetWorld()->IsPaused(),Solo,*GetWorld()->GetMapName()));return;}
        Spawn=P->GetActorLocation();Clock=GetWorld()->GetTimeSeconds();
        PC->SetMouseLocation(4,4); // Keyboard phase must not depend on desktop cursor position.
        if(FMath::Abs(Spawn.X+3800)>80||FMath::Abs(Spawn.Z-1989)>35){Finish(false,TEXT("not_spawned_in_wheelhouse"));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s START spawn=%s"),*Role,*Spawn.ToString());GLog->Flush();
        Photo(TEXT("01-terminal"));Since=Now;Stage=1;
    }
    else if(Stage==1&&Now-Since>2)
    {
        const float Elapsed=GetWorld()->GetTimeSeconds()-Clock;
        if((Solo&&Elapsed>.001f)||(!Solo&&Elapsed<1)||FVector::Dist(P->GetActorLocation(),Spawn)>40)
        {Finish(false,TEXT("pause_clock_or_pawn_motion"));return;}
        Key(EKeys::Down);Since=Now;Stage=11;
    }
    // InputKey queues input for the controller tick. Separate real key presses
    // across frames instead of checking state before that tick can process it.
    else if(Stage==11&&Now-Since>.25)
    {
        float MX=-1,MY=-1;PC->GetMousePosition(MX,MY);
        UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s ENTER page=%d selected=%d mouse=%.0f,%.0f mouse_row=%d"),*Role,T->GetPage(),T->GetSelected(),MX,MY,T->MouseRow());GLog->Flush();
        Key(EKeys::Enter);Since=Now;Stage=12;
    }
    else if(Stage==12&&Now-Since>.25){if(!PC->IsSettingsOpen()){float MX=-1,MY=-1;PC->GetMousePosition(MX,MY);Finish(false,FString::Printf(TEXT("full_settings_not_open open=%d page=%d selected=%d mouse=%.0f,%.0f"),PC->IsPauseMenuOpen(),T->GetPage(),T->GetSelected(),MX,MY));return;}Photo(TEXT("02-settings"));Since=Now;Stage=14;}
    // Screenshots are captured at end of frame: keep the settings panel open until then.
    else if(Stage==14&&Now-Since>.5){Steady=P->IsSteadyCamera();PC->MenuCamera();Since=Now;Stage=13;}
    else if(Stage==13&&Now-Since>.25)
    {
        if(P->IsSteadyCamera()==Steady){Finish(false,FString::Printf(TEXT("terminal_settings_key_failed open=%d page=%d selected=%d"),PC->IsPauseMenuOpen(),T->GetPage(),T->GetSelected()));return;}
        PC->MenuCamera();PC->CloseSettings();Since=Now;Stage=2;
    }
    else if(Stage==2&&Now-Since>2)
    {
        Key(EKeys::Escape);Since=Now;Stage=3;
    }
    else if(Stage==3&&Now-Since>2)
    {
        if(PC->IsPauseMenuOpen()||PC->IsMoveInputIgnored()||PC->IsLookInputIgnored()||GetWorld()->IsPaused()||PC->GetViewTarget()!=P||!P->GetCharacterMovement()->IsMovingOnGround())
        {Finish(false,TEXT("resume_or_floor_failed"));return;}
        Photo(TEXT("03-bodycam-spawn"));Spawn=P->GetActorLocation();Since=Now;Stage=4;
    }
    else if(Stage==4)
    {
        if(Now-Since<.5){P->AddMovementInput(FVector(-1,0,0),1);return;}
        P->ConsumeMovementInputVector();P->GetCharacterMovement()->StopMovementImmediately();
        if(FVector::Dist(P->GetActorLocation(),Spawn)<30||!P->GetCharacterMovement()->IsMovingOnGround())
        {Finish(false,TEXT("walk_after_resume_failed"));return;}
        Spawn=ShipLocal(P);CrewAtMenu=CrewCount();Key(EKeys::Escape);Since=Now;Stage=5;
    }
    else if(Stage==5&&Now-Since>2)
    {
        // A joining crew pawn spawns in the wheelhouse and can shove the idle player.
        // Its capsule keeps settling a few seconds after login, so a join shortly before the menu
        // opened counts too. Judge menu stillness only over a window without recent joins.
        const int32 Crew=CrewCount();const float Moved=FVector::Dist(ShipLocal(P),Spawn);
        if((Crew!=CrewAtMenu||Now-LastCrewChange<5)&&Rebases<5)
        {UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s REBASE crew=%d->%d since_join=%.1f moved=%.1f"),*Role,CrewAtMenu,Crew,Now-LastCrewChange,Moved);GLog->Flush();CrewAtMenu=Crew;++Rebases;Spawn=ShipLocal(P);Since=Now;return;}
        // A menu teleport would jump the pawn >4 m to the terminal. Riding the live hull and replicated
        // ship/pawn updates on clients jitter ~10 cm, so 25 cm keeps a wide margin on both sides.
        if(Moved>25){Finish(false,FString::Printf(TEXT("menu_teleported_pawn moved=%.1f crew=%d"),Moved,Crew));return;}
        if(Role==TEXT("Visual")&&TourStep==0){Since=Now;Stage=20;return;}
        if(Role==TEXT("Visual"))
        {
            FVector2D Pixel;if(!PC->ProjectWorldLocationToScreen(T->RowWorld(0),Pixel)){Finish(false,TEXT("screen_projection"));return;}
            UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s AIM row0_pixel=%.0f,%.0f"),*Role,Pixel.X,Pixel.Y);GLog->Flush();
            PC->SetMouseLocation(Pixel.X,Pixel.Y);Since=Now;Stage=6;return;
        }
        Key(EKeys::Enter);Since=Now;Stage=7;
    }
    // Visual-only page tour: photograph crew session and leave confirmation, back to the first row.
    else if(Stage==20&&Now-Since>.4)
    {
        static const TCHAR* Tour[]={TEXT("Down"),TEXT("Down"),TEXT("Down"),TEXT("Enter"),TEXT("04-crew-session"),TEXT("Down"),TEXT("Down"),TEXT("Down"),TEXT("Enter"),TEXT("Down"),TEXT("Enter"),TEXT("05-leave-confirm"),TEXT("Enter")};
        static const int32 TourPage[]={0,0,0,0,3,3,3,3,3,0,0,2,2};
        if(TourStep<UE_ARRAY_COUNT(Tour))
        {
            if(T->GetPage()!=TourPage[TourStep]){Finish(false,FString::Printf(TEXT("page_tour_failed step=%d page=%d selected=%d"),TourStep,T->GetPage(),T->GetSelected()));return;}
            const FString Step=Tour[TourStep++];
            if(Step==TEXT("Down"))Key(EKeys::Down);else if(Step==TEXT("Enter"))Key(EKeys::Enter);else Photo(Step);
            Since=Now;return;
        }
        if(!PC->IsPauseMenuOpen()||T->GetPage()!=0||T->GetSelected()!=0){Finish(false,FString::Printf(TEXT("page_tour_return_failed open=%d page=%d selected=%d"),PC->IsPauseMenuOpen(),T->GetPage(),T->GetSelected()));return;}
        Since=Now-3;Stage=5;
    }
    else if(Stage==6&&Now-Since>1)
    {
        float MX=-1,MY=-1;PC->GetMousePosition(MX,MY);
        UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s CLICK mouse=%.0f,%.0f row=%d view=%s"),*Role,MX,MY,T->MouseRow(),*GetNameSafe(PC->GetViewTarget()));GLog->Flush();
        Key(EKeys::LeftMouseButton);Since=Now;Stage=7;
    }
    else if(Stage==7&&Now-Since>1)
    {
        if(PC->IsPauseMenuOpen()||PC->IsMoveInputIgnored()){Finish(false,TEXT("final_resume_failed"));return;}
        // A client asking for training replay must not travel the host session.
        if(GetWorld()->GetNetMode()==NM_Client){PC->RequestReplayTutorial();Since=Now;Stage=71;return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s READY"),*Role);GLog->Flush();Stage=8;
    }
    else if(Stage==71&&Now-Since>4)
    {
        if(ACalmyTutorial::IsForcedReplay()||PC->IsPauseMenuOpen()||!GetWorld()->GetMapName().Contains(TEXT("OceanFreighterR7")))
        {Finish(false,FString::Printf(TEXT("client_replay_not_host_only map=%s"),*GetWorld()->GetMapName()));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_BRIDGE_MENU %s READY client_replay_ignored"),*Role);GLog->Flush();Stage=8;
    }
    else if(Stage==8&&(Role!=TEXT("Host")||Now-Start>65))Finish(true,TEXT("bridge_spawn grounded menu keys settings resume local_clock walked no_teleport client_replay_host_only"));
}
