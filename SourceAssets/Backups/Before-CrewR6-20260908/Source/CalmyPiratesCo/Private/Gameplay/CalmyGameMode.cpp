#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyBridgeTerminal.h"
#include "Camera/CameraActor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Components/InputComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

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
}

void ACalmyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::TogglePauseMenu).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::MenuConfirm).bExecuteWhenPaused = true;
    auto& Click=InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&ThisClass::MenuClick);
    Click.bExecuteWhenPaused=true;Click.bConsumeInput=false;
    InputComponent->BindKey(EKeys::Up,IE_Pressed,this,&ThisClass::MenuUp).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::Down,IE_Pressed,this,&ThisClass::MenuDown).bExecuteWhenPaused=true;
    // Menu shortcuts do not shadow the pawn's M/V actions during normal play.
    auto& CameraKey = InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ThisClass::MenuCamera);
    CameraKey.bExecuteWhenPaused = true; CameraKey.bConsumeInput = false;
    auto& MotionKey = InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ThisClass::MenuMotion);
    MotionKey.bExecuteWhenPaused = true; MotionKey.bConsumeInput = false;
    InputComponent->BindKey(EKeys::LeftBracket,IE_Pressed,this,&ThisClass::MenuLookDown).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::RightBracket,IE_Pressed,this,&ThisClass::MenuLookUp).bExecuteWhenPaused=true;
}

void ACalmyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!bCheckedStartupPause && IsLocalController() && GetPawn() && GetWorld()->HasBegunPlay())
    {
        bCheckedStartupPause = true;
        if (BridgeEntry(GetWorld()) || FParse::Param(FCommandLine::Get(), TEXT("CalmyStartPaused"))) SetPauseMenuOpen(true);
    }
    if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Hover();
}

void ACalmyPlayerController::MenuClick(){if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Click();}
void ACalmyPlayerController::MenuUp(){if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(-1);}
void ACalmyPlayerController::MenuDown(){if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(1);}
void ACalmyPlayerController::MenuConfirm(){if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Activate();else ResumeFromMenu();}

void ACalmyPlayerController::TogglePauseMenu() { SetPauseMenuOpen(!bPauseMenuOpen); }
void ACalmyPlayerController::ResumeFromMenu() { SetPauseMenuOpen(false); }
void ACalmyPlayerController::MenuCamera()
{
    if (bPauseMenuOpen) if (auto* P = Cast<ACalmyCharacter>(GetPawn())) P->ToggleSteadyCamera();
}
void ACalmyPlayerController::MenuMotion()
{
    if (bPauseMenuOpen) if (auto* P = Cast<ACalmyCharacter>(GetPawn())) P->CycleCameraMotion();
}
void ACalmyPlayerController::MenuLookDown()
{ if(bPauseMenuOpen) if(auto* P=Cast<ACalmyCharacter>(GetPawn())) P->AdjustLookSensitivity(-.25f); }
void ACalmyPlayerController::MenuLookUp()
{ if(bPauseMenuOpen) if(auto* P=Cast<ACalmyCharacter>(GetPawn())) P->AdjustLookSensitivity(.25f); }

void ACalmyPlayerController::SetPauseMenuOpen(bool bOpen)
{
    // Local UI only. Never call ServerPause: host/client menus must not freeze other players.
    if (!IsLocalController() || bOpen == bPauseMenuOpen) return;
    bPauseMenuOpen = bOpen;
    if (bOpen)
    {
        if(BridgeEntry(GetWorld()))
        {
            if(!BridgeTerminal){BridgeTerminal=GetWorld()->SpawnActor<ACalmyBridgeTerminal>();BridgeTerminal->Initialize(this);}
            PreviousMenuView=GetViewTarget();BridgeTerminal->Open();
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
	if (GetNumPlayers() >= 4) ErrorMessage = TEXT("This build supports at most four crew members.");
}

void ACalmyHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
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
        DrawText(PC->IsSoloPause() ? TEXT("PAUSED | CALMY PIRATES CO.") : TEXT("CREW MENU | ONLINE GAME CONTINUES"), FLinearColor(.4f,.9f,.9f), X+24, Y+24, GEngine->GetMediumFont());
        DrawText(PC->IsSoloPause() ? TEXT("World, physics and contract timer are paused.") : TEXT("Only your input is blocked. Cargo, water and other players stay active."), FLinearColor::White, X+24, Y+64);
        auto Button = [&](FName Name, const FString& Label, float Offset)
        {
            float MX=0, MY=0; PC->GetMousePosition(MX, MY);
            const bool Hover = MX >= X+24 && MX <= X+Width-24 && MY >= Y+Offset && MY <= Y+Offset+48;
            DrawRect(Hover ? FLinearColor(.12f,.36f,.4f) : FLinearColor(.07f,.17f,.21f), X+24, Y+Offset, Width-48, 48);
            DrawText(Label, FLinearColor::White, X+40, Y+Offset+14);
            AddHitBox(FVector2D(X+24,Y+Offset), FVector2D(Width-48,48), Name, true);
        };
        Button(TEXT("Resume"), TEXT("RESUME  [Esc / Enter]"), 106);
        if (const auto* P = Cast<ACalmyCharacter>(GetOwningPawn()))
        {
            Button(TEXT("Camera"), FString::Printf(TEXT("CAMERA: %s  [V]"), P->IsSteadyCamera()?TEXT("STEADY"):TEXT("BODYCAM")), 166);
            Button(TEXT("Motion"), FString::Printf(TEXT("CAMERA MOTION: %.0f%%  [M]"), P->GetCameraMotionScale()*100), 226);
            const float Half=(Width-56)*.5f;
            for(int32 Side=0;Side<2;++Side)
            {
                const float BX=X+24+Side*(Half+8);
                DrawRect(FLinearColor(.07f,.17f,.21f),BX,Y+286,Half,48);
                DrawText(FString::Printf(TEXT("LOOK %s | %.2fx  [%s]"),Side?TEXT("+"):TEXT("-"),P->GetLookSensitivity(),Side?TEXT("]"):TEXT("[")),FLinearColor::White,BX+12,Y+300);
                AddHitBox(FVector2D(BX,Y+286),FVector2D(Half,48),Side?TEXT("LookUp"):TEXT("LookDown"),true);
            }
        }
        DrawText(TEXT("WASD move | Space jump | Aim + E grab | G drop | LMB throw"), FLinearColor(.65f,.8f,.85f), X+24,Y+360);
        DrawText(TEXT("H helper | Q handoff | B stumble | R restart completed shift"), FLinearColor(.65f,.8f,.85f), X+24,Y+384);
        DrawText(bShipReview ? TEXT("R7 static ship review | No sailing yet | Alt+F4 to close") : TEXT("WaterLab prototype | Separate ship review available | Alt+F4 to close"), FLinearColor(.55f,.65f,.7f), X+24,Y+414);
        return;
    }
	const auto* Shift = ACalmyShift::Find(GetWorld());
	const auto* Leak = ACalmyLeak::Find(GetWorld());
	DrawRect(FLinearColor(0.025f, .045f, .06f, .82f), 16, 16, FMath::Min(620.f,Canvas->ClipX-32.f), Shift ? 210 : 122);
    DrawText(bShipReview ? TEXT("CALMY PIRATES CO. | OCEAN FREIGHTER R7") : ACalmyOcean::Find(GetWorld()) ? TEXT("CALMY PIRATES CO. | WATER LAB - NO SHIP") : (Shift ? TEXT("CALMY PIRATES CO. | COASTAL CARGO SHIFT") : TEXT("CALMY PIRATES CO. | DECK TEST")), FLinearColor(.4f,.85f,.9f),30,26,GEngine->GetMediumFont());
    DrawText(TEXT("WASD move/swim | SPACE jump | E grab | G drop | LMB throw | ESC pause"),FLinearColor::White,30,58);
	if (const auto* Pawn = Cast<ACalmyCharacter>(GetOwningPawn()))
	{
        const auto* Aimed=Pawn->FindGrabTarget();
        const auto Feedback=Pawn->GetGrabFeedback(Aimed);
        const FVector Dot=Project(Pawn->GetInteractionOrigin()+Pawn->GetBaseAimRotation().Vector()*1500.f);
        const FLinearColor DotColor=Feedback==ECalmyGrabFeedback::Ready?FLinearColor(.3f,1.f,.6f):Aimed?FLinearColor(1.f,.65f,.2f):FLinearColor::White;
        DrawRect(DotColor,Dot.X-2,Dot.Y-2,4,4);
        FString Hint=Pawn->GetInteractionHint();
        if(Pawn->HeldCargo && Pawn->HeldCargo->Kind==ECalmyCargoKind::Standard)
            Hint=TEXT("CARRYING | G put down | LMB throw");
        float HintWidth=0,HintHeight=0;GetTextSize(Hint,HintWidth,HintHeight,GEngine->GetSmallFont());
        const float HintX=FMath::Max(20.f,(Canvas->ClipX-HintWidth)*.5f),HintY=Canvas->ClipY*.5f+32;
        DrawRect(FLinearColor(.01f,.025f,.035f,.9f),HintX-12,HintY-8,HintWidth+24,HintHeight+16);
        DrawRect(DotColor,HintX-12,HintY-8,3,HintHeight+16);
        DrawText(Hint,FLinearColor::White,HintX,HintY,GEngine->GetSmallFont());
        if(const auto* Ocean=ACalmyOcean::Find(GetWorld()))
        {
            const float Strength=ACalmyOcean::SeaStrength(Ocean->WaveTime());
            DrawText(Strength>1.8f?TEXT("SEA: STRONG SWELL | current pushes swimmers and loose cargo"):
                Strength>1.f?TEXT("SEA: CHANGING SWELL | watch the waves before entering"):
                TEXT("SEA: CALM INTERVAL | stronger waves arrive in sets"),FLinearColor(.5f,.8f,.9f),30,Canvas->ClipY-80);
        }
		const FString State = StaticEnum<ECalmyPlayerLifecycleState>()->GetNameStringByValue(static_cast<int64>(Pawn->Lifecycle));
        DrawText(FString::Printf(TEXT("V %s | M motion %.0f%% | B stumble | H %s"),Pawn->IsSteadyCamera()?TEXT("bodycam"):TEXT("steady camera"),Pawn->GetCameraMotionScale()*100,
            Pawn->CrewHelper?TEXT("ask to wait"):Pawn->FindHelperTarget()?TEXT("ask Misha for help"):TEXT("approach Misha to ask for help")),FLinearColor(.65f,.85f,.9f),30,Canvas->ClipY-30);
        if(Pawn->CrewHelper)
            DrawText(FString::Printf(TEXT("CREW MATE: %s | %.1fm | keep within 3m for heavy cargo"),
                *StaticEnum<ECalmyHelperState>()->GetNameStringByValue(static_cast<int64>(Pawn->CrewHelper->Assistance)),Pawn->CrewHelper->FollowDistance/100),
                FLinearColor(.8f,.8f,.35f),30,Canvas->ClipY-55);
		DrawText(FString::Printf(TEXT("%s | Cargo: %s | %s"), *State, Pawn->HeldCargo ? TEXT("HELD") : TEXT("none"),
			GetNetMode() == NM_Client ? TEXT("CLIENT") : (GetNetMode() == NM_ListenServer ? TEXT("HOST") : TEXT("SOLO"))), FLinearColor::White, 30, 82);
		const ACalmyCargo* Target = Pawn->HeldCargo ? Pawn->HeldCargo.Get() : Pawn->FindGrabTarget();
        DrawText(Pawn->IsInOcean() ? (bShipReview ? TEXT("SURFACE SWIM | Boarding route not implemented in this review") : TEXT("SURFACE SWIM | free hands | return along the orange-railed ramp")) : (Target ? Target->GetHandlingHint(Pawn) : (bShipReview ? TEXT("Explore the ship | E cargo | Q handoff | green delivery zone") : ACalmyOcean::Find(GetWorld()) ? TEXT("E cargo | Q handoff | test buoyancy at the water ramp") : TEXT("E cargo | Q handoff | deliver to the green zone"))),FLinearColor(.65f,.8f,.85f),30,106);
	}
	if (Shift)
	{
		const auto& S = Shift->State;
		DrawText(FString::Printf(TEXT("SHIFT %d | Delivered %d / %d | Rescues: %d | Damaged: %d"), S.Round, S.Delivered, S.Required, S.Rescues, S.DamagedDelivered),
			FLinearColor(1, .75f, .15f), 30, 132);
		const int32 Minutes = S.SecondsRemaining / 60;
		const int32 Seconds = S.SecondsRemaining % 60;
		const FString Economy = S.Phase == ECalmyShiftPhase::Complete ?
			FString::Printf(TEXT("CONTRACT PAID: $%d | RATING: %d/3"), S.FinalPayout, S.PerformanceStars) :
			FString::Printf(TEXT("CONTRACT TIME %02d:%02d | Base $%d + speed bonus - losses"), Minutes, Seconds, 1200);
		DrawText(Economy, S.Phase == ECalmyShiftPhase::Failed ? FLinearColor::Red : FLinearColor(.4f, .85f, .9f), 30, 158);
		const TCHAR* Hint = S.Phase == ECalmyShiftPhase::Complete ? TEXT("SHIFT COMPLETE! Host: press R for a new contract.") :
			(S.Phase == ECalmyShiftPhase::Failed ? TEXT("CONTRACT FAILED: time expired. Host: press R to retry.") :
			(S.Phase == ECalmyShiftPhase::InvalidManifest ? TEXT("Contract unavailable: invalid cargo manifest.") :
			TEXT("Bring all crates to the GREEN zone. G drop, let them settle.")));
		DrawText(Hint, S.Phase == ECalmyShiftPhase::Complete ? FLinearColor::Green :
			(S.Phase == ECalmyShiftPhase::Failed ? FLinearColor::Red : FLinearColor::White), 30, 184);
	}
	if (Leak)
	{
		const float BannerY = FMath::Max(240.f, Canvas->ClipY - 78.f);
		DrawRect(FLinearColor(.025f, .045f, .06f, .9f), 16, BannerY, FMath::Min(760.f, Canvas->ClipX - 32.f), 62);
		DrawText(Leak->GetStatusText(), FLinearColor(1, .7f, .25f), 30, BannerY + 9);
		const TCHAR* Consequence = Leak->State.bPowerDegraded ? TEXT("EMERGENCY POWER | Red valve + blue delivery task lights remain active") :
			(Leak->GetDeckSpeedScale() < 1 ? TEXT("FLOODED DECK: movement -30% until water drops below 50%") :
			TEXT("F repair | Leave the valve to pause; teammates can resume your work"));
		DrawText(Consequence, FLinearColor(.4f, .85f, 1), 30, BannerY + 35);
	}
}

void ACalmyHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    auto* PC = Cast<ACalmyPlayerController>(GetOwningPlayerController());
    if (!PC || !PC->IsPauseMenuOpen()) return;
    if (BoxName == TEXT("Resume")) PC->ResumeFromMenu();
    else if (BoxName == TEXT("Camera")) PC->MenuCamera();
    else if (BoxName == TEXT("Motion")) PC->MenuMotion();
    else if (BoxName == TEXT("LookDown")) PC->MenuLookDown();
    else if (BoxName == TEXT("LookUp")) PC->MenuLookUp();
}
