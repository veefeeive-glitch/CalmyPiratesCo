#include "Gameplay/CalmyCharacter.h"
#include "Core/CrewImpact.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Core/AimInteraction.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/FlySwarmMob.h"
#include "GameFramework/GameStateBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Gameplay/CalmyPreferences.h"
#include "Misc/App.h"
#include "UObject/ConstructorHelpers.h"

float UCalmyCharacterMovement::GetMaxSpeed() const
{
	const auto* Pawn = Cast<ACalmyCharacter>(CharacterOwner);
	if (Pawn && !Pawn->IsControlled()) return 0.f;
    if (IsSurfaceSwimming()) return 260.f;
	const float Base = Pawn && Pawn->HeldCargo ? Pawn->HeldCargo->GetCarrySpeed() : FCalmyMovementPolicy::MaxSpeedCentimetersPerSecond;
	const auto* Leak = ACalmyLeak::Find(GetWorld());
	return Base * (Leak ? Leak->GetDeckSpeedScale() : 1.f);
}

float UCalmyCharacterMovement::GetMaxAcceleration() const
{
	const auto* Pawn = Cast<ACalmyCharacter>(CharacterOwner);
	return Pawn && !Pawn->IsControlled() ? 0.f : Super::GetMaxAcceleration();
}

void UCalmyCharacterMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    const auto* Ocean=ACalmyOcean::Find(GetWorld());
    if(!Ocean || !UpdatedComponent) return;
    const FVector P=UpdatedComponent->GetComponentLocation();
    if(Ocean->IsInsideDryHull(P))
    {
        if(IsSurfaceSwimming())SetMovementMode(MOVE_Falling);
        return;
    }
    FFindFloorResult Floor;FindFloor(P,Floor,false);
    const bool Wade=Floor.IsWalkableFloor() && Floor.FloorDist <= MaxStepHeight && Ocean->HeightAt(P)-Floor.HitResult.ImpactPoint.Z < 105.f;
    if(IsSurfaceSwimming() && Wade) SetMovementMode(MOVE_Walking);
    else if(!Wade && P.Z<Ocean->HeightAt(P)+10.f && MovementMode!=MOVE_None) SetMovementMode(MOVE_Custom,1);
}
void UCalmyCharacterMovement::PhysCustom(float DeltaSeconds,int32 Iterations)
{
    if(!IsSurfaceSwimming()) { Super::PhysCustom(DeltaSeconds,Iterations);return; }
    const auto* Ocean=ACalmyOcean::Find(GetWorld());
    if(!Ocean || !UpdatedComponent) { SetMovementMode(MOVE_Falling);return; }
    float Remaining=FMath::Min(DeltaSeconds,.25f);
    while(Remaining>MIN_TICK_TIME)
    {
        const float DT=FMath::Min(Remaining,1.f/60.f);Remaining-=DT;
        const FVector P=UpdatedComponent->GetComponentLocation();
        const FVector Wanted=Acceleration.GetClampedToMaxSize(GetMaxAcceleration())/FMath::Max(1.f,GetMaxAcceleration())*GetMaxSpeed()+Ocean->FlowAt(P);
        Velocity.X=FMath::FInterpTo(Velocity.X,Wanted.X,DT,3.f);
        Velocity.Y=FMath::FInterpTo(Velocity.Y,Wanted.Y,DT,3.f);
        const float WaterVZ=Ocean->VerticalVelocityAt(P);
        Velocity.Z=FMath::Clamp(Velocity.Z+((Ocean->HeightAt(P)-20.f-P.Z)*12.f-(Velocity.Z-WaterVZ)*6.f)*DT,-450.f,250.f);
        FHitResult Hit;SafeMoveUpdatedComponent(Velocity*DT,UpdatedComponent->GetComponentQuat(),true,Hit);
        if(Hit.IsValidBlockingHit()) SlideAlongSurface(Velocity*DT,1.f-Hit.Time,Hit.Normal,Hit,true);
    }
}
bool ACalmyCharacter::IsInOcean() const
{
    return CastChecked<UCalmyCharacterMovement>(GetCharacterMovement())->IsSurfaceSwimming();
}
void ACalmyCharacter::BeginJumpInput() { if(IsControlled() && !IsInOcean()) Jump(); }
void ACalmyCharacter::EndJumpInput() { StopJumping(); }

ACalmyCharacter::ACalmyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCalmyCharacterMovement>(CharacterMovementComponentName))
{
	bReplicates = true;
	SetReplicateMovement(true);
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0, 720, 0);
	GetCharacterMovement()->MaxWalkSpeed = FCalmyMovementPolicy::MaxSpeedCentimetersPerSecond;
	GetCharacterMovement()->bEnablePhysicsInteraction = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxBody"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body->SetStaticMesh(Cylinder.Object);
	Body->SetRelativeScale3D(FVector(.65f, .65f, 1.5f));
	Body->SetRelativeLocation(FVector(0, 0, -10));
	Body->SetOwnerNoSee(true);
	Body->SetCastHiddenShadow(true);
    Body->SetVisibility(false);Body->SetCastShadow(false);
    CrewPresentation=CreateDefaultSubobject<UCalmyCrewPresentation>(TEXT("CrewPresentation"));
	auto* Nose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	Nose->SetupAttachment(Body);
	Nose->SetStaticMesh(Cylinder.Object);
	Nose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Nose->SetRelativeLocation(FVector(60, 0, 25));
	Nose->SetRelativeScale3D(FVector(.25f));
	Nose->SetOwnerNoSee(true);
	Nose->SetCastHiddenShadow(true);
    Nose->SetVisibility(false);Nose->SetCastShadow(false);
	auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());
	Camera->SetRelativeLocation(FVector(8.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(92.f);
}

void ACalmyCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	auto* Enhanced = Cast<UEnhancedInputComponent>(Input);
	auto* PC = Cast<APlayerController>(GetController());
	if (!Enhanced || !PC || !PC->GetLocalPlayer()) return;
    if(!FApp::IsUnattended())
    {
        GConfig->GetBool(TEXT("Calmy.Accessibility"),TEXT("SteadyCamera"),bSteadyCamera,GGameUserSettingsIni);
        GConfig->GetFloat(TEXT("Calmy.Accessibility"),TEXT("CameraMotion"),CameraMotionScale,GGameUserSettingsIni);
        CameraMotionScale=FMath::IsFinite(CameraMotionScale)?FMath::Clamp(CameraMotionScale,0.f,1.f):1.f;
        GConfig->GetFloat(TEXT("Calmy.Accessibility"),TEXT("LookSensitivity"),LookSensitivity,GGameUserSettingsIni);
        LookSensitivity=FCalmyAimInteraction::Sensitivity(LookSensitivity);
    }
	InputContext = NewObject<UInputMappingContext>(this);
	InputActions.Reset();InputDefaultKeys.Reset();
	auto MakeAction = [this](FKey Key, EInputActionValueType Type)
	{
		auto* Action = NewObject<UInputAction>(this);
		Action->ValueType = Type;
		InputActions.Add(Action);
		InputDefaultKeys.Add(Key);
        InputContext->MapKey(Action, UCalmyPreferences::Get()->KeyFor(Key));
		return Action;
	};
	Enhanced->BindAction(MakeAction(EKeys::W, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::Forward);
	Enhanced->BindAction(MakeAction(EKeys::S, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::Backward);
	Enhanced->BindAction(MakeAction(EKeys::D, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::Right);
	Enhanced->BindAction(MakeAction(EKeys::A, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::Left);
	Enhanced->BindAction(MakeAction(EKeys::MouseX, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::LookYaw);
	Enhanced->BindAction(MakeAction(EKeys::MouseY, EInputActionValueType::Axis1D), ETriggerEvent::Triggered, this, &ThisClass::LookPitch);
	Enhanced->BindAction(MakeAction(EKeys::E, EInputActionValueType::Boolean), ETriggerEvent::Started, this, &ThisClass::RequestGrab);
	Enhanced->BindAction(MakeAction(EKeys::G, EInputActionValueType::Boolean), ETriggerEvent::Started, this, &ThisClass::RequestDrop);
	Enhanced->BindAction(MakeAction(EKeys::LeftMouseButton, EInputActionValueType::Boolean), ETriggerEvent::Started, this, &ThisClass::RequestThrow);
	Enhanced->BindAction(MakeAction(EKeys::R, EInputActionValueType::Boolean), ETriggerEvent::Started, this, &ThisClass::RequestRestart);
	Enhanced->BindAction(MakeAction(EKeys::Q, EInputActionValueType::Boolean), ETriggerEvent::Started, this, &ThisClass::RequestHandoff);
    Enhanced->BindAction(MakeAction(EKeys::B,EInputActionValueType::Boolean),ETriggerEvent::Started,this,&ThisClass::RequestStumble);
    Enhanced->BindAction(MakeAction(EKeys::V,EInputActionValueType::Boolean),ETriggerEvent::Started,this,&ThisClass::ToggleSteadyCamera);
    Enhanced->BindAction(MakeAction(EKeys::H,EInputActionValueType::Boolean),ETriggerEvent::Started,this,&ThisClass::RequestHelper);
    Enhanced->BindAction(MakeAction(EKeys::M,EInputActionValueType::Boolean),ETriggerEvent::Started,this,&ThisClass::CycleCameraMotion);
    auto* JumpAction=MakeAction(EKeys::SpaceBar,EInputActionValueType::Boolean);
    Enhanced->BindAction(JumpAction,ETriggerEvent::Started,this,&ThisClass::BeginJumpInput);
    Enhanced->BindAction(JumpAction,ETriggerEvent::Completed,this,&ThisClass::EndJumpInput);
	auto* RepairAction = MakeAction(EKeys::F, EInputActionValueType::Boolean);
	Enhanced->BindAction(RepairAction, ETriggerEvent::Started, this, &ThisClass::BeginRepairInput);
	Enhanced->BindAction(RepairAction, ETriggerEvent::Completed, this, &ThisClass::EndRepairInput);
	Enhanced->BindAction(RepairAction, ETriggerEvent::Canceled, this, &ThisClass::EndRepairInput);
	PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->AddMappingContext(InputContext, 0);
}

void ACalmyCharacter::Forward(const FInputActionValue& V) { MoveRelativeIntent(FVector2D(V.Get<float>(), 0)); }
void ACalmyCharacter::Backward(const FInputActionValue& V) { MoveRelativeIntent(FVector2D(-V.Get<float>(), 0)); }
void ACalmyCharacter::Right(const FInputActionValue& V) { MoveRelativeIntent(FVector2D(0, V.Get<float>())); }
void ACalmyCharacter::Left(const FInputActionValue& V) { MoveRelativeIntent(FVector2D(0, -V.Get<float>())); }

void ACalmyCharacter::MoveIntent(const FVector2D& Intent)
{
	if (!IsControlled() || Intent.ContainsNaN()) return;
	const FVector2D Safe = Intent.GetClampedToMaxSize(1);
	AddMovementInput(FVector::ForwardVector, Safe.X);
	AddMovementInput(FVector::RightVector, Safe.Y);
}

void ACalmyCharacter::MoveRelativeIntent(const FVector2D& Intent)
{
	if (Intent.ContainsNaN()) return;
	const FVector2D Safe = Intent.GetClampedToMaxSize(1);
	const FRotator ControlYaw(0.f, Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw, 0.f);
	const FVector WorldIntent = FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::X) * Safe.X
		+ FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::Y) * Safe.Y;
	MoveIntent(FVector2D(WorldIntent.X, WorldIntent.Y));
}

void ACalmyCharacter::LookYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>()*LookSensitivity); }
void ACalmyCharacter::LookPitch(const FInputActionValue& Value) { AddControllerPitchInput(Value.Get<float>()*LookSensitivity*(UCalmyPreferences::Get()->InvertY?1.f:-1.f)); }

FVector ACalmyCharacter::GetInteractionOrigin() const
{
    // Stable chest ray; camera shake never changes authoritative targeting.
    return GetActorLocation()+FRotator(0,GetBaseAimRotation().Yaw,0).RotateVector(FVector(20,0,36));
}

bool ACalmyCharacter::CanGrab(const ACalmyCargo* Cargo) const
{
	if (!IsValid(Cargo) || Cargo->Carry.bDelivered || Cargo->GetWorld() != GetWorld() || !IsControlled() || HeldCargo || IsInOcean()) return false;
    if(!Cargo->Mesh || !Cargo->Mesh->GetStaticMesh()) return false;
    const FTransform T=Cargo->Mesh->GetComponentTransform();
    const FVector Closest=T.TransformPosition(Cargo->Mesh->GetStaticMesh()->GetBoundingBox().GetClosestPointTo(T.InverseTransformPosition(GetInteractionOrigin())));
    const float Range=FMath::IsFinite(GrabRangeCm)?FMath::Clamp(GrabRangeCm,100.f,250.f):220.f;
    if(FVector::Distance(GetInteractionOrigin(),Closest)>Range) return false;
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CalmyGrab), false, this);
	const bool Blocked = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Cargo->GetActorLocation(), ECC_Visibility, Query);
	return !Blocked || Hit.GetActor() == Cargo;
}

ACalmyCargo* ACalmyCharacter::FindGrabTarget() const
{
	ACalmyCargo* Best = nullptr;
    float Distance=500.f;
    const FVector Origin=GetInteractionOrigin(),Direction=GetBaseAimRotation().Vector();
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
	{
        if(!It->Mesh || !It->Mesh->GetStaticMesh()) continue;
        float CandidateDistance=0;
        // Include occupied/delivered proxies even though their physics is disabled, for honest refusal hints.
        if(FCalmyAimInteraction::HitBox(Origin,Direction,It->Mesh->GetComponentTransform(),
            It->Mesh->GetStaticMesh()->GetBoundingBox(),Distance,CandidateDistance))
		{
			Distance = CandidateDistance;
			Best = *It;
		}
	}
	return Best;
}

ECalmyGrabFeedback ACalmyCharacter::GetGrabFeedback(const ACalmyCargo* Cargo) const
{
    if(!IsControlled()) return ECalmyGrabFeedback::Recovering;
    if(IsInOcean()) return ECalmyGrabFeedback::Swimming;
    if(HeldCargo) return ECalmyGrabFeedback::HandsFull;
    if(!IsValid(Cargo) || Cargo->GetWorld()!=GetWorld() || !Cargo->Mesh || !Cargo->Mesh->GetStaticMesh() || FindGrabTarget()!=Cargo) return ECalmyGrabFeedback::NoTarget;
    float Distance=0;
    const FVector Origin=GetInteractionOrigin(),Direction=GetBaseAimRotation().Vector();
    if(!FCalmyAimInteraction::HitBox(Origin,Direction,Cargo->Mesh->GetComponentTransform(),Cargo->Mesh->GetStaticMesh()->GetBoundingBox(),500.f,Distance)) return ECalmyGrabFeedback::NoTarget;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(GrabAimVisibility),false,this);Q.AddIgnoredActor(Cargo);
    if(GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*Distance,ECC_Visibility,Q)) return ECalmyGrabFeedback::Obstructed;
    const float Range=FMath::IsFinite(GrabRangeCm)?FMath::Clamp(GrabRangeCm,100.f,250.f):220.f;
    if(Distance>Range) return ECalmyGrabFeedback::TooFar;
    if(Cargo->Carry.bDelivered) return ECalmyGrabFeedback::Delivered;
    if(Cargo->Carry.State!=ECalmyCarryState::Free) return ECalmyGrabFeedback::Occupied;
    if(!CanGrab(Cargo)) return ECalmyGrabFeedback::Obstructed;
    if(Cargo->Kind==ECalmyCargoKind::Bulky && !Cargo->HasNearbyHelper(this)) return ECalmyGrabFeedback::NeedHelper;
    if(!Cargo->CanLift(this)) return ECalmyGrabFeedback::Unstable;
    return ECalmyGrabFeedback::Ready;
}
FString ACalmyCharacter::GrabFeedbackText(ECalmyGrabFeedback Reason)
{
    switch(Reason)
    {
    case ECalmyGrabFeedback::Ready:return TEXT("E - GRAB");
    case ECalmyGrabFeedback::TooFar:return TEXT("TOO FAR - move closer to the cargo");
    case ECalmyGrabFeedback::Obstructed:return TEXT("BLOCKED - reach it from this side of the obstacle");
    case ECalmyGrabFeedback::Occupied:return TEXT("BUSY - another holder or cargo still moving after a throw");
    case ECalmyGrabFeedback::Delivered:return TEXT("DELIVERED - this cargo is already accepted");
    case ECalmyGrabFeedback::HandsFull:return TEXT("HANDS FULL - G to put down; Q to hand off unstable cargo");
    case ECalmyGrabFeedback::Recovering:return TEXT("RECOVERING - wait until you regain balance");
    case ECalmyGrabFeedback::Swimming:return TEXT("SWIMMING - return to the ramp before lifting cargo");
    case ECalmyGrabFeedback::NeedHelper:return TEXT("TOO HEAVY ALONE - bring a free teammate within 3m, or ask Misha");
    case ECalmyGrabFeedback::Unstable:return TEXT("UNSTABLE - let the cargo rest on deck before lifting");
    case ECalmyGrabFeedback::Retry:return TEXT("TARGET CHANGED - aim again");
    default:return TEXT("Aim at a cargo surface and approach to pick it up");
    }
}
FString ACalmyCharacter::GetInteractionHint() const
{
    if(const auto* Door=ACalmyDoor::FindTarget(this))return Door->State==ECalmyDoorState::Blocked?TEXT("E - REVERSE DOOR | clear the swing area"):Door->State==ECalmyDoorState::Opening||Door->State==ECalmyDoorState::Closing?TEXT("DOOR MOVING"):Door->IsOpen()?TEXT("E - CLOSE DOOR"):TEXT("E - OPEN DOOR");
    if(FeedbackUntil>GetWorld()->GetTimeSeconds()) return GrabFeedbackText(LastGrabFeedback);
    if(HeldCargo) return HeldCargo->GetHandlingHint(this);
    if (const auto* PC=Cast<ACalmyPlayerController>(GetController()))
        if (AFlySwarmMob::FindAimedSwarm(PC)) return TEXT("E - INTERACT WITH SWARM | LMB - SWAT");
    if (const auto* Misha=Cast<ACalmyCrewHelper>(FindMeleeTarget()))
        return Misha->Reaction.IsEmpty()?TEXT("MISHA | H - ASK FOR HELP | LMB - PUNCH"):Misha->Reaction;
    return GrabFeedbackText(GetGrabFeedback(FindGrabTarget()));
}
void ACalmyCharacter::ClientGrabFeedback_Implementation(ECalmyGrabFeedback Reason)
{
    LastGrabFeedback=Reason;FeedbackUntil=Reason==ECalmyGrabFeedback::Ready?0:GetWorld()->GetTimeSeconds()+1.5f;
}

void ACalmyCharacter::RequestGrab()
{
    auto* PC = Cast<ACalmyPlayerController>(GetController());
    if (PC && PC->IsGameplayInputSuppressed()) return;
    if(auto* Door=ACalmyDoor::FindTarget(this)){ServerDoor(Door,Door->Revision,++LastIssuedSequence);return;}
    if (PC && PC->TryOpenFlyWheel()) return;
    ServerGrab(FindGrabTarget(), ++LastIssuedSequence);
}
void ACalmyCharacter::ServerDoor_Implementation(ACalmyDoor* Door,uint32 ExpectedRevision,uint32 Sequence)
{
    if(!CommandGate.Accept(Sequence,GetWorld()->GetTimeSeconds())||!IsValid(Door)||Door->GetWorld()!=GetWorld()||!Door->RequestToggle(this,ExpectedRevision))++RejectedCommands;
}
void ACalmyCharacter::RequestDrop() { ServerDrop(++LastIssuedSequence); }
void ACalmyCharacter::RequestThrow()
{
    if (const auto* PC = Cast<ACalmyPlayerController>(GetController())) if (PC->IsGameplayInputSuppressed()) return;
    if (HeldCargo) ServerThrow(++LastIssuedSequence); else RequestPunch();
}
void ACalmyCharacter::RequestPunch() { ServerPunch(++LastIssuedSequence); }
void ACalmyCharacter::ServerPunch_Implementation(uint32 Sequence)
{
    if (!CommandGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !PerformPunch()) ++RejectedCommands;
}
AActor* ACalmyCharacter::FindMeleeTarget() const
{
    const FVector Origin = GetInteractionOrigin(), Direction = GetBaseAimRotation().Vector();
    float Distance = 145.f;
    FHitResult Block; FCollisionQueryParams Q(SCENE_QUERY_STAT(MeleeVisibility), true, this);
    if (GetWorld()->LineTraceSingleByChannel(Block, Origin, Origin + Direction * Distance, ECC_Visibility, Q)) Distance = Block.Distance;
    AActor* Best = nullptr;
    for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
    {
        if (*It == this || !It->IsControlled()) continue;
        float HitDistance;
        if (FCalmyAimInteraction::HitBox(Origin, Direction, It->GetActorTransform(), FBox(FVector(-40,-40,-80),FVector(40,40,80)), Distance, HitDistance))
        { Distance = HitDistance; Best = *It; }
    }
    for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It)
    {
        float HitDistance;
        if (It->AimDistance(Origin, Direction, Distance, 30.f, HitDistance)) { Distance = HitDistance; Best = *It; }
    }
    return Best;
}
bool ACalmyCharacter::PerformPunch()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (!HasAuthority() || !IsControlled() || Health <= 0.f || HeldCargo || IsInOcean() || Now < NextPunchAt) return false;
    NextPunchAt = Now + (Cast<ACalmyCrewHelper>(this) ? 1.25f : .65f);
    PunchStartedAt = Now; ++PunchCount;
    if (auto* Target = FindMeleeTarget())
    {
        if (auto* Swarm = Cast<AFlySwarmMob>(Target)) Swarm->ReceiveSwat(this);
        else if (auto* Crew = Cast<ACalmyCharacter>(Target)) Crew->ReceiveMeleeHit(this);
    }
    ForceNetUpdate(); return true;
}
bool ACalmyCharacter::ReceiveMeleeHit(ACalmyCharacter* Attacker)
{
    if (!HasAuthority() || !IsControlled() || !IsValid(Attacker) || Attacker == this || Attacker->GetWorld() != GetWorld()) return false;
    Health = FMath::Max(0.f, Health - (Cast<ACalmyCrewHelper>(Attacker) ? 12.f : 6.f));
    ++MeleeHitsReceived; LastMeleeHitAt = GetWorld()->GetTimeSeconds();
    if (Health <= 0.f) BeginKnockdown(600.f, GetActorLocation()-Attacker->GetActorLocation());
    ForceNetUpdate(); return true;
}
float ACalmyCharacter::GetPunchBlend() const
{
    if (!IsControlled() || HeldCargo) return 0.f;
    const auto* GS = GetWorld()->GetGameState();
    const float Age = (GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) - PunchStartedAt;
    return Age >= 0.f && Age < .42f ? FMath::Sin(Age / .42f * PI) : 0.f;
}
float ACalmyCharacter::GetHitFlash() const
{
    const auto* GS = GetWorld()->GetGameState();
    const float Age = (GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) - LastMeleeHitAt;
    return Age >= 0.f ? FMath::Clamp(1.f-Age/.45f,0.f,1.f) : 0.f;
}

void ACalmyCharacter::BeginRepairInput() { bRepairInput = true; RepairPulseCountdown = 0; }
void ACalmyCharacter::EndRepairInput()
{
	bRepairInput = false;
	if (auto* Leak = ACalmyLeak::Find(GetWorld())) ServerRepair(Leak, Leak->State.Round, false, ++LastRepairIssued);
}

void ACalmyCharacter::ServerRepair_Implementation(ACalmyLeak* Leak, int32 ExpectedRound, bool bActive, uint32 Sequence)
{
	if (!Sequence || Sequence <= LastRepairReceived) { ++RejectedCommands; return; }
	LastRepairReceived = Sequence;
	if (!IsValid(Leak) || Leak->GetWorld() != GetWorld() || Leak->State.Round != ExpectedRound) { ++RejectedCommands; return; }
	if (!bActive) { Leak->StopRepair(this); return; }
	if (!RepairGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !Leak->TryRepairPulse(this, ExpectedRound)) ++RejectedCommands;
}

void ACalmyCharacter::RequestHandoff()
{
	ACalmyCharacter* Receiver = nullptr;
	double BestDistance = FMath::Square(250.f);
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (*It != this && !Cast<ACalmyCrewHelper>(*It) && It->IsControlled() && !It->HeldCargo && Distance <= BestDistance) { Receiver = *It; BestDistance = Distance; }
	}
	ServerHandoff(Receiver, ++LastIssuedSequence);
}

void ACalmyCharacter::ServerHandoff_Implementation(ACalmyCharacter* Receiver, uint32 Sequence)
{
	if (!CommandGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !IsControlled() || !HeldCargo || !HeldCargo->TryHandoff(this, Receiver)) ++RejectedCommands;
}

void ACalmyCharacter::RequestRestart()
{
	if (auto* Shift = ACalmyShift::Find(GetWorld())) ServerRestart(Shift->State.Round, ++LastIssuedSequence);
}

void ACalmyCharacter::ServerRestart_Implementation(int32 ExpectedRound, uint32 Sequence)
{
	auto* Shift = ACalmyShift::Find(GetWorld());
	if (!CommandGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !Shift || !Shift->TryRestart(this, ExpectedRound)) ++RejectedCommands;
}

void ACalmyCharacter::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
}

void ACalmyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
    if(IsLocallyControlled() && DeltaSeconds>SMALL_NUMBER)
    {
        const FVector Local=GetActorQuat().UnrotateVector(GetVelocity());
        const FVector Accel=GetActorQuat().UnrotateVector((GetVelocity()-CameraPreviousVelocity)/FMath::Max(DeltaSeconds,.008f));
        CameraPreviousVelocity=GetVelocity();
        const float Speed=FMath::Clamp(GetVelocity().Size2D()/450.f,0.f,1.f);
        const bool Grounded=GetCharacterMovement()->IsMovingOnGround();
        CameraPhase=FMath::Fmod(CameraPhase+FMath::Min(DeltaSeconds,.1f)*GetVelocity().Size2D()*(2*PI/200.f),2*PI);
        const float Step=Grounded?FMath::Cos(CameraPhase*2)*Speed*.8f:0.f;
        const float Load=HeldCargo?1.25f:1.f;
        BodycamMotion.Step(FVector(FMath::Clamp(-Accel.X*.0015,-2.5,2.5),FMath::Clamp(-Local.Y*.005,-2.5,2.5),Step*Load),DeltaSeconds);
        CameraFall=FMath::FInterpTo(CameraFall,Lifecycle==ECalmyPlayerLifecycleState::KnockedDown?1.f:0.f,DeltaSeconds,6.f);
    }
	if (IsLocallyControlled() && bRepairInput)
	{
		RepairPulseCountdown -= DeltaSeconds;
		if (RepairPulseCountdown <= 0)
		{
			RepairPulseCountdown = .2f;
			if (auto* Leak = ACalmyLeak::Find(GetWorld())) ServerRepair(Leak, Leak->State.Round, true, ++LastRepairIssued);
		}
	}
    if(HasAuthority())
    {
        const auto* Ocean=ACalmyOcean::Find(GetWorld());
        if(Ocean && IsInOcean() && HeldCargo) ReleaseCargo();
        if(Ocean ? Ocean->IsOutsideSafetyArea(GetActorLocation()) : FCalmyShipBoundsPolicy::IsPlayerOverboard(GetActorLocation())) ReturnToDeck();
    }
}

bool ACalmyCharacter::ReturnToDeck(bool bIncident)
{
	if (!HasAuthority()) return false;
	TArray<FVector> Candidates{HomeLocation};
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) Candidates.Add(It->GetActorLocation());
	// Deterministic fallback slots if somebody is standing on every player start.
	for (int32 X = -600; X <= 400; X += 200) for (int32 Y = -300; Y <= 300; Y += 150) Candidates.Add(FVector(X, Y, 100));
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CalmyReturn), false, this);
	if (HeldCargo) Query.AddIgnoredActor(HeldCargo);
	bool bReturned = false;
	for (FVector Position : Candidates)
	{
		FHitResult Floor;
		if (!GetWorld()->LineTraceSingleByChannel(Floor, Position + FVector(0, 0, 200), Position - FVector(0, 0, 250), ECC_WorldStatic, Query) ||
			Floor.ImpactNormal.Z < .8f || (!ACalmyOcean::Find(GetWorld()) && Floor.ImpactPoint.Z > 30.f)) continue;
		Position.Z = Floor.ImpactPoint.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 4.f;
		if (GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Query)) continue;
		if (TeleportTo(Position, HomeRotation, false, false)) { bReturned = true; break; }
	}
	if (!bReturned) return false; // Retry next tick; never teleport into occupied geometry.
    CrewPresentation->ResetAfterRescue();
    Health=100.f;
	if (IsValid(HeldCargo)) HeldCargo->ResetHome(bIncident);
	HeldCargo = nullptr;
	GetWorldTimerManager().ClearTimer(RecoveryTimer);
	if (Lifecycle == ECalmyPlayerLifecycleState::KnockedDown) SetLifecycle(ECalmyPlayerLifecycleState::Recovering);
	if (Lifecycle == ECalmyPlayerLifecycleState::Recovering) FinishRecovery();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ConsumeMovementInputVector();
	if (bIncident)
	{
		++ResetCount;
		if (auto* Shift = ACalmyShift::Find(GetWorld())) Shift->RecordRescue();
	}
	ForceNetUpdate();
	return true;
}

void ACalmyCharacter::ServerGrab_Implementation(ACalmyCargo* Cargo, uint32 Sequence)
{
    if(!CommandGate.Accept(Sequence,GetWorld()->GetTimeSeconds())) {++RejectedCommands;return;}
    const ECalmyGrabFeedback Feedback=GetGrabFeedback(Cargo);
    if(Feedback!=ECalmyGrabFeedback::Ready)
	{ ++RejectedCommands;ClientGrabFeedback(Feedback);return; }
    if (!Cargo->TryGrab(this))
	{
		++RejectedCommands;
		ClientGrabFeedback(ECalmyGrabFeedback::Retry);
		return;
	}
	HeldCargo = Cargo;
    ClientGrabFeedback(ECalmyGrabFeedback::Ready);
	ForceNetUpdate();
}

void ACalmyCharacter::ServerDrop_Implementation(uint32 Sequence)
{
	if (!CommandGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !IsControlled() || !HeldCargo)
	{
		++RejectedCommands;
		return;
	}
	ReleaseCargo();
}

void ACalmyCharacter::ServerThrow_Implementation(uint32 Sequence)
{
	if (!CommandGate.Accept(Sequence, GetWorld()->GetTimeSeconds()) || !IsControlled() || !HeldCargo || !HeldCargo->Throw(this))
	{
		++RejectedCommands;
		return;
	}
	HeldCargo = nullptr;
	ForceNetUpdate();
}

FVector ACalmyCharacter::GetCarryLocation() const
{
	const FVector Start = GetActorLocation();
    // Carry below the chest camera, with room to see the route over the object.
    const float CarryDistance=IsValid(HeldCargo) && HeldCargo->Kind==ECalmyCargoKind::Bulky?92.f:78.f;
	const FVector End = Start + GetActorForwardVector() * CarryDistance + FVector(0, 0, -8);
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CalmyCarry), false, this);
	if (HeldCargo) Query.AddIgnoredActor(HeldCargo);
	// Sweep the real oriented crate, not an undersized sphere that misses corners.
	// Held cargo remains non-pushing; server and clients derive the same pose.
	const FVector Half = IsValid(HeldCargo) && HeldCargo->Mesh && HeldCargo->Mesh->GetStaticMesh()
        ? HeldCargo->Mesh->GetStaticMesh()->GetBoundingBox().GetExtent()*HeldCargo->GetActorScale3D().GetAbs()+FVector(1)
        : FVector(38);
    FVector SweepStart=Start;
    for(int32 Attempt=0;Attempt<4;++Attempt)
    {
        const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,SweepStart,End,GetActorQuat(),ECC_WorldStatic,FCollisionShape::MakeBox(Half),Query);
        if(!Blocked || !Hit.bStartPenetrating)
        {
            const FVector Candidate=Blocked?Hit.Location:End;
            FHitResult Sight;
            // A correction must not move the held object through a thin wall.
            if(!GetWorld()->LineTraceSingleByChannel(Sight,Start,Candidate,ECC_WorldStatic,Query)) return Candidate;
            break;
        }
        // Capsule can fit nearer a wall than a wide crate. Resolve the query's
        // initial overlap by a bounded MTD, never shrink cargo or disable walls.
        SweepStart+=Hit.Normal*(Hit.PenetrationDepth+2.f);
        if(FVector::DistSquared(SweepStart,Start)>FMath::Square(60.f)) break;
    }
    return Start; // Unresolvable spaces remain visible to the clearance audit.
}

void ACalmyCharacter::ReleaseCargo()
{
	if (!HasAuthority()) return;
	if (IsValid(HeldCargo)) HeldCargo->Drop(this);
	HeldCargo = nullptr;
	ForceNetUpdate();
}

bool ACalmyCharacter::ReceiveCargoImpact(float Speed,FVector Direction)
{
	if (!HasAuthority() || !IsControlled() || !FMath::IsFinite(Speed) || Speed < 500.f) return false;
    Health=FMath::Max(0.f,Health-FCalmyCrewImpact::Damage(Speed,false));
    return BeginKnockdown(Speed,Direction);
}

void ACalmyCharacter::ApplyLandingImpact(float Speed)
{
    if(!HasAuthority() || IsInOcean() || !FMath::IsFinite(Speed)) return;
    Health=FMath::Max(0.f,Health-FCalmyCrewImpact::Damage(Speed,true));
    ForceNetUpdate();
    // Landing while already down still hurts, but does not restart the state
    // machine or double-charge the cargo-impact damage formula.
    if(IsControlled() && (Speed>900.f || Health<=0.f)) BeginKnockdown(Speed,FVector::ZeroVector);
}

bool ACalmyCharacter::BeginKnockdown(float Speed,FVector Direction)
{
	ReleaseCargo();
	++KnockdownCount;
    const FVector Impulse=Direction.ContainsNaN()?FVector::ZeroVector:Direction.GetSafeNormal2D()*FMath::Clamp(Speed*.22f,100.f,280.f);
    CrewPresentation->SetFallVelocity(GetVelocity()+Impulse);
	SetLifecycle(ECalmyPlayerLifecycleState::KnockedDown);
	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &ThisClass::BeginRecovery, FMath::Clamp(Speed/650.f,1.2f,2.8f), false);
	return true;
}

void ACalmyCharacter::SetLifecycle(ECalmyPlayerLifecycleState Next)
{
	if (!HasAuthority() || !FCalmyPlayerLifecycleTransitions::IsValidTransition(Lifecycle, Next)) return;
	Lifecycle = Next;
	OnRep_Lifecycle();
	ForceNetUpdate();
}

void ACalmyCharacter::BeginRecovery()
{
    if(!HasAuthority()) return;
    // No timed stand-up in mid-air. At zero health the existing safe rescue
    // flow restores the pawn; this is not a death/revive system yet.
    if(Health<=0.f)
    {
        if(!ReturnToDeck()) GetWorldTimerManager().SetTimer(RecoveryTimer,this,&ThisClass::BeginRecovery,.25f,false);
        return;
    }
    if(CrewPresentation->IsPhysicalFall()? !CrewPresentation->PrepareRecovery():
        (!GetCharacterMovement()->IsMovingOnGround() && !IsInOcean()))
    { GetWorldTimerManager().SetTimer(RecoveryTimer,this,&ThisClass::BeginRecovery,.2f,false);return; }
	SetLifecycle(ECalmyPlayerLifecycleState::Recovering);
	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &ThisClass::FinishRecovery, .7f, false);
}

void ACalmyCharacter::FinishRecovery()
{
    if(!HasAuthority() || Lifecycle!=ECalmyPlayerLifecycleState::Recovering) return;
    if(!CrewPresentation->CanFinishRecovery())
    { GetWorldTimerManager().SetTimer(RecoveryTimer,this,&ThisClass::FinishRecovery,.1f,false);return; }
	++RecoveryCount;
	SetLifecycle(ECalmyPlayerLifecycleState::Controlled);
}

void ACalmyCharacter::OnRep_Lifecycle()
{
	if (IsControlled()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	else { GetCharacterMovement()->StopMovementImmediately(); if(!IsInOcean()) GetCharacterMovement()->SetMovementMode(MOVE_Falling); }
	Body->SetRelativeRotation(FRotator(Lifecycle == ECalmyPlayerLifecycleState::KnockedDown ? 80.f :
		(Lifecycle == ECalmyPlayerLifecycleState::Recovering ? 35.f : 0.f), 0, 0));
}

void ACalmyCharacter::RequestStumble() { ServerStumble(++LastIssuedSequence); }
void ACalmyCharacter::ServerStumble_Implementation(uint32 Sequence)
{
    if(!CommandGate.Accept(Sequence,GetWorld()->GetTimeSeconds()) || !IsControlled() || IsInOcean() ||
        !GetCharacterMovement()->IsMovingOnGround() || GetWorld()->GetTimeSeconds()<NextStumbleTime)
    { ++RejectedCommands;return; }
    NextStumbleTime=GetWorld()->GetTimeSeconds()+5.f;
    ReceiveCargoImpact(650.f,GetActorForwardVector());
}
void ACalmyCharacter::SaveCameraPreferences()
{
    if(!IsLocallyControlled() || FApp::IsUnattended()) return;
    auto* Preferences=UCalmyPreferences::Get();Preferences->SteadyCamera=bSteadyCamera;Preferences->CameraMotion=CameraMotionScale;Preferences->Sensitivity=LookSensitivity;Preferences->Persist();
    GConfig->SetBool(TEXT("Calmy.Accessibility"),TEXT("SteadyCamera"),bSteadyCamera,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Calmy.Accessibility"),TEXT("CameraMotion"),CameraMotionScale,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Calmy.Accessibility"),TEXT("LookSensitivity"),LookSensitivity,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void ACalmyCharacter::ApplyLocalPreferences()
{
    if(!IsLocallyControlled())return;
    const auto* Preferences=UCalmyPreferences::Get();bSteadyCamera=Preferences->SteadyCamera;CameraMotionScale=Preferences->CameraMotion;LookSensitivity=Preferences->Sensitivity;
    BodycamMotion=FCalmyBodycamSpring();
    if(InputContext && InputActions.Num()==InputDefaultKeys.Num())
    {
        InputContext->UnmapAll();
        for(int32 I=0;I<InputActions.Num();++I)InputContext->MapKey(InputActions[I],Preferences->KeyFor(InputDefaultKeys[I]));
        if(auto* PC=Cast<APlayerController>(GetController()))if(PC->GetLocalPlayer())
            PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->RequestRebuildControlMappings();
    }
}
void ACalmyCharacter::ToggleSteadyCamera() { bSteadyCamera=!bSteadyCamera;BodycamMotion=FCalmyBodycamSpring();SaveCameraPreferences(); }
void ACalmyCharacter::AdjustLookSensitivity(float Delta)
{ if(FMath::IsFinite(Delta)) {LookSensitivity=FCalmyAimInteraction::Sensitivity(LookSensitivity+Delta);SaveCameraPreferences();} }
void ACalmyCharacter::CycleCameraMotion()
{
    CameraMotionScale=CameraMotionScale<.4f?.65f:(CameraMotionScale<.9f?1.f:.3f);SaveCameraPreferences();
}
void ACalmyCharacter::RequestHelper() { ServerHelper(++LastIssuedSequence); }
ACalmyCrewHelper* ACalmyCharacter::FindHelperTarget() const
{
    ACalmyCrewHelper* Best=nullptr;float Distance=220.f;
    for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It)
    {
        const float D=FVector::Distance(GetActorLocation(),It->GetActorLocation());
        if(IsValid(It->Leader) || It->IsDefending() || !It->IsControlled() || It->IsInOcean() || D>Distance) continue;
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CrewRecruitSight),false,this);Q.AddIgnoredActor(*It);
        if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),It->GetActorLocation(),ECC_Visibility,Q)) continue;
        Best=*It;Distance=D;
    }
    return Best;
}
void ACalmyCharacter::ServerHelper_Implementation(uint32 Sequence)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(!CommandGate.Accept(Sequence,Now) || Cast<ACalmyCrewHelper>(this) || !Cast<APlayerController>(GetController()) ||
        !IsControlled() || IsInOcean() || !GetCharacterMovement()->IsMovingOnGround() || Now<NextHelperCommand)
    {++RejectedCommands;return;}
    NextHelperCommand=Now+2.f;
    if(IsValid(CrewHelper)) {CrewHelper->ReleaseLeader();return;}
    if(auto* Helper=FindHelperTarget())
    { Helper->AssignLeader(this);CrewHelper=Helper;ForceNetUpdate();return; }
    ++RejectedCommands;
}
void ACalmyCharacter::Landed(const FHitResult& Hit)
{
    const float Speed=FMath::Max(0.f,-GetVelocity().Z);
    Super::Landed(Hit);
    if(IsLocallyControlled()) BodycamMotion.Velocity.Z-=FMath::Clamp(Speed*.05f,0.f,35.f);
    ApplyLandingImpact(Speed);
}
void ACalmyCharacter::CalcCamera(float DT,FMinimalViewInfo& Out)
{
    Super::CalcCamera(DT,Out);
    const float Strength=bSteadyCamera?0.f:CameraMotionScale;
    const FVector Motion=BodycamMotion.Position*Strength;
    const FVector Offset(20.f,0.f,36.f+Motion.Z-CameraFall*(bSteadyCamera?12.f:40.f));
    const bool Physical=CrewPresentation && CrewPresentation->IsPhysicalFall();
    const FTransform Chest=Physical?CrewPresentation->GetChestTransform():GetActorTransform();
    const FVector Start=Physical?Chest.GetLocation():GetActorLocation()+FVector(0,0,Offset.Z);
    const FVector Desired=Physical?Chest.TransformPosition(FVector(20,0,32)):GetActorTransform().TransformPosition(Offset);
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(BodycamWall),false,this);
    if(HeldCargo) Query.AddIgnoredActor(HeldCargo);
    Out.Location=GetWorld()->SweepSingleByChannel(Hit,Start,Desired,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(7),Query)?Hit.Location:Desired;
    Out.Rotation=GetControlRotation()+FRotator(Motion.X+CameraFall*7.f*Strength,0,Motion.Y+CameraFall*18.f*Strength);
    if(Physical && !bSteadyCamera)
    {
        const FRotator Relative=(GetActorQuat().Inverse()*Chest.GetRotation()).Rotator();
        Out.Rotation+=FRotator(FMath::Clamp(Relative.Pitch,-65.f,65.f)*Strength,0,FMath::Clamp(Relative.Roll,-65.f,65.f)*Strength);
    }
    Out.FOV=UCalmyPreferences::Get()->FieldOfView;
    Out.PostProcessBlendWeight=1;
    Out.PostProcessSettings.bOverride_VignetteIntensity=true;Out.PostProcessSettings.VignetteIntensity=bSteadyCamera?.2f:.42f;
    Out.PostProcessSettings.bOverride_SceneFringeIntensity=true;Out.PostProcessSettings.SceneFringeIntensity=bSteadyCamera?0.f:.3f;
    Out.PostProcessSettings.bOverride_MotionBlurAmount=true;Out.PostProcessSettings.MotionBlurAmount=0.f;
}

void ACalmyCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if(HasAuthority() && IsValid(CrewHelper)) CrewHelper->ReleaseLeader();
	if (auto* Leak = ACalmyLeak::Find(GetWorld())) Leak->StopRepair(this);
	ReleaseCargo();
	GetWorldTimerManager().ClearTimer(RecoveryTimer);
	if (auto* PC = Cast<APlayerController>(GetController()); PC && PC->GetLocalPlayer() && InputContext)
		PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->RemoveMappingContext(InputContext);
	Super::EndPlay(Reason);
}

void ACalmyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyCharacter, HeldCargo);
    DOREPLIFETIME(ACalmyCharacter, Health);
    DOREPLIFETIME(ACalmyCharacter, PunchCount); DOREPLIFETIME(ACalmyCharacter, PunchStartedAt);
    DOREPLIFETIME(ACalmyCharacter, MeleeHitsReceived); DOREPLIFETIME(ACalmyCharacter, LastMeleeHitAt);
    DOREPLIFETIME(ACalmyCharacter, GrabRangeCm);
    DOREPLIFETIME(ACalmyCharacter, CrewHelper);
	DOREPLIFETIME(ACalmyCharacter, Lifecycle);
	DOREPLIFETIME(ACalmyCharacter, KnockdownCount);
	DOREPLIFETIME(ACalmyCharacter, RecoveryCount);
	DOREPLIFETIME(ACalmyCharacter, RejectedCommands);
	DOREPLIFETIME(ACalmyCharacter, ResetCount);
}
