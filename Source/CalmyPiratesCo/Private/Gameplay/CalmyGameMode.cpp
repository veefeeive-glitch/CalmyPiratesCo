#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmySettingsWidget.h"
#include "Gameplay/CalmyJoinWidget.h"
#include "Core/CalmySession.h"
#include "GameFramework/GameSession.h"
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
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
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
    if (!bCheckedStartupPause && IsLocalController() && GetPawn() && GetWorld()->HasBegunPlay())
    {
        bCheckedStartupPause = true;
        if (BridgeEntry(GetWorld()) || FParse::Param(FCommandLine::Get(), TEXT("CalmyStartPaused"))) SetPauseMenuOpen(true);
    }
    if(bPauseMenuOpen&&BridgeTerminal&&!IsSettingsOpen())BridgeTerminal->Hover();
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
void ACalmyPlayerController::MenuUp(){if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(-1);}
void ACalmyPlayerController::MenuDown(){if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(1);}
void ACalmyPlayerController::MenuConfirm(){if(IsSettingsOpen()||IsJoinDialogOpen())return;if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Activate();else ResumeFromMenu();}

void ACalmyPlayerController::TogglePauseMenu() {if(IsJoinDialogOpen()){CloseJoinDialog();return;} if(IsSettingsOpen()){CloseSettings();return;} if (bWheelInputLease) CloseFlyWheel(); else SetPauseMenuOpen(!bPauseMenuOpen); }
void ACalmyPlayerController::ResumeFromMenu() { SetPauseMenuOpen(false); }
void ACalmyPlayerController::RequestReplayTutorial()
{
	if (!IsLocalController() || !GetWorld()) return;
	CloseSettings();
	SetPauseMenuOpen(false);
	ACalmyTutorial::BeginForcedReplay();
	if (GetNetMode() == NM_Client) ServerReplayTutorial();
	else TravelToTrainingMap();
}
void ACalmyPlayerController::ServerReplayTutorial_Implementation()
{
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
	if (!FCalmySession::CanAdmit(GetNumPlayers()))
	{
		ErrorMessage = TEXT("This build supports at most four crew members.");
	}
}

void ACalmyHUD::DrawScaledText(const FString& Text,FLinearColor Color,float X,float Y,UFont* Font,float Scale,bool ScalePosition)
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
    Super::DrawText(Display,Color,X,Y,Font,Scale,ScalePosition);
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
    const float UIScale=UCalmyPreferences::Get()->HUDScale;
    const float OriginalX=Canvas->ClipX,OriginalY=Canvas->ClipY;
    Canvas->ClipX/=UIScale;Canvas->ClipY/=UIScale;
    Canvas->Canvas->PushRelativeTransform(FScaleMatrix(FVector(UIScale,UIScale,1)));
    ON_SCOPE_EXIT {Canvas->Canvas->PopTransform();Canvas->ClipX=OriginalX;Canvas->ClipY=OriginalY;};
    if(UCalmyPreferences::Get()->ShowFPS)
    {
        const float Ms=FApp::GetDeltaTime()*1000;
        DrawScaledText(FString::Printf(TEXT("%.0f FPS | %.1f ms"),1000/FMath::Max(1.f,Ms),Ms),FLinearColor(.6,1,.8),Canvas->ClipX-205,18);
    }
	const auto* Shift = ACalmyShift::Find(GetWorld());
    if (const auto* Swarm = AFlySwarmMob::FindConversationalSwarm(PC); Swarm && (!Swarm->Speech.IsEmpty() || Swarm->VisibleInscriptions.Num() > 0))
    {
        const float PanelH = Swarm->Speech.IsEmpty() ? 78.f : 108.f;
        const float Y = Canvas->ClipY - 40.f - PanelH;
        DrawRect(FLinearColor(.02f,.04f,.035f,.90f), 16, Y, FMath::Min(900.f, Canvas->ClipX-32.f), PanelH);
        DrawScaledText(FString::Printf(TEXT("SHIP RESIDENT  |  %s%s"),
            *FFlyCognition::ActivityLabel(Swarm->Activity),
            Swarm->bTravelPlanning ? TEXT("  ·  THINKING") : TEXT("")), FLinearColor(.6f,1.f,.65f), 30, Y+10);
        float LineY = Y + 34.f;
        if (!Swarm->Speech.IsEmpty())
        {
            DrawScaledText(Swarm->Speech, FLinearColor::White, 30, LineY);
            LineY += 24.f;
        }
        // Shared marks: nearest few, Dark Souls appraisal style (xEchoes).
        struct FNearMark { const FFlyInscriptionView* Mark; float DistSq; };
        TArray<FNearMark, TInlineAllocator<8>> Near;
        const FVector Eye = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
        for (const auto& Mark : Swarm->VisibleInscriptions)
        {
            const float DistSq = FVector::DistSquared(Mark.Location, Eye);
            if (DistSq <= FMath::Square(900.f)) Near.Add({&Mark, DistSq});
        }
        Near.Sort([](const FNearMark& A, const FNearMark& B) { return A.DistSq < B.DistSq; });
        FString Marks;
        for (int32 I = 0; I < Near.Num() && I < 3; ++I)
        {
            if (I) Marks += TEXT("   ·   ");
            Marks += FString::Printf(TEXT("[%dx] %s"), Near[I].Mark->Echoes + 1, *FFlyCognition::MessageLabel(Near[I].Mark->Kind));
        }
        if (!Marks.IsEmpty()) DrawScaledText(Marks, FLinearColor(.85f,.78f,.45f), 30, LineY);
    }
	const auto* Tutorial = ACalmyTutorial::Find(GetWorld());
	const auto* Leak = ACalmyLeak::Find(GetWorld());
	DrawRect(FLinearColor(0.025f, .045f, .06f, .82f), 16, 16, FMath::Min(620.f,Canvas->ClipX-32.f), Shift ? 210 : 122);
    DrawScaledText(bShipReview ? TEXT("CALMY PIRATES CO. | OCEAN FREIGHTER R7") : Tutorial ? TEXT("CREW TRAINING | NEXT STOP: OCEAN FREIGHTER") : ACalmyOcean::Find(GetWorld()) ? TEXT("CALMY PIRATES CO. | WATER LAB - FREE PRACTICE") : (Shift ? TEXT("CALMY PIRATES CO. | COASTAL CARGO SHIFT") : TEXT("CALMY PIRATES CO. | DECK TEST")), FLinearColor(.4f,.85f,.9f),30,26,GEngine->GetMediumFont());
	DrawScaledText(TEXT("WASD move/swim | SPACE jump | E interact | G drop | LMB punch / throw | ESC pause"),FLinearColor::White,30,58);
	if (const auto* Pawn = Cast<ACalmyCharacter>(GetOwningPawn()))
	{
        const float Hit = Pawn->GetHitFlash();
        if (Hit>0.f)
        {
            const FLinearColor Flash(.6f,.05f,.025f,Hit*.55f);
            DrawRect(Flash,0,0,Canvas->ClipX,8); DrawRect(Flash,0,Canvas->ClipY-8,Canvas->ClipX,8);
            DrawRect(Flash,0,0,8,Canvas->ClipY); DrawRect(Flash,Canvas->ClipX-8,0,8,Canvas->ClipY);
        }
        const auto* Aimed=Pawn->FindGrabTarget();
        const auto Feedback=Pawn->GetGrabFeedback(Aimed);
        const FVector Dot=Project(Pawn->GetInteractionOrigin()+Pawn->GetBaseAimRotation().Vector()*1500.f)/UIScale;
        const FLinearColor DotColor=Feedback==ECalmyGrabFeedback::Ready?FLinearColor(.3f,1.f,.6f):Aimed?FLinearColor(1.f,.65f,.2f):FLinearColor::White;
        if(UCalmyPreferences::Get()->ShowCrosshair)DrawRect(DotColor,Dot.X-2,Dot.Y-2,4,4);
        FString Hint=Pawn->GetInteractionHint();
        if(Pawn->HeldCargo && Pawn->HeldCargo->Kind==ECalmyCargoKind::Standard)
            Hint=TEXT("CARRYING | G put down | LMB throw");
        float HintWidth=0,HintHeight=0;GetTextSize(Hint,HintWidth,HintHeight,GEngine->GetSmallFont());
        const float HintX=FMath::Max(20.f,(Canvas->ClipX-HintWidth)*.5f),HintY=Canvas->ClipY*.5f+32;
        DrawRect(FLinearColor(.01f,.025f,.035f,.9f),HintX-12,HintY-8,HintWidth+24,HintHeight+16);
        DrawRect(DotColor,HintX-12,HintY-8,3,HintHeight+16);
        DrawScaledText(Hint,FLinearColor::White,HintX,HintY,GEngine->GetSmallFont());
        if(const auto* Ocean=ACalmyOcean::Find(GetWorld()))
        {
            const float Strength=ACalmyOcean::SeaStrength(Ocean->WaveTime());
            DrawScaledText(Strength>1.8f?TEXT("SEA: STRONG SWELL | current pushes swimmers and loose cargo"):
                Strength>1.f?TEXT("SEA: CHANGING SWELL | watch the waves before entering"):
                TEXT("SEA: CALM INTERVAL | stronger waves arrive in sets"),FLinearColor(.5f,.8f,.9f),30,Canvas->ClipY-80);
        }
		const FString State = StaticEnum<ECalmyPlayerLifecycleState>()->GetNameStringByValue(static_cast<int64>(Pawn->Lifecycle));
        DrawScaledText(FString::Printf(TEXT("V %s | M motion %.0f%% | B stumble | H %s"),Pawn->IsSteadyCamera()?TEXT("bodycam"):TEXT("steady camera"),Pawn->GetCameraMotionScale()*100,
            Pawn->CrewHelper?TEXT("ask to wait"):Pawn->FindHelperTarget()?TEXT("ask Misha for help"):TEXT("approach Misha to ask for help")),FLinearColor(.65f,.85f,.9f),30,Canvas->ClipY-30);
        if(Pawn->CrewHelper)
            DrawScaledText(FString::Printf(TEXT("CREW MATE: %s | %.1fm | keep within 3m for heavy cargo"),
                *StaticEnum<ECalmyHelperState>()->GetNameStringByValue(static_cast<int64>(Pawn->CrewHelper->Assistance)),Pawn->CrewHelper->FollowDistance/100),
                FLinearColor(.8f,.8f,.35f),30,Canvas->ClipY-55);
		DrawScaledText(FString::Printf(TEXT("HP %.0f | %s | Cargo: %s | %s"), Pawn->Health, *State, Pawn->HeldCargo ? TEXT("HELD") : TEXT("none"),
			*FCalmySession::StatusLine(GetWorld())), FLinearColor::White, 30, 82);
		const ACalmyCargo* Target = Pawn->HeldCargo ? Pawn->HeldCargo.Get() : Pawn->FindGrabTarget();
        DrawScaledText(Pawn->IsInOcean() ? (bShipReview ? TEXT("SURFACE SWIM | Boarding route not implemented in this review") : TEXT("SURFACE SWIM | free hands | return along the orange-railed ramp")) : (Target ? Target->GetHandlingHint(Pawn) : (bShipReview ? TEXT("Explore the ship | E cargo | Q handoff | green delivery zone") : ACalmyOcean::Find(GetWorld()) ? TEXT("E cargo | Q handoff | test buoyancy at the water ramp") : TEXT("E cargo | Q handoff | deliver to the green zone"))),FLinearColor(.65f,.8f,.85f),30,106);
	}
	if (Tutorial)
	{
        DrawScaledText(Tutorial->Instruction(),FLinearColor(1,.8f,.3f),30,132);
        DrawScaledText(TEXT("No time limit | Shared crew lesson | Then: ship, cabins and engine room"),FLinearColor(.5f,.85f,.9f),30,158);
        FString Guidance=TEXT("Misha and water: optional practice | Entire crew travels together");
        if(GetOwningPawn() && Tutorial->Step<ECalmyTutorialStep::Depart)
        {
            const FVector Delta=Tutorial->ObjectiveLocation()-GetOwningPawn()->GetActorLocation();
            const float Angle=FMath::FindDeltaAngleDegrees(GetOwningPlayerController()->GetControlRotation().Yaw,Delta.Rotation().Yaw);
            Guidance=FString::Printf(TEXT("%s: %.1f m | %s"),Tutorial->Step==ECalmyTutorialStep::Deliver?TEXT("Green delivery zone"):TEXT("Wooden training crate"),Delta.Size2D()/100,
                FMath::Abs(Angle)<30?TEXT("AHEAD"):FMath::Abs(Angle)>135?TEXT("TURN AROUND"):Angle>0?TEXT("TURN RIGHT"):TEXT("TURN LEFT"));
        }
        DrawScaledText(Guidance,FLinearColor::White,30,184);
        if(GetOwningPawn() && Tutorial->Step<ECalmyTutorialStep::Depart)
        {
            const FVector Target=Tutorial->ObjectiveLocation();
            const FVector Screen=Project(Target+FVector(0,0,100));
            if(Screen.Z>0 && Screen.X>20 && Screen.X<Canvas->ClipX-180 && Screen.Y>240 && Screen.Y<Canvas->ClipY-120)
                DrawScaledText(FString::Printf(TEXT("%s  %.1f m"),Tutorial->Step==ECalmyTutorialStep::Deliver?TEXT("DELIVER HERE"):TEXT("TRAINING CRATE"),FVector::Dist2D(Target,GetOwningPawn()->GetActorLocation())/100),FLinearColor(1,.85f,.2f),Screen.X,Screen.Y);
        }
	}
	else if (Shift)
	{
		const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
		const auto& S = Shift->State;
		if (Voyage && Voyage->State.Phase == ECalmyVoyagePhase::Briefing)
		{
			DrawScaledText(Voyage->BriefingText(), FLinearColor(1.f, .8f, .3f), 30, 132);
			DrawScaledText(TEXT("Host: Esc bridge BEGIN WATCH NOW, or wait for auto-start | Leak valve near green zone"), FLinearColor(.5f, .85f, .9f), 30, 158);
			DrawScaledText(TEXT("Contract: 1 STANDARD + 1 BULKY | Unstable/Fragile stay off-manifest this watch"), FLinearColor::White, 30, 184);
		}
		else if (Voyage && Voyage->State.Phase == ECalmyVoyagePhase::Outcome)
		{
			DrawScaledText(Voyage->OutcomeText(), Voyage->State.bShiftFailed ? FLinearColor::Red : FLinearColor::Green, 30, 132);
			DrawScaledText(FString::Printf(TEXT("WATCH %d settled | Delivered %d / %d | Rescues %d | Damaged %d"),
				Voyage->State.Round, S.Delivered, S.Required, S.Rescues, S.DamagedDelivered), FLinearColor(1, .75f, .15f), 30, 158);
			DrawScaledText(TEXT("Host: press R or Esc bridge NEW WATCH for another cycle"), FLinearColor::White, 30, 184);
		}
		else
		{
			DrawScaledText(FString::Printf(TEXT("%s %d | Delivered %d / %d | Rescues: %d | Damaged: %d"),
				Voyage ? TEXT("WATCH") : TEXT("SHIFT"), S.Round, S.Delivered, S.Required, S.Rescues, S.DamagedDelivered),
				FLinearColor(1, .75f, .15f), 30, 132);
			const int32 Minutes = S.SecondsRemaining / 60;
			const int32 Seconds = S.SecondsRemaining % 60;
			const FString Economy = S.Phase == ECalmyShiftPhase::Complete ?
				FString::Printf(TEXT("CONTRACT PAID: $%d | RATING: %d/3"), S.FinalPayout, S.PerformanceStars) :
				FString::Printf(TEXT("CONTRACT TIME %02d:%02d | Base $%d + speed bonus - losses"), Minutes, Seconds, 1200);
			DrawScaledText(Economy, S.Phase == ECalmyShiftPhase::Failed ? FLinearColor::Red : FLinearColor(.4f, .85f, .9f), 30, 158);
			const TCHAR* Hint = S.Phase == ECalmyShiftPhase::Complete ? TEXT("SHIFT COMPLETE! Host: press R for a new contract.") :
				(S.Phase == ECalmyShiftPhase::Failed ? TEXT("CONTRACT FAILED: time expired. Host: press R to retry.") :
				(S.Phase == ECalmyShiftPhase::InvalidManifest ? TEXT("Contract unavailable: invalid cargo manifest.") :
				(Voyage ? TEXT("Ship watch live | Deliver STANDARD + BULKY | Repair leak with F when warned.") :
				TEXT("Bring all crates to the GREEN zone. G drop, let them settle."))));
			DrawScaledText(Hint, S.Phase == ECalmyShiftPhase::Complete ? FLinearColor::Green :
				(S.Phase == ECalmyShiftPhase::Failed ? FLinearColor::Red : FLinearColor::White), 30, 184);
		}
	}
	if (Leak)
	{
		const float BannerY = FMath::Max(240.f, Canvas->ClipY - 78.f);
		DrawRect(FLinearColor(.025f, .045f, .06f, .9f), 16, BannerY, FMath::Min(760.f, Canvas->ClipX - 32.f), 62);
		DrawScaledText(Leak->GetStatusText(), FLinearColor(1, .7f, .25f), 30, BannerY + 9);
		const TCHAR* Consequence = Leak->State.bPowerDegraded ? TEXT("EMERGENCY POWER | Red valve + blue delivery task lights remain active") :
			(Leak->GetDeckSpeedScale() < 1 ? TEXT("FLOODED DECK: movement -30% until water drops below 50%") :
			TEXT("F repair | Leave the valve to pause; teammates can resume your work"));
		DrawScaledText(Consequence, FLinearColor(.4f, .85f, 1), 30, BannerY + 35);
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
