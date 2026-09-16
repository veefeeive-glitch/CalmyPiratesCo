#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmyBridgeDisplays.h"
#include "Gameplay/CalmySettingsWidget.h"
#include "Gameplay/CalmyJoinWidget.h"
#include "Core/CalmySession.h"
#include "GameFramework/GameSession.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Gameplay/CalmyPreferences.h"
#include "Engine/GameViewportClient.h"
#include "Components/AudioComponent.h"
#include "Sound/AmbientSound.h"
#include "AudioDevice.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/ScopeExit.h"
#include "Math/ScaleMatrix.h"
#include "Gameplay/FlySwarmMob.h"
#include "Camera/CameraActor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyMissionBoard.h"
#include "Core/CalmyAudio.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Core/CalmyHudLayout.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "RenderUtils.h"
#include "Core/CalmyBreakdownCompass.h"
#include "Gameplay/CalmyFuelStation.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Components/InputComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"

static bool BridgeEntry(const UWorld* World)
{
    // Existing weather-deck route/door fixtures intentionally keep their original
    // start. BridgeMenuProbe exercises the real wheelhouse entry independently.
    return World && World->GetMapName().Contains(TEXT("OceanFreighterR7"))
        && !FString(FCommandLine::Get()).Contains(TEXT("CalmyShipProbe="))
        && !FString(FCommandLine::Get()).Contains(TEXT("CalmyDoorProbe="));
}

ACalmyPlayerController::ACalmyPlayerController()
{
    PrimaryActorTick.bTickEvenWhenPaused = true;
    bShouldPerformFullTickWhenPaused = true;
}

void ACalmyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    // Pawn handles E in ordinary gameplay. Controller handles it while wheel
    // owns the input lease and the pawn's input component is disabled.
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::TogglePauseMenu).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::MenuConfirm).bExecuteWhenPaused = true;
    auto& Click=InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&ThisClass::MenuClick);
    Click.bExecuteWhenPaused=true;Click.bConsumeInput=false;
    InputComponent->BindKey(EKeys::Up,IE_Pressed,this,&ThisClass::MenuUp).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::Down,IE_Pressed,this,&ThisClass::MenuDown).bExecuteWhenPaused=true;
    // Menu shortcuts do not shadow the pawn's M/V actions during normal play.
    InputComponent->BindKey(EKeys::LeftBracket,IE_Pressed,this,&ThisClass::MenuLookDown).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::RightBracket,IE_Pressed,this,&ThisClass::MenuLookUp).bExecuteWhenPaused=true;
}

void ACalmyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    // Video safety must also run while Slate is hidden/minimized or NullRHI.
    if(SettingsWidget.IsValid())SettingsWidget->UpdateDeadline();
    if(IsLocalController()&&GetPawn()&&!bAppliedLocalPreferences)
    {bAppliedLocalPreferences=true;if(!FApp::IsUnattended())UCalmyPreferences::Get()->Apply(this);}
    if(IsLocalController()&&FPlatformTime::Seconds()>=NextAudioUpdate)
    {NextAudioUpdate=FPlatformTime::Seconds()+1;ApplyAudioPreferences();}
    MaybeLiveHold1Teleport();
    if (!bCheckedStartupPause && IsLocalController() && GetPawn() && GetWorld()->HasBegunPlay())
    {
        bCheckedStartupPause = true;
        if (BridgeEntry(GetWorld()) || FParse::Param(FCommandLine::Get(), TEXT("CalmyStartPaused"))) SetPauseMenuOpen(true);
    }
    if(bPauseMenuOpen&&BridgeTerminal&&!IsSettingsOpen())BridgeTerminal->Hover();
    // Wheelhouse status screens carry the watch, alarms, crew and sea info instead of HUD panels.
    if(!BridgeDisplays&&IsLocalController()&&GetPawn()&&BridgeEntry(GetWorld()))
    {BridgeDisplays=GetWorld()->SpawnActor<ACalmyBridgeDisplays>();if(BridgeDisplays)BridgeDisplays->Initialize(this);}
    // Ambient beds are local presentation: every player builds their own, not only the host.
    if(!bEnsuredAmbience&&IsLocalController()&&GetPawn()){bEnsuredAmbience=true;FCalmyAudio::EnsureWorldAmbience(GetWorld());}
    if (bWheelInputLease)
    {
        const auto* P = Cast<ACalmyCharacter>(GetPawn());
        if (!WheelSwarm.IsValid() || WheelPawn != GetPawn() || !P || P->HeldCargo || P->MeleeHitsReceived != WheelHitsAtOpen || !WheelSwarm->CanConverse(this)) { CloseFlyWheel(); return; }
        int32 W,H; GetViewportSize(W,H); float X,Y;
        if (GetMousePosition(X,Y)) WheelSelection = WheelSector(FVector2D(X-W*.5f,Y-H*.5f), FMath::Min(W,H)*.25f);
    }
}

void ACalmyPlayerController::MenuClick()
{ if(IsSettingsOpen()||IsJoinDialogOpen())return;if (bWheelInputLease) ConfirmFlyWheel(); else if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Click(); }
bool ACalmyPlayerController::InputKey(const FInputKeyEventArgs& Args)
{
    if(Args.Event==IE_Pressed)
    {
        if(IsJoinDialogOpen()) {if(Args.Key==EKeys::Escape)CloseJoinDialog();return true;}
        if(IsSettingsOpen()) {if(Args.Key==EKeys::Escape)CloseSettings();return true;}
        const auto* Pref=UCalmyPreferences::Get();
        if(bWheelInputLease&&Args.Key==Pref->KeyFor(EKeys::E)){CloseFlyWheel();return true;}
        if(bPauseMenuOpen&&Args.Key==Pref->KeyFor(EKeys::V)){MenuCamera();return true;}
        if(bPauseMenuOpen&&Args.Key==Pref->KeyFor(EKeys::M)){MenuMotion();return true;}
    }
    return Super::InputKey(Args);
}
void ACalmyPlayerController::OpenSettings()
{
    if(!IsLocalController()||SettingsWidget.IsValid()||!GEngine||!GEngine->GameViewport)return;
    if(!bPauseMenuOpen)SetPauseMenuOpen(true);
    SettingsWidget=SNew(SCalmySettingsWidget).Controller(this);
    GEngine->GameViewport->AddViewportWidgetContent(SettingsWidget.ToSharedRef(),100);
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(SettingsWidget);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    FSlateApplication::Get().SetKeyboardFocus(SettingsWidget);
}
void ACalmyPlayerController::CloseSettings()
{
    if(!SettingsWidget.IsValid())return;
    SettingsWidget->RevertVideo();
    if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(SettingsWidget.ToSharedRef());
    SettingsWidget.Reset();FlushPressedKeys();
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
void ACalmyPlayerController::OpenJoinDialog()
{
    if(!IsLocalController()||JoinWidget.IsValid()||!GEngine||!GEngine->GameViewport)return;
    if(!bPauseMenuOpen)SetPauseMenuOpen(true);
    CloseSettings();
    JoinWidget=SNew(SCalmyJoinWidget).Controller(this);
    GEngine->GameViewport->AddViewportWidgetContent(JoinWidget.ToSharedRef(),110);
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(JoinWidget);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    FSlateApplication::Get().SetKeyboardFocus(JoinWidget);
}
void ACalmyPlayerController::CloseJoinDialog()
{
    if(!JoinWidget.IsValid())return;
    if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(JoinWidget.ToSharedRef());
    JoinWidget.Reset();FlushPressedKeys();
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();
    if(BridgeTerminal)BridgeTerminal->Refresh();
}
bool ACalmyPlayerController::HostLanSession(FString* OutError)
{
    if(!IsLocalController()) { if(OutError)*OutError=TEXT("Local controller required"); return false; }
    CloseJoinDialog();
    const bool Ok=FCalmySession::HostLanListen(GetWorld(), FCalmySession::DefaultPort, OutError);
    if(BridgeTerminal)BridgeTerminal->Refresh();
    if(Ok && bOwnsWorldPause && GetWorld() && GetWorld()->IsPaused()) { SetPause(false); bOwnsWorldPause=false; }
    return Ok;
}
bool ACalmyPlayerController::JoinLanSession(const FString& Address, FString* OutError)
{
    if(!IsLocalController()) { if(OutError)*OutError=TEXT("Local controller required"); return false; }
    CloseSettings();
    if(bOwnsWorldPause && GetWorld() && GetWorld()->IsPaused()) { SetPause(false); bOwnsWorldPause=false; }
    SetPauseMenuOpen(false);
    return FCalmySession::JoinLan(this, Address, OutError);
}
void ACalmyPlayerController::CopyLanEndpointToClipboard()
{
    if(!IsLocalController()||!GetWorld())return;
    if(GetNetMode()!=NM_ListenServer)
    {
        UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION copy_skipped role=%s"), *FCalmySession::RoleLabel(GetWorld()));
        return;
    }
    const FString Endpoint=FCalmySession::DescribeListenEndpoint(GetWorld());
    FPlatformApplicationMisc::ClipboardCopy(*Endpoint);
    UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION COPY_LAN %s"), *Endpoint);
}
void ACalmyPlayerController::ApplyAudioPreferences()
{
    if(!IsLocalController()||!GetWorld())return;
    const auto* Pref=UCalmyPreferences::Get();
    const bool Silent=Pref->MuteUnfocused&&FSlateApplication::IsInitialized()&&!FSlateApplication::Get().IsActive();
    FApp::SetUnfocusedVolumeMultiplier(Pref->MuteUnfocused?0.f:1.f);
    if(auto* Device=GetWorld()->GetAudioDeviceRaw())Device->SetTransientPrimaryVolume(Silent?0.f:Pref->MasterVolume);
    for(TObjectIterator<UAudioComponent> It;It;++It)
    {
        auto* A=*It;if(A->GetWorld()!=GetWorld()||A->IsTemplate())continue;
        const float Base=OriginalAudioVolumes.FindOrAdd(A,A->VolumeMultiplier);
        const bool Ambient=Cast<AAmbientSound>(A->GetOwner())||A->ComponentHasTag(TEXT("Ambience"));
        A->SetVolumeMultiplier(Base*(Ambient?Pref->AmbienceVolume:Pref->EffectsVolume));
    }
    for(auto It=OriginalAudioVolumes.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
bool ACalmyPlayerController::IsGameplayInputSuppressed() const
{ return bPauseMenuOpen || bWheelInputLease || GetWorld()->GetTimeSeconds() < SuppressGameplayUntil; }
int32 ACalmyPlayerController::WheelSector(FVector2D Offset, float Radius)
{
    if (Offset.Size() < Radius*.32f || Offset.Size() > Radius || Radius <= 0.f) return -1;
    const double Angle = FMath::Atan2(Offset.Y,Offset.X) + PI*.5;
    // Eight sectors: 0-3 talk, 4-7 leave shared marks.
    return (FMath::FloorToInt((Angle + PI*.125) / (PI*.25)) + 8) % 8;
}
bool ACalmyPlayerController::TryOpenFlyWheel()
{
    if (!IsLocalController() || IsGameplayInputSuppressed()) return false;
    auto* Swarm = AFlySwarmMob::FindAimedSwarm(this);
    auto* P = Cast<ACalmyCharacter>(GetPawn());
    if (!Swarm || !P) return false;
    WheelSwarm = Swarm; WheelPawn = P; bWheelInputLease = true; WheelSelection = -1;
    WheelHitsAtOpen = P->MeleeHitsReceived;
    bWheelPreviousCursor = bShowMouseCursor; bWheelPreviousClicks = bEnableClickEvents;
    P->EndRepairInput(); P->EndJumpInput(); P->ConsumeMovementInputVector(); P->DisableInput(this);
    FlushPressedKeys(); SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
    bShowMouseCursor = true; bEnableClickEvents = true;
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
    int32 W,H; GetViewportSize(W,H); SetMouseLocation(W/2,H/2);
    return true; // Local UI lease; never pauses the world or sends a gameplay RPC.
}
void ACalmyPlayerController::CloseFlyWheel()
{
    if (!bWheelInputLease) return;
    bWheelInputLease = false; WheelSwarm.Reset(); WheelSelection = -1;
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    if (WheelPawn.IsValid() && WheelPawn == GetPawn()) WheelPawn->EnableInput(this);
    WheelPawn.Reset(); FlushPressedKeys();
    bShowMouseCursor = bWheelPreviousCursor; bEnableClickEvents = bWheelPreviousClicks;
    SetInputMode(FInputModeGameOnly()); SuppressGameplayUntil = GetWorld()->GetTimeSeconds() + .2;
}
void ACalmyPlayerController::WheelInteractKey() { if (bWheelInputLease) CloseFlyWheel(); }
float ACalmyPlayerController::GetFlyTalkCooldown() const
{ return FMath::Max(0.f, float(4. - (GetWorld()->GetTimeSeconds() - LastFlyTalkRequest))); }
void ACalmyPlayerController::ConfirmFlyWheel()
{
    if (!bWheelInputLease || WheelSelection < 0 || GetFlyTalkCooldown() > 0.f) return;
    auto* Swarm = WheelSwarm.Get();
    const int32 Selection = WheelSelection;
    CloseFlyWheel();
    if (!IsValid(Swarm) || GetWorld()->GetTimeSeconds() - LastFlyTalkRequest < 4.) return;
    LastFlyTalkRequest = GetWorld()->GetTimeSeconds();
    // Even sectors = talk (cardinals); odd = leave shared marks (diagonals).
    if ((Selection & 1) == 0) ServerTalkToSwarm(Swarm, static_cast<EFlyUtterance>(Selection / 2));
    else ServerInscribeSwarm(Swarm, static_cast<EFlyMessageKind>(Selection / 2));
}
void ACalmyPlayerController::RequestFlyTalk(EFlyUtterance Intent)
{
    if (bPauseMenuOpen || GetWorld()->GetTimeSeconds() - LastFlyTalkRequest < 4.) return;
    if (auto* Swarm = AFlySwarmMob::FindConversationalSwarm(this))
    { LastFlyTalkRequest = GetWorld()->GetTimeSeconds(); ServerTalkToSwarm(Swarm, Intent); }
}
void ACalmyPlayerController::ServerTalkToSwarm_Implementation(AFlySwarmMob* Swarm, EFlyUtterance Intent)
{
    if (IsValid(Swarm)) Swarm->ReceiveUtterance(this, Intent);
}
void ACalmyPlayerController::ServerInscribeSwarm_Implementation(AFlySwarmMob* Swarm, EFlyMessageKind Kind)
{
    if (IsValid(Swarm)) Swarm->ReceiveInscription(this, Kind);
}
void ACalmyPlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
    Super::ClientRestart_Implementation(NewPawn);
    // Possession acknowledgement can arrive after the initial local menu opened.
    // Keep its view/input lease instead of letting the restart steal the camera.
    if(bPauseMenuOpen && NewPawn)
    {
        MenuPawn=NewPawn;NewPawn->DisableInput(this);
        SetIgnoreMoveInput(true);SetIgnoreLookInput(true);
        if(BridgeTerminal) {PreviousMenuView=NewPawn;SetViewTarget(BridgeTerminal->MenuCamera);}
    }
}
void ACalmyPlayerController::ClientSetViewTarget_Implementation(AActor* Target,FViewTargetTransitionParams Params)
{
    // Engine server camera verification cannot resolve the owner-local terminal
    // camera, and sends the pawn back. Preserve that target for after the menu.
    if(bPauseMenuOpen && BridgeTerminal)
    {
        PreviousMenuView=IsValid(Target)?Target:GetPawn();return;
    }
    Super::ClientSetViewTarget_Implementation(Target,Params);
}
void ACalmyPlayerController::MenuUp()
{
	if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal){BridgeTerminal->Step(-1);return;}
	if(!bPauseMenuOpen)RequestMissionBoardStep(-1);
}
void ACalmyPlayerController::MenuDown()
{
	if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal){BridgeTerminal->Step(1);return;}
	if(!bPauseMenuOpen)RequestMissionBoardStep(1);
}
void ACalmyPlayerController::MenuConfirm()
{
	if(IsSettingsOpen()||IsJoinDialogOpen())return;
	if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Activate();
	else if(ACalmyMissionBoard::IsPortMap(GetWorld()))RequestMissionBoardAccept();
	else ResumeFromMenu();
}

void ACalmyPlayerController::TogglePauseMenu() {if(IsJoinDialogOpen()){CloseJoinDialog();return;} if(IsSettingsOpen()){CloseSettings();return;} if (bWheelInputLease) CloseFlyWheel(); else SetPauseMenuOpen(!bPauseMenuOpen); }
void ACalmyPlayerController::ResumeFromMenu() { SetPauseMenuOpen(false); }
void ACalmyPlayerController::RequestReplayTutorial()
{
	if (!GetWorld() || !FCalmySession::CanRequestSessionTravel(GetNetMode(), IsLocalController())) return;
	CloseSettings();
	SetPauseMenuOpen(false);
	ACalmyTutorial::BeginForcedReplay();
	TravelToTrainingMap();
}
void ACalmyPlayerController::RequestBeginVoyage()
{
	if (!IsLocalController()) return;
	ServerBeginVoyage();
}
void ACalmyPlayerController::RequestRestartVoyage()
{
	if (!IsLocalController()) return;
	const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	ServerRestartVoyage(Voyage ? Voyage->State.Round : 1);
}
void ACalmyPlayerController::RequestReturnToPort()
{
	if (!IsLocalController()) return;
	ServerReturnToPort();
}
void ACalmyPlayerController::RequestMissionBoardStep(int32 Direction)
{
	if (!IsLocalController() || !ACalmyMissionBoard::IsPortMap(GetWorld())) return;
	ServerMissionBoardStep(Direction);
}
void ACalmyPlayerController::RequestMissionBoardAccept()
{
	if (!IsLocalController() || !ACalmyMissionBoard::IsPortMap(GetWorld())) return;
	ServerMissionBoardAccept();
}
void ACalmyPlayerController::ServerBeginVoyage_Implementation()
{
	auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (Voyage && Crew) Voyage->TryBeginWatch(Crew);
}
void ACalmyPlayerController::ServerRestartVoyage_Implementation(int32 ExpectedRound)
{
	auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (Voyage && Crew) Voyage->TryRestartWatch(Crew, ExpectedRound);
}
void ACalmyPlayerController::ServerReturnToPort_Implementation()
{
	auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (Voyage && Crew) Voyage->TryReturnToPort(Crew);
}
void ACalmyPlayerController::ServerMissionBoardStep_Implementation(int32 Direction)
{
	auto* Board = ACalmyMissionBoard::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (Board && Crew) Board->TryStep(Crew, Direction);
}
void ACalmyPlayerController::ServerMissionBoardAccept_Implementation()
{
	auto* Board = ACalmyMissionBoard::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (Board && Crew) Board->TryAccept(Crew);
}
void ACalmyPlayerController::TravelToTrainingMap()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client) return;
	if (!FPackageName::DoesPackageExist(ACalmyTutorial::TrainingMap()) ||
		!World->ServerTravel(ACalmyTutorial::TrainingMap(), false))
	{
		UE_LOG(LogTemp, Error, TEXT("CALMY_TUTORIAL replay_travel_failed"));
	}
}
void ACalmyPlayerController::MaybeLiveHold1Teleport()
{
	// Harness-only: -CalmyLiveHold1 teleports into HOLD 1 after Begin Watch, then to the green zone once cargo is held.
#if UE_BUILD_SHIPPING
	return;
#else
	if (!IsLocalController() || !GetWorld()) return;
	if (LiveHold1Mode < 0) LiveHold1Mode = FParse::Param(FCommandLine::Get(), TEXT("CalmyLiveHold1")) ? 1 : 0;
	if (LiveHold1Mode == 0) return;
	const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	auto* Crew = Cast<ACalmyCharacter>(GetPawn());
	if (!Voyage || !Crew || Voyage->State.Phase != ECalmyVoyagePhase::Active) return;
	if (!bLiveHold1Teleported)
	{
		bLiveHold1Teleported = true;
		const FVector Hold1(-850.f, -420.f, 360.f);
		Crew->SetActorLocationAndRotation(Hold1, FRotator(0.f, 90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Display, TEXT("CALMY_LIVE_HOLD1 teleported_to_hold1"));
		return;
	}
	if (!bLiveHold1Delivered && Crew->HeldCargo)
	{
		bLiveHold1Delivered = true;
		const FVector Green(0.f, -830.f, 1130.f);
		Crew->SetActorLocationAndRotation(Green, FRotator(0.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Display, TEXT("CALMY_LIVE_HOLD1 teleported_to_green_zone held=%s"), *GetNameSafe(Crew->HeldCargo));
	}
#endif
}
void ACalmyPlayerController::MenuCamera()
{
    if (bPauseMenuOpen) if (auto* P = Cast<ACalmyCharacter>(GetPawn())) P->ToggleSteadyCamera();
    if(bPauseMenuOpen && BridgeTerminal)BridgeTerminal->Refresh();
}
void ACalmyPlayerController::MenuMotion()
{
    if (bPauseMenuOpen) if (auto* P = Cast<ACalmyCharacter>(GetPawn())) P->CycleCameraMotion();
    if(bPauseMenuOpen && BridgeTerminal)BridgeTerminal->Refresh();
}
void ACalmyPlayerController::MenuLookDown()
{ if(bPauseMenuOpen) if(auto* P=Cast<ACalmyCharacter>(GetPawn())) P->AdjustLookSensitivity(-.25f);if(bPauseMenuOpen && BridgeTerminal)BridgeTerminal->Refresh(); }
void ACalmyPlayerController::MenuLookUp()
{ if(bPauseMenuOpen) if(auto* P=Cast<ACalmyCharacter>(GetPawn())) P->AdjustLookSensitivity(.25f);if(bPauseMenuOpen && BridgeTerminal)BridgeTerminal->Refresh(); }

void ACalmyPlayerController::SetPauseMenuOpen(bool bOpen)
{
    // Local UI only. Never call ServerPause: host/client menus must not freeze other players.
    if (!IsLocalController() || bOpen == bPauseMenuOpen) return;
    if (bOpen) CloseFlyWheel();else {CloseSettings();CloseJoinDialog();}
    bPauseMenuOpen = bOpen;
    if (bOpen)
    {
        if(BridgeEntry(GetWorld()))
        {
            if(!BridgeTerminal){BridgeTerminal=GetWorld()->SpawnActor<ACalmyBridgeTerminal>();BridgeTerminal->Initialize(this);}
            PreviousMenuView=GetViewTarget();BridgeTerminal->Open();
            if(!PreviousMenuView.IsValid() || PreviousMenuView==this)PreviousMenuView=GetPawn();
            bPreviousAutoCamera=bAutoManageActiveCameraTarget;bAutoManageActiveCameraTarget=false;
            SetViewTarget(BridgeTerminal->MenuCamera);
        }
        bPreviousCursor = bShowMouseCursor; bPreviousClicks = bEnableClickEvents;
        MenuPawn = GetPawn();
        if (auto* P = Cast<ACalmyCharacter>(MenuPawn.Get()))
        {
            P->EndRepairInput(); P->EndJumpInput(); P->ConsumeMovementInputVector();
        }
        if (MenuPawn.IsValid()) MenuPawn->DisableInput(this);
        FlushPressedKeys();
        SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
        bShowMouseCursor = true; bEnableClickEvents = true;
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Mode);
        bOwnsWorldPause = IsSoloPause() && !GetWorld()->IsPaused() && SetPause(true);
    }
    else
    {
        if (bOwnsWorldPause) SetPause(false);
        bOwnsWorldPause = false;
        if(BridgeTerminal)
        {
            SetViewTargetWithBlend(PreviousMenuView.IsValid()?PreviousMenuView.Get():GetPawn(),.45f);
            PreviousMenuView.Reset();
            bAutoManageActiveCameraTarget=bPreviousAutoCamera;
        }
        FlushPressedKeys();
        SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
        if (MenuPawn.IsValid() && MenuPawn == GetPawn()) MenuPawn->EnableInput(this);
        MenuPawn.Reset();
        bShowMouseCursor = bPreviousCursor; bEnableClickEvents = bPreviousClicks;
        SetInputMode(FInputModeGameOnly());
    }
    UE_LOG(LogTemp, Display, TEXT("CALMY_PAUSE open=%d world_paused=%d solo=%d"), bOpen, GetWorld()->IsPaused(), IsSoloPause());
}

void ACalmyPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    CloseFlyWheel();
    CloseJoinDialog();
    CloseSettings();
    SetPauseMenuOpen(false);
    if(BridgeTerminal){if(BridgeTerminal->MenuCamera)BridgeTerminal->MenuCamera->Destroy();BridgeTerminal->Destroy();BridgeTerminal=nullptr;}
    Super::EndPlay(Reason);
}

ACalmyGameMode::ACalmyGameMode()
{
	DefaultPawnClass = ACalmyCharacter::StaticClass();
	HUDClass = ACalmyHUD::StaticClass();
    PlayerControllerClass = ACalmyPlayerController::StaticClass();
}

void ACalmyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (GameSession)
	{
		GameSession->MaxPlayers = FCalmySession::MaxCrew;
	}
}

AActor* ACalmyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		for (TActorIterator<APlayerStart> Start(GetWorld()); Start; ++Start)
		{
            const TCHAR* Prefix=BridgeEntry(GetWorld())?TEXT("BR"):TEXT("P");
            if (!Start->ActorHasTag(FName(*FString::Printf(TEXT("%s%d"), Prefix,Index)))) continue;
			bool Occupied = false;
			for (TActorIterator<ACalmyCharacter> Pawn(GetWorld()); Pawn; ++Pawn)
				Occupied |= FVector::DistSquared(Pawn->GetActorLocation(), Start->GetActorLocation()) < FMath::Square(150.f);
			if (!Occupied) return *Start;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ACalmyGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (ACalmyTutorial::ShouldAutoBoard(GetWorld()))
	{
		if (GetWorld()->GetNetMode() != NM_Client)
		{
			if (!FPackageName::DoesPackageExist(ACalmyTutorial::Destination()) ||
				!GetWorld()->ServerTravel(ACalmyTutorial::Destination(), false))
			{
				UE_LOG(LogTemp, Error, TEXT("CALMY_TUTORIAL auto_board_failed"));
			}
		}
		return;
	}
	if (ACalmyTutorial::Enabled(GetWorld()) && !ACalmyTutorial::Find(GetWorld()))
		GetWorld()->SpawnActor<ACalmyTutorial>();
	if (GetWorld()->GetNetMode() != NM_Client && ACalmyVoyageDirector::IsShipMap(GetWorld()) && !ACalmyVoyageDirector::Find(GetWorld()))
	{
		GetWorld()->SpawnActor<ACalmyVoyageDirector>();
		UE_LOG(LogTemp, Display, TEXT("CALMY_VOYAGE director_spawned map=%s"), *GetWorld()->GetMapName());
	}
	if (GetWorld()->GetNetMode() != NM_Client && ACalmyMissionBoard::IsPortMap(GetWorld()) && !ACalmyMissionBoard::Find(GetWorld()))
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Board = GetWorld()->SpawnActor<ACalmyMissionBoard>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
#if WITH_EDITOR
		if (Board) Board->SetActorLabel(TEXT("PortHub_MissionBoard"));
#endif
		UE_LOG(LogTemp, Display, TEXT("CALMY_PORT mission_board_spawned map=%s"), *GetWorld()->GetMapName());
	}
	FCalmyAudio::EnsureWorldAmbience(GetWorld());
}

void ACalmyGameMode::Logout(AController* Exiting)
{
	if (auto* Pawn = Cast<ACalmyCharacter>(Exiting->GetPawn()))
	{
		Pawn->ReleaseCargo();
		if (auto* Leak = ACalmyLeak::Find(GetWorld())) Leak->StopRepair(Pawn);
	}
	Super::Logout(Exiting);
}

void ACalmyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) return;
	// The joining connection and clients still loading the map hold seats before they own a PlayerController.
	int32 LocalPlayers = 0, OpenConnections = 0;
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It) if (It->IsValid() && (*It)->IsLocalController()) ++LocalPlayers;
	if (const UNetDriver* Driver = GetWorld()->GetNetDriver())
		for (const UNetConnection* Connection : Driver->ClientConnections) if (Connection && Connection->GetConnectionState() != USOCK_Closed) ++OpenConnections;
	if (!FCalmySession::CanAdmit(FCalmySession::SeatsTakenAtPreLogin(LocalPlayers, OpenConnections, GetNumPlayers())))
	{
		ErrorMessage = TEXT("This build supports at most four crew members.");
	}
}

bool ACalmyHUD::bReviewCrowded=false;
bool ACalmyHUD::bReviewBreakdown=false;
FVector ACalmyHUD::ReviewBreakdownLocation=FVector::ZeroVector;
bool ACalmyHUD::bLastLayoutValid=true;
int32 ACalmyHUD::LastVisibleTop=0;
int32 ACalmyHUD::LastTopItems=0;
FString ACalmyHUD::LocalizeKeys(const FString& Text) const
{
    const auto* Pref=UCalmyPreferences::Get();
    auto KeyName=[](FKey K){return K==EKeys::LeftMouseButton?FString(TEXT("LMB")):K==EKeys::RightMouseButton?FString(TEXT("RMB")):K==EKeys::SpaceBar?FString(TEXT("SPACE")):K.ToString();};
    FString Display;
    for(int32 I=0;I<Text.Len();)
    {
        if(!FChar::IsAlpha(Text[I])) {Display.AppendChar(Text[I++]);continue;}
        const int32 Begin=I;while(I<Text.Len()&&FChar::IsAlpha(Text[I]))++I;
        const FString Word=Text.Mid(Begin,I-Begin);
        if(Word==TEXT("WASD"))
        {
            bool Changed=false;for(FKey K:{EKeys::W,EKeys::A,EKeys::S,EKeys::D})Changed|=Pref->KeyFor(K)!=K;
            Display+=Changed?KeyName(Pref->KeyFor(EKeys::W))+TEXT("/")+KeyName(Pref->KeyFor(EKeys::A))+TEXT("/")+KeyName(Pref->KeyFor(EKeys::S))+TEXT("/")+KeyName(Pref->KeyFor(EKeys::D)):Word;
            continue;
        }
        FKey Key=Word==TEXT("LMB")?EKeys::LeftMouseButton:(Word==TEXT("SPACE")||Word==TEXT("Space"))?EKeys::SpaceBar:FKey(FName(*Word));
        Display+=UCalmyPreferences::DefaultKeys().Contains(Key)?KeyName(Pref->KeyFor(Key)):Word;
    }
    return Display;
}
void ACalmyHUD::DrawScaledText(const FString& Text,FLinearColor Color,float X,float Y,UFont* Font,float Scale,bool ScalePosition)
{
    Super::DrawText(LocalizeKeys(Text),Color,X,Y,Font,Scale,ScalePosition);
}
void ACalmyHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
    if(const auto* MenuController=Cast<ACalmyPlayerController>(GetOwningPlayerController()))if(MenuController->IsSettingsOpen())return;
    const bool bShipReview = GetWorld()->GetMapName().Contains(TEXT("OceanFreighterR7"));
    auto* PC = Cast<ACalmyPlayerController>(GetOwningPlayerController());
    if (PC && PC->IsPauseMenuOpen())
    {
        if(PC->GetBridgeTerminal())return; // UI is drawn on the physical console screen.
        const float X = FMath::Max(16.f, (Canvas->ClipX - 620.f) * .5f);
        const float Y = FMath::Max(16.f, (Canvas->ClipY - 450.f) * .5f);
        const float Width = FMath::Min(620.f, Canvas->ClipX - 32.f);
        DrawRect(FLinearColor(.008f, .016f, .025f, .82f), 0, 0, Canvas->ClipX, Canvas->ClipY);
        DrawRect(FLinearColor(.025f, .055f, .07f, .98f), X, Y, Width, 450);
        DrawScaledText(PC->IsSoloPause() ? TEXT("PAUSED | CALMY PIRATES CO.") : TEXT("CREW MENU | ONLINE GAME CONTINUES"), FLinearColor(.4f,.9f,.9f), X+24, Y+24, GEngine->GetMediumFont());
        DrawScaledText(PC->IsSoloPause() ? TEXT("World, physics and contract timer are paused.") : TEXT("Only your input is blocked. Cargo, water and other players stay active."), FLinearColor::White, X+24, Y+64);
        auto Button = [&](FName Name, const FString& Label, float Offset)
        {
            float MX=0, MY=0; PC->GetMousePosition(MX, MY);
            const bool Hover = MX >= X+24 && MX <= X+Width-24 && MY >= Y+Offset && MY <= Y+Offset+48;
            DrawRect(Hover ? FLinearColor(.12f,.36f,.4f) : FLinearColor(.07f,.17f,.21f), X+24, Y+Offset, Width-48, 48);
            DrawScaledText(Label, FLinearColor::White, X+40, Y+Offset+14);
            AddHitBox(FVector2D(X+24,Y+Offset), FVector2D(Width-48,48), Name, true);
        };
        Button(TEXT("Resume"), TEXT("RESUME  [Esc / Enter]"), 106);
        if (const auto* P = Cast<ACalmyCharacter>(GetOwningPawn()))
        {
            Button(TEXT("Settings"), TEXT("SETTINGS / НАСТРОЙКИ"), 166);
            Button(TEXT("Motion"), FString::Printf(TEXT("CAMERA MOTION: %.0f%%  [M]"), P->GetCameraMotionScale()*100), 226);
            const float Half=(Width-56)*.5f;
            for(int32 Side=0;Side<2;++Side)
            {
                const float BX=X+24+Side*(Half+8);
                DrawRect(FLinearColor(.07f,.17f,.21f),BX,Y+286,Half,48);
                DrawScaledText(FString::Printf(TEXT("LOOK %s | %.2fx  [%s]"),Side?TEXT("+"):TEXT("-"),P->GetLookSensitivity(),Side?TEXT("]"):TEXT("[")),FLinearColor::White,BX+12,Y+300);
                AddHitBox(FVector2D(BX,Y+286),FVector2D(Half,48),Side?TEXT("LookUp"):TEXT("LookDown"),true);
            }
        }
        DrawScaledText(TEXT("WASD move | Space jump | E interact | G drop | LMB punch / throw"), FLinearColor(.65f,.8f,.85f), X+24,Y+360);
        DrawScaledText(TEXT("H helper | Q handoff | B stumble | R restart completed shift"), FLinearColor(.65f,.8f,.85f), X+24,Y+384);
        DrawScaledText(bShipReview ? TEXT("ON BOARD | Explore cabins, cargo decks and engine room") : ACalmyTutorial::Find(GetWorld()) ? TEXT("Finish platform training to board the ship with your crew") : TEXT("WaterLab free practice | Alt+F4 to close"), FLinearColor(.55f,.65f,.7f), X+24,Y+414);
        return;
    }
    if (PC && PC->IsFlyWheelOpen()) { DrawFlyWheel(PC); return; }
    DrawGameplayHUD(PC, bShipReview);
}

void ACalmyHUD::Toast(const FString& Text,const FLinearColor& Color,float Seconds)
{
    ToastText=Text;ToastColor=Color;ToastUntil=FPlatformTime::Seconds()+Seconds;
}

void ACalmyHUD::DrawGameplayHUD(ACalmyPlayerController* PC, bool bShipReview)
{
    // Minimal HUD: crosshair, a prompt only when something is interactable, health only when hurt, one alert
    // for an active leak, short toasts on watch changes and subtitles for the ship resident. Watch, alarms,
    // crew and sea status live on the wheelhouse screens (ACalmyBridgeDisplays); controls and tips live in
    // Settings -> Подсказки. Fonts are rasterised at the final pixel size in 1080-unit reference space.
    const float S=FMath::Max(.1f,UCalmyPreferences::Get()->HUDScale*FMath::Clamp(Canvas->ClipY/1080.f,.6f,3.f));
    const FVector2D Viewport(Canvas->ClipX/S,Canvas->ClipY/S);
    const UFont* Face=GEngine->GetMediumFont();
    const bool bCanMeasure=FSlateApplication::IsInitialized()&&FSlateApplication::Get().GetRenderer();
    auto Font=[&](float Size,bool bBold,int32 Spacing){FSlateFontInfo F(Face,Size*S,bBold?FName(TEXT("Bold")):FName(TEXT("Regular")));F.LetterSpacing=Spacing;return F;};
    auto Measure=[&](const FString& Str,const FSlateFontInfo& F)->FVector2D
    {
        if(bCanMeasure)return FVector2D(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str,F))/S;
        return FVector2D(Str.Len()*F.Size*.55f,F.Size*1.3f)/S;
    };
    auto Rect=[&](float X,float Y,float W,float H,const FLinearColor& Color){DrawRect(Color,X*S,Y*S,W*S,H*S);};
    auto Text=[&](const FString& Str,float X,float Y,const FSlateFontInfo& F,const FLinearColor& Color)
    {FCanvasTextItem Item(FVector2D(X*S,Y*S),FText::FromString(Str),F,Color);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);};

    struct FLine{FString Text;FLinearColor Color;float Size=13;bool bBold=false;int32 Spacing=0;float Bar=-1;};
    struct FRow{int32 Line=0;FString Text;bool bFirst=true;};
    struct FCard{FLinearColor Accent;TArray<FLine> Lines;float MaxW=600;float Alpha=.72f;bool bBar=true;bool bCenter=false;TArray<FRow> Rows;FVector2D Size=FVector2D::ZeroVector;};
    const float PadX=14,PadY=8,LineGap=2,BarW=150;
    // Word-wraps every line to the block width; a block never grows past MaxW and never clips text.
    auto Build=[&](FCard& Card)
    {
        Card.Rows.Reset();Card.Size=FVector2D::ZeroVector;
        if(Card.Lines.Num()==0)return;
        float W=0,H=PadY*2;
        for(int32 L=0;L<Card.Lines.Num();++L)
        {
            FLine& Line=Card.Lines[L];
            Line.Text=LocalizeKeys(Line.Text).Replace(TEXT(" | "),TEXT("   ·   "));
            const FSlateFontInfo F=Font(Line.Size,Line.bBold,Line.Spacing);
            const float Lead=Line.Bar>=0?BarW+12:0;
            const float Limit=Card.MaxW-2*PadX-Lead;
            TArray<FString> Words;Line.Text.ParseIntoArray(Words,TEXT(" "),false);
            FString Row;bool bFirst=true;
            auto Flush=[&]{const FVector2D M=Measure(Row.IsEmpty()?FString(TEXT(" ")):Row,F);W=FMath::Max(W,float(M.X)+Lead);H+=float(M.Y)+LineGap;Card.Rows.Add({L,Row,bFirst});Row.Reset();bFirst=false;};
            for(const FString& Word:Words)
            {
                const FString Next=Row.IsEmpty()?Word:Row+TEXT(" ")+Word;
                if(!Row.IsEmpty()&&Measure(Next,F).X>Limit){Flush();Row=Word;}
                else Row=Next;
            }
            Flush();
        }
        Card.Size=FVector2D(FMath::Min(W+2*PadX,Card.MaxW),H-LineGap);
    };
    auto DrawCard=[&](const FCard& Card,const FCalmyHudRect& R)
    {
        if(R.IsEmpty())return;
        Rect(R.X,R.Y,R.W,R.H,FLinearColor(.012f,.024f,.03f,Card.Alpha));
        if(Card.bBar)Rect(R.X,R.Y,3,R.H,Card.Accent);
        float Y=R.Y+PadY;
        for(const FRow& Row:Card.Rows)
        {
            const FLine& Line=Card.Lines[Row.Line];
            const FSlateFontInfo F=Font(Line.Size,Line.bBold,Line.Spacing);
            const FVector2D M=Measure(Row.Text.IsEmpty()?FString(TEXT(" ")):Row.Text,F);
            const float Lead=Line.Bar>=0?BarW+12:0;
            float X=Card.bCenter?R.X+(R.W-float(M.X)-Lead)*.5f:R.X+PadX;
            if(Line.Bar>=0)
            {
                if(Row.bFirst)
                {
                    const float Fill=FMath::Clamp(Line.Bar,0.f,1.f);
                    const float BarY=Y+float(M.Y)*.5f-3;
                    Rect(X,BarY,BarW,6,FLinearColor(.07f,.11f,.12f,.95f));
                    Rect(X,BarY,BarW*Fill,6,FMath::Lerp(FLinearColor(.95f,.28f,.2f),FLinearColor(.35f,.9f,.6f),Fill));
                }
                X+=Lead;
            }
            Text(Row.Text,X,Y,F,Line.Color);
            Y+=float(M.Y)+LineGap;
        }
    };

    const FLinearColor Teal(.42f,.86f,.9f),Amber(1.f,.78f,.3f),Dim(.62f,.74f,.76f),White(.93f,.96f,.95f),Red(1.f,.4f,.32f),Green(.5f,.95f,.62f);
    const auto* Shift=ACalmyShift::Find(GetWorld());
    const auto* Tutorial=ACalmyTutorial::Find(GetWorld());
    const auto* Leak=ACalmyLeak::Find(GetWorld());
    const auto* Ocean=ACalmyOcean::Find(GetWorld());
    const auto* Voyage=ACalmyVoyageDirector::Find(GetWorld());
    const auto* Pawn=Cast<ACalmyCharacter>(GetOwningPawn());
    const double Now=FPlatformTime::Seconds();

    // Toasts only when something changes; the rest of the watch lives on the bridge screens.
    if(Pawn&&!bShownControlsTip){bShownControlsTip=true;TipUntil=Now+8;}
    if(Voyage)
    {
        if(int32(Voyage->State.Phase)!=SeenVoyagePhase||Voyage->State.Round!=SeenRound)
        {
            SeenVoyagePhase=int32(Voyage->State.Phase);SeenRound=Voyage->State.Round;
            if(Voyage->State.Phase==ECalmyVoyagePhase::Briefing)Toast(TEXT("WATCH BRIEFING  ·  details on the WATCH screen in the wheelhouse"),Amber,7);
            else if(Voyage->State.Phase==ECalmyVoyagePhase::Outcome)
            {
                FString Head,Tail;const FString Outcome=Voyage->OutcomeText();
                Toast(Outcome.Split(TEXT(" | "),&Head,&Tail)?Head:Outcome,Voyage->State.bShiftFailed?Red:Green,8);
            }
            else Toast(TEXT("WATCH STARTED  ·  deliver the manifest to the green zone"),Teal,7);
        }
    }
    else if(Shift&&int32(Shift->State.Phase)!=SeenShiftPhase)
    {
        SeenShiftPhase=int32(Shift->State.Phase);
        if(Shift->State.Phase==ECalmyShiftPhase::Complete)Toast(FString::Printf(TEXT("SHIFT COMPLETE  ·  $%d  ·  %d/3"),Shift->State.FinalPayout,Shift->State.PerformanceStars),Green,8);
        else if(Shift->State.Phase==ECalmyShiftPhase::Failed)Toast(TEXT("CONTRACT FAILED  ·  time expired"),Red,8);
    }
    if(Ocean)
    {
        const float Strength=ACalmyOcean::SeaStrength(Ocean->WaveTime());
        const int32 Sea=Strength>1.8f?2:Strength>1.f?1:0;
        if(SeenSea>=0&&Sea==2&&SeenSea!=2)Toast(TEXT("STRONG SWELL  ·  current pushes swimmers and loose cargo"),Red,6);
        SeenSea=Sea;
    }

    TArray<FCard> Top;
    auto Pill=[&](const FString& Title,const FLinearColor& Color,const FString& Sub,float Alpha)
    {
        FCard Card{Color};Card.bCenter=true;Card.bBar=false;Card.MaxW=900;Card.Alpha=Alpha;
        Card.Lines.Add({Title,Color,15,true,40});
        if(!Sub.IsEmpty())Card.Lines.Add({Sub,White,13});
        Top.Add(Card);
    };
    if(Leak)
    {
        const auto& L=Leak->State;
        if(L.Phase==ECalmyLeakPhase::Warning)Pill(FString::Printf(TEXT("LEAK WARNING  ·  %d s"),L.WarningSeconds),Amber,TEXT("Find the yellow valve near the green zone"),.78f);
        else if(L.Phase==ECalmyLeakPhase::Leaking)Pill(FString::Printf(TEXT("LEAK  ·  WATER %d%%  ·  REPAIR %d%%"),L.Water,L.Repair),Red,L.Repairer?TEXT("Repairing: stay on the valve"):TEXT("Hold F at the red valve with empty hands"),.78f);
        else if(L.Phase==ECalmyLeakPhase::Draining)Pill(FString::Printf(TEXT("SEALED  ·  PUMPING OUT  ·  WATER %d%%"),L.Water),Teal,FString(),.6f);
    }
    if(Voyage&&Voyage->State.Phase==ECalmyVoyagePhase::Active&&Voyage->State.bFuelStationArmed&&Voyage->State.FuelPercent<=25)
        Pill(FString::Printf(TEXT("FUEL LOW  ·  %d%%"),Voyage->State.FuelPercent),Voyage->State.FuelPercent<=10?Red:Amber,TEXT("Hold F at the green-lit fuel pump"),.74f);
    if(Now<ToastUntil)Pill(ToastText,ToastColor,FString(),.66f);
    if(Tutorial)
    {
        FString Head,Tail;const FString Instruction=Tutorial->Instruction();
        FString Guidance;
        if(Pawn&&Tutorial->Step<ECalmyTutorialStep::Depart)
        {
            const FVector Delta=Tutorial->ObjectiveLocation()-Pawn->GetActorLocation();
            const float Angle=FMath::FindDeltaAngleDegrees(GetOwningPlayerController()->GetControlRotation().Yaw,Delta.Rotation().Yaw);
            Guidance=FString::Printf(TEXT("%s  %.1f m  ·  %s"),Tutorial->Step==ECalmyTutorialStep::Deliver?TEXT("Green zone"):TEXT("Training crate"),Delta.Size2D()/100,
                FMath::Abs(Angle)<30?TEXT("ahead"):FMath::Abs(Angle)>135?TEXT("behind you"):Angle>0?TEXT("turn right"):TEXT("turn left"));
        }
        Pill(Instruction.Split(TEXT(" | "),&Head,&Tail)?Head:Instruction,Amber,Guidance,.62f);
    }
    if(Now<TipUntil)Pill(TEXT("ESC menu  ·  controls and tips: Settings → Подсказки"),Dim,FString(),.5f);

    FLinearColor DotColor=White;FVector DotPx(Canvas->ClipX*.5f,Canvas->ClipY*.5f,1);
    FCard Prompt{White};Prompt.bCenter=true;Prompt.bBar=false;Prompt.MaxW=760;Prompt.Alpha=.5f;
    if(Pawn)
    {
        const auto* Aimed=Pawn->FindGrabTarget();
        const auto Feedback=Pawn->GetGrabFeedback(Aimed);
        DotPx=Project(Pawn->GetInteractionOrigin()+Pawn->GetBaseAimRotation().Vector()*1500.f);
        DotColor=Feedback==ECalmyGrabFeedback::Ready?FLinearColor(.3f,1.f,.6f):Aimed?FLinearColor(1.f,.65f,.2f):White;
        FString PromptText=Pawn->GetInteractionHint();
        if(Pawn->HeldCargo&&Pawn->HeldCargo->Kind==ECalmyCargoKind::Standard)PromptText=TEXT("CARRYING | G put down | LMB throw");
        // Nothing to interact with: no text at all, only the crosshair.
        if(PromptText==ACalmyCharacter::GrabFeedbackText(ECalmyGrabFeedback::NoTarget))PromptText.Reset();
        if(!PromptText.IsEmpty())Prompt.Lines.Add({PromptText,Feedback==ECalmyGrabFeedback::Ready&&Aimed?FLinearColor(.6f,1.f,.75f):White,14});
    }
    FCard Vitals{Red};Vitals.MaxW=460;Vitals.Alpha=.55f;Vitals.bBar=false;
    if(Pawn&&(Pawn->Health<99.5f||Pawn->Lifecycle!=ECalmyPlayerLifecycleState::Controlled))
    {
        const FString State=Pawn->Lifecycle==ECalmyPlayerLifecycleState::Controlled?FString():
            TEXT("   ·   ")+StaticEnum<ECalmyPlayerLifecycleState>()->GetNameStringByValue(static_cast<int64>(Pawn->Lifecycle)).ToUpper();
        Vitals.Lines.Add({FString::Printf(TEXT("HP %.0f%s"),Pawn->Health,*State),White,13,true,40,Pawn->Health/100.f});
    }
    FCard Subtitle{Green};Subtitle.bCenter=true;Subtitle.bBar=false;Subtitle.MaxW=900;Subtitle.Alpha=.5f;
    if(const auto* Swarm=PC?AFlySwarmMob::FindConversationalSwarm(PC):nullptr;Swarm&&!Swarm->Speech.IsEmpty())
    {
        Subtitle.Lines.Add({TEXT("SHIP RESIDENT"),Green,11,true,90});
        Subtitle.Lines.Add({Swarm->Speech,White,16});
    }

#if !UE_BUILD_SHIPPING
    if(bReviewCrowded)
    {
        // Probe-only worst case: every minimal block at once.
        Top.Reset();
        Pill(TEXT("LEAK  ·  WATER 83%  ·  REPAIR 0%"),Red,TEXT("Hold F at the red valve with empty hands"),.78f);
        Pill(TEXT("WATCH STARTED  ·  deliver the manifest to the green zone"),Teal,FString(),.66f);
        Pill(TEXT("ESC menu  ·  controls and tips: Settings → Подсказки"),Dim,FString(),.5f);
        Prompt.Lines.Reset();Prompt.Lines.Add({TEXT("TOO HEAVY ALONE - bring a free teammate within 3m, or ask Misha"),White,14});
        Vitals.Lines.Reset();Vitals.Lines.Add({TEXT("HP 42   ·   RECOVERING"),White,13,true,40,.42f});
        Subtitle.Lines.Reset();Subtitle.Lines.Add({TEXT("SHIP RESIDENT"),Green,11,true,90});
        Subtitle.Lines.Add({TEXT("Someone is near. I want to make contact before the next swell reaches the lower deck."),White,16});
    }
#endif

    // NFS-style breakdown arrow: points at the most urgent breakdown from replicated state.
    FCalmyBreakdownTarget Breakdown;
    if(Pawn)
    {
        const auto* Fuel=ACalmyFuelStation::Find(GetWorld());
        const bool bFuelWatch=Fuel&&Voyage&&Voyage->State.Phase==ECalmyVoyagePhase::Active&&Voyage->State.bFuelStationArmed;
        Breakdown=FCalmyBreakdownCompass::Select(Leak&&Leak->State.Phase==ECalmyLeakPhase::Warning,Leak&&Leak->State.Phase==ECalmyLeakPhase::Leaking,
            Leak?Leak->GetActorLocation():FVector::ZeroVector,bFuelWatch,Voyage?Voyage->State.FuelPercent:100,Fuel?Fuel->GetActorLocation():FVector::ZeroVector);
#if !UE_BUILD_SHIPPING
        if(bReviewBreakdown){Breakdown.Kind=ECalmyBreakdownKind::Leak;Breakdown.Location=ReviewBreakdownLocation;}
#endif
    }
    for(FCard& Card:Top)Build(Card);
    Build(Prompt);Build(Vitals);Build(Subtitle);
    const FString Fps=UCalmyPreferences::Get()->ShowFPS?FString::Printf(TEXT("%.0f FPS   ·   %.1f ms"),1000/FMath::Max(1.f,FApp::GetDeltaTime()*1000),FApp::GetDeltaTime()*1000):FString();
    const FSlateFontInfo FpsFont=Font(12,true,40);
    FCalmyHudLayoutInput In;
    In.Viewport=Viewport;
    const FVector2D ArrowBlock(380,120);
    if(Breakdown.IsValid())In.Top.Add(ArrowBlock);
    for(const FCard& Card:Top)In.Top.Add(Card.Size);
    In.Prompt=Prompt.Size;In.PromptTop=float(DotPx.Y)/S+22;
    In.Subtitle=Subtitle.Size;In.Vitals=Vitals.Size;
    if(!Fps.IsEmpty())In.Fps=Measure(Fps,FpsFont);
    const FCalmyHudLayout Layout=FCalmyHudLayoutSolver::Solve(In);
    bLastLayoutValid=FCalmyHudLayoutSolver::IsValid(Layout,Viewport);
    const int32 TopBase=Breakdown.IsValid()&&Layout.Top.Num()>0?1:0;
    LastVisibleTop=Layout.Top.Num();LastTopItems=Top.Num()+(Breakdown.IsValid()?1:0);

    if(Pawn)
    {
        if(const float Hit=Pawn->GetHitFlash();Hit>0.f)
        {
            const FLinearColor Flash(.6f,.05f,.025f,Hit*.55f);
            DrawRect(Flash,0,0,Canvas->ClipX,8);DrawRect(Flash,0,Canvas->ClipY-8,Canvas->ClipX,8);
            DrawRect(Flash,0,0,8,Canvas->ClipY);DrawRect(Flash,Canvas->ClipX-8,0,8,Canvas->ClipY);
        }
        if(UCalmyPreferences::Get()->ShowCrosshair)DrawRect(DotColor,DotPx.X-2,DotPx.Y-2,4,4);
    }
    auto DrawBreakdownArrow=[&](const FCalmyHudRect& R)
    {
        const bool bCam=PC&&PC->PlayerCameraManager;
        const FCalmyCompassArrow A=FCalmyBreakdownCompass::Arrow(bCam?PC->PlayerCameraManager->GetCameraRotation():Pawn->GetControlRotation(),
            bCam?PC->PlayerCameraManager->GetCameraLocation():Pawn->GetActorLocation(),Breakdown.Location);
        const bool bCritical=FCalmyBreakdownCompass::IsCritical(Breakdown.Kind);
        const float Pulse=bCritical?.78f+.22f*FMath::Sin(float(Now)*6.f):1.f;
        const FLinearColor Base=bCritical?FLinearColor(1.f,.24f,.16f):FLinearColor(1.f,.74f,.22f);
        const FLinearColor Body(Base.R,Base.G,Base.B,.95f*Pulse),Shade(Base.R*.3f,Base.G*.3f,Base.B*.3f,.9f*Pulse);
        const FVector2D Center((R.X+R.W*.5f)*S,(R.Y+48)*S);
        const float Rad=FMath::DegreesToRadians(A.YawDelta),Cos=FMath::Cos(Rad),Sin=FMath::Sin(Rad);
        // The chevron lies on a floor tilted toward the camera: the far end is smaller and flattened, as in NFS Underground.
        auto Point=[&](float X,float Y,float Lift)
        {
            const float RX=X*Cos-Y*Sin,RY=X*Sin+Y*Cos;
            const float Depth=1.f/(1.f+RY*.006f);
            return Center+FVector2D(RX*Depth,RY*.5f*Depth+Lift)*S;
        };
        struct FTri{float AX,AY,BX,BY,CX,CY;};
        static const FTri Tris[]={{0,-46,-32,-4,32,-4},{-12,-4,-12,40,12,40},{-12,-4,12,40,12,-4}};
        for(float Lift:{7.f,0.f})
            for(const FTri& T:Tris)
            {
                FCanvasTriangleItem Tri(Point(T.AX,T.AY,Lift),Point(T.BX,T.BY,Lift),Point(T.CX,T.CY,Lift),GWhiteTexture);
                Tri.SetColor(Lift>0?Shade:Body);Tri.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Tri);
            }
        FString Label=FString(FCalmyBreakdownCompass::Label(Breakdown.Kind))+TEXT("   ·   ")+(A.bArrived?FString(TEXT("HERE")):FString::Printf(TEXT("%.0f m"),A.DistanceMeters));
        if(A.DeckOffset==1)Label+=TEXT("   ·   DECK ABOVE");
        else if(A.DeckOffset==-1)Label+=TEXT("   ·   DECK BELOW");
        else if(A.DeckOffset!=0)Label+=FString::Printf(TEXT("   ·   %d DECKS %s"),FMath::Abs(A.DeckOffset),A.DeckOffset>0?TEXT("UP"):TEXT("DOWN"));
        const FSlateFontInfo F=Font(13,true,50);const FVector2D M=Measure(Label,F);
        Rect(R.X+(R.W-float(M.X))*.5f-10,R.Y+R.H-float(M.Y)-8,float(M.X)+20,float(M.Y)+6,FLinearColor(.012f,.024f,.03f,.55f));
        Text(Label,R.X+(R.W-float(M.X))*.5f,R.Y+R.H-float(M.Y)-5,F,FLinearColor(Base.R,Base.G,Base.B,1));
    };
    if(TopBase&&Pawn)DrawBreakdownArrow(Layout.Top[0]);
    for(int32 I=TopBase;I<Layout.Top.Num();++I)DrawCard(Top[I-TopBase],Layout.Top[I]);
    DrawCard(Prompt,Layout.Prompt);
    DrawCard(Vitals,Layout.Vitals);
    DrawCard(Subtitle,Layout.Subtitle);
    if(!Layout.Fps.IsEmpty())Text(Fps,Layout.Fps.X,Layout.Fps.Y,FpsFont,FLinearColor(.6f,1,.8f));
    if(Tutorial&&Pawn&&Tutorial->Step<ECalmyTutorialStep::Depart)
    {
        const FVector Target=Tutorial->ObjectiveLocation();
        const FVector Screen=Project(Target+FVector(0,0,100))/S;
        const float TopBottom=Layout.Top.Num()?Layout.Top.Last().Bottom():0;
        if(Screen.Z>0&&Screen.X>20&&Screen.X<Viewport.X-220&&Screen.Y>TopBottom+20&&Screen.Y<Viewport.Y-140)
        {
            const FString Label=FString::Printf(TEXT("%s   %.1f m"),Tutorial->Step==ECalmyTutorialStep::Deliver?TEXT("DELIVER HERE"):TEXT("TRAINING CRATE"),FVector::Dist2D(Target,Pawn->GetActorLocation())/100);
            const FSlateFontInfo F=Font(13,true,60);const FVector2D M=Measure(Label,F);
            Rect(Screen.X-10,Screen.Y-6,M.X+20,M.Y+12,FLinearColor(.012f,.024f,.03f,.7f));
            Text(Label,Screen.X,Screen.Y,F,Amber);
        }
    }
}

void ACalmyHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    auto* PC = Cast<ACalmyPlayerController>(GetOwningPlayerController());
    if (!PC || !PC->IsPauseMenuOpen()) return;
    if (BoxName == TEXT("Resume")) PC->ResumeFromMenu();
    else if(BoxName==TEXT("Settings"))PC->OpenSettings();
    else if (BoxName == TEXT("Camera")) PC->MenuCamera();
    else if (BoxName == TEXT("Motion")) PC->MenuMotion();
    else if (BoxName == TEXT("LookDown")) PC->MenuLookDown();
    else if (BoxName == TEXT("LookUp")) PC->MenuLookUp();
}

void ACalmyHUD::DrawFlyWheel(ACalmyPlayerController* PC)
{
    const FVector2D Center(Canvas->ClipX*.5f, Canvas->ClipY*.5f);
    const float Radius = FMath::Min(Canvas->ClipX,Canvas->ClipY)*.28f;
    DrawRect(FLinearColor(.005f,.012f,.018f,.55f),0,0,Canvas->ClipX,Canvas->ClipY);
    const TCHAR* Labels[] = {
        TEXT("Greet"), TEXT("Be wary"), TEXT("Calm gesture"), TEXT("Safe here"),
        TEXT("Recall"), TEXT("Praise flies"), TEXT("Shoo away"), TEXT("Try talking")
    };
    const TCHAR* Details[] = {
        TEXT("Make contact"), TEXT("Leave a warning mark"), TEXT("Show peaceful intent"), TEXT("Mark this place as calm"),
        TEXT("Ask what it remembers"), TEXT("Strengthen goodwill"), TEXT("Warn it to keep away"), TEXT("Invite the colony closer")
    };
    for (int32 Sector=0; Sector<8; ++Sector)
    {
        const bool Selected = PC->GetFlyWheelSelection()==Sector;
        const bool Mark = (Sector & 1) != 0;
        const FLinearColor Color = Selected
            ? (Mark ? FLinearColor(.38f,.28f,.1f,.96f) : FLinearColor(.12f,.38f,.32f,.96f))
            : (Mark ? FLinearColor(.06f,.045f,.02f,.96f) : FLinearColor(.028f,.06f,.075f,.96f));
        TArray<FCanvasUVTri> Triangles;
        for (int32 Segment=0; Segment<12; ++Segment)
        {
            const float Start=-PI*.625f + Sector*PI*.25f + .02f;
            const float A=Start+(PI*.25f-.04f)*Segment/12.f, B=Start+(PI*.25f-.04f)*(Segment+1)/12.f;
            const FVector2D DA(FMath::Cos(A),FMath::Sin(A)), DB(FMath::Cos(B),FMath::Sin(B));
            FCanvasUVTri T;
            T.V0_Pos=Center+DA*Radius*.32f; T.V1_Pos=Center+DA*Radius; T.V2_Pos=Center+DB*Radius;
            T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector; T.V0_Color=T.V1_Color=T.V2_Color=Color; Triangles.Add(T);
            T.V1_Pos=T.V2_Pos; T.V2_Pos=Center+DB*Radius*.32f; Triangles.Add(T);
        }
        FCanvasTriangleItem Item(Triangles,GWhiteTexture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item);
        const float Angle=-PI*.5f+Sector*PI*.25f;
        const FVector2D Label=Center+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius*.68f;
        float W,H; GetTextSize(Labels[Sector],W,H,GEngine->GetSmallFont());
        DrawScaledText(Labels[Sector],Selected?FLinearColor(1,.94f,.65f):FLinearColor(.85f,.92f,.9f),Label.X-W*.5f,Label.Y-H*.5f);
    }
    auto CenterText = [&](const FString& Text,float Y,FLinearColor Color)
    { float W,H; GetTextSize(Text,W,H,GEngine->GetSmallFont()); DrawScaledText(Text,Color,Center.X-W*.5f,Y,GEngine->GetSmallFont()); };
    CenterText(TEXT("FLY SWARM"),Center.Y-Radius-60,FLinearColor(.65f,1,.8f));
    CenterText(TEXT("Talk  ·  Leave a mark for others"),Center.Y-Radius-38,FLinearColor(.75f,.7f,.45f));
    CenterText(PC->GetFlyTalkCooldown()>0.f?FString::Printf(TEXT("Give it a moment... %.1f s"),PC->GetFlyTalkCooldown()):
        PC->GetFlyWheelSelection()<0?TEXT("Choose an action"):Details[PC->GetFlyWheelSelection()],Center.Y+Radius+26,FLinearColor::White);
    CenterText(TEXT("Mouse to choose  /  Click to confirm  /  E or Esc to cancel"),Center.Y+Radius+54,FLinearColor(.7f,.8f,.8f));
    CenterText(TEXT("E"),Center.Y-8,FLinearColor(.7f,.85f,.8f));
}
