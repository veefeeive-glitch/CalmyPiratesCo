#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/FlySwarmMob.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

ACalmyShipMotion::ACalmyShipMotion()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(30.f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
	SetRootComponent(Root);

	DeckBase = CreateDefaultSubobject<UBoxComponent>(TEXT("DeckBase"));
	DeckBase->SetupAttachment(Root);
	DeckBase->SetBoxExtent(FVector(6000.f, 1000.f, 40.f));
	DeckBase->SetRelativeLocation(FVector(0.f, 0.f, 900.f));
	// Marker only — walkable collision comes from attached R7 meshes.
	DeckBase->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeckBase->SetCanEverAffectNavigation(false);
	DeckBase->SetHiddenInGame(true);
	DeckBase->SetGenerateOverlapEvents(false);

	// Proxy marks the waterline sample slab (kinematic). Kept for authored extent/placement.
	Proxy = CreateDefaultSubobject<UBoxComponent>(TEXT("BuoyancyProxy"));
	Proxy->SetupAttachment(Root);
	Proxy->SetBoxExtent(FVector(2800.f, 700.f, 120.f));
	Proxy->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Proxy->SetHiddenInGame(true);
	Proxy->SetCanEverAffectNavigation(false);
}

ACalmyShipMotion* ACalmyShipMotion::Find(const UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<ACalmyShipMotion> It(World); It; ++It) return *It;
	return nullptr;
}

void ACalmyShipMotion::BeginPlay()
{
	Super::BeginPlay();
	Ocean = ACalmyOcean::Find(GetWorld());
	CaptureRest();
	if (ACalmyOcean* O = Ocean.Get()) O->SetShipMotion(this);
	LinearVelocity = FVector::ZeroVector;
	AngularVelocityDeg = FVector::ZeroVector;
}

void ACalmyShipMotion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACalmyOcean* O = Ocean.Get()) O->SetShipMotion(nullptr);
	Super::EndPlay(EndPlayReason);
}

void ACalmyShipMotion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ACalmyShipMotion::CaptureRest()
{
	RestTransform = GetActorTransform();
	LastAppliedTransform = RestTransform;
	if (const ACalmyOcean* O = Ocean.Get()) RestDryHullOrigin = O->DryHullOrigin;
	else RestDryHullOrigin = RestTransform.GetLocation();
	bCapturedRest = true;
}

FTransform ACalmyShipMotion::ClampShipTransform(const FTransform& Proposed) const
{
	FVector Loc = Proposed.GetLocation();
	const FVector RestLoc = RestTransform.GetLocation();
	Loc.Z = FMath::Clamp(Loc.Z, RestLoc.Z - MaxHeaveCm, RestLoc.Z + MaxHeaveCm);
	const FVector2D Drift(Loc.X - RestLoc.X, Loc.Y - RestLoc.Y);
	const float MaxDrift = MaxDriftSpeed * 20.f;
	if (Drift.Size() > MaxDrift)
	{
		const FVector2D Clamped = Drift.GetSafeNormal() * MaxDrift;
		Loc.X = RestLoc.X + Clamped.X;
		Loc.Y = RestLoc.Y + Clamped.Y;
	}
	FRotator Rot = Proposed.Rotator();
	const FRotator RestRot = RestTransform.Rotator();
	Rot.Pitch = FMath::Clamp(Rot.Pitch, RestRot.Pitch - MaxHeelDeg, RestRot.Pitch + MaxHeelDeg);
	Rot.Roll = FMath::Clamp(Rot.Roll, RestRot.Roll - MaxHeelDeg, RestRot.Roll + MaxHeelDeg);
	Rot.Yaw = RestRot.Yaw;
	return FTransform(Rot, Loc, RestTransform.GetScale3D());
}

void ACalmyShipMotion::IntegrateWater(float DeltaSeconds)
{
	ACalmyOcean* O = Ocean.Get();
	if (!O || !Proxy) return;

	// Four waterline samples in rest-proxy local space, transformed by current pose.
	const FVector Ext = Proxy->GetUnscaledBoxExtent();
	const FVector Rel = Proxy->GetRelativeLocation();
	const FTransform Cur = GetActorTransform();
	struct FSample { FVector Local; float Height; float WaterVZ; FVector Flow; };
	TArray<FSample, TInlineAllocator<4>> Samples;
	for (float SX : {-1.f, 1.f}) for (float SY : {-1.f, 1.f})
	{
		const FVector Local = Rel + FVector(SX * Ext.X * .85f, SY * Ext.Y * .85f, -Ext.Z);
		const FVector World = Cur.TransformPosition(Local);
		FSample S;
		S.Local = Local;
		S.Height = O->HeightAt(World);
		S.WaterVZ = O->VerticalVelocityAt(World);
		S.Flow = O->FlowAt(World);
		Samples.Add(S);
	}

	float MeanSurface = 0.f, MeanWaterVZ = 0.f;
	FVector MeanFlow = FVector::ZeroVector;
	for (const FSample& S : Samples)
	{
		MeanSurface += S.Height;
		MeanWaterVZ += S.WaterVZ;
		MeanFlow += S.Flow;
	}
	MeanSurface /= Samples.Num();
	MeanWaterVZ /= Samples.Num();
	MeanFlow /= Samples.Num();

	// Target heave: keep proxy centre near mean surface (buoyancy equilibrium).
	const FVector ProxyWorld = Cur.TransformPosition(Rel);
	const float Immersion = MeanSurface - ProxyWorld.Z;
	const float LiftAccel = FMath::Clamp(Immersion * 2.8f * BuoyancyLiftRatio - (LinearVelocity.Z - MeanWaterVZ) * 1.6f, -220.f, 220.f);
	LinearVelocity.Z += LiftAccel * DeltaSeconds;
	LinearVelocity.X = FMath::FInterpTo(LinearVelocity.X, MeanFlow.X, DeltaSeconds, 1.2f);
	LinearVelocity.Y = FMath::FInterpTo(LinearVelocity.Y, MeanFlow.Y, DeltaSeconds, 1.2f);
	LinearVelocity.X = FMath::Clamp(LinearVelocity.X, -MaxDriftSpeed, MaxDriftSpeed);
	LinearVelocity.Y = FMath::Clamp(LinearVelocity.Y, -MaxDriftSpeed, MaxDriftSpeed);
	LinearVelocity.Z = FMath::Clamp(LinearVelocity.Z, -MaxDriftSpeed, MaxDriftSpeed);

	// Heel from port/starboard and fore/aft surface difference (same field as cargo buoyancy).
	const float Port = .5f * (Samples[0].Height + Samples[1].Height);
	const float Stbd = .5f * (Samples[2].Height + Samples[3].Height);
	const float Fore = .5f * (Samples[1].Height + Samples[3].Height);
	const float Aft = .5f * (Samples[0].Height + Samples[2].Height);
	const float Beam = FMath::Max(1.f, Ext.Y * 1.7f);
	const float Length = FMath::Max(1.f, Ext.X * 1.7f);
	const float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2(Stbd - Port, Beam));
	const float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(Aft - Fore, Length));
	const FRotator Now = GetActorRotation();
	const FRotator RestRot = RestTransform.Rotator();
	AngularVelocityDeg.X = FMath::FInterpTo(AngularVelocityDeg.X, (FMath::Clamp(TargetRoll, -MaxHeelDeg, MaxHeelDeg) - (Now.Roll - RestRot.Roll)) * 2.f, DeltaSeconds, 3.f);
	AngularVelocityDeg.Y = FMath::FInterpTo(AngularVelocityDeg.Y, (FMath::Clamp(TargetPitch, -MaxHeelDeg, MaxHeelDeg) - (Now.Pitch - RestRot.Pitch)) * 2.f, DeltaSeconds, 3.f);
	AngularVelocityDeg.Z = 0.f;

	FVector NextLoc = GetActorLocation() + LinearVelocity * DeltaSeconds;
	FRotator NextRot = Now;
	NextRot.Roll += AngularVelocityDeg.X * DeltaSeconds;
	NextRot.Pitch += AngularVelocityDeg.Y * DeltaSeconds;
	NextRot.Yaw = RestRot.Yaw;

	// Soft spring home so long-term drift stays bounded.
	const FVector HomePull = (RestTransform.GetLocation() - NextLoc) * 0.04f;
	NextLoc += HomePull * DeltaSeconds * 60.f;
	LinearVelocity *= (1.f - 0.35f * DeltaSeconds);

	const FTransform Clamped = ClampShipTransform(FTransform(NextRot, NextLoc, RestTransform.GetScale3D()));
	const FVector SmoothedLoc = FMath::VInterpTo(GetActorLocation(), Clamped.GetLocation(), DeltaSeconds, FollowInterp);
	const FQuat SmoothedRot = FMath::QInterpTo(GetActorQuat(), Clamped.GetRotation(), DeltaSeconds, FollowInterp);
	SetActorLocationAndRotation(SmoothedLoc, SmoothedRot.Rotator(), false, nullptr, ETeleportType::None);
}

void ACalmyShipMotion::CarryWorldRiders()
{
	if (!HasAuthority()) return;
	const FTransform Delta = GetActorTransform() * LastAppliedTransform.Inverse();
	LastAppliedTransform = GetActorTransform();
	if (Delta.Equals(FTransform::Identity, 0.05f)) return;

	for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It)
		It->ApplyShipMotion(Delta);

	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
	{
		ACalmyCargo* Cargo = *It;
		if (!IsValid(Cargo) || Cargo->Carry.bDelivered || Cargo->Carry.Holder) continue;
		UPrimitiveComponent* Mesh = Cast<UPrimitiveComponent>(Cargo->GetRootComponent());
		if (Mesh && Mesh->IsSimulatingPhysics()) continue;
		Cargo->SetActorTransform(Delta * Cargo->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// NPC helpers standing on the freighter (not currently walking on a movement base).
	for (TActorIterator<ACalmyCrewHelper> It(GetWorld()); It; ++It)
	{
		ACalmyCrewHelper* Helper = *It;
		if (!IsValid(Helper) || !Helper->IsControlled()) continue;
		UCharacterMovementComponent* Move = Helper->GetCharacterMovement();
		if (Move && Move->GetMovementBaseObject()) continue; // based movement already carries them
		Helper->SetActorLocation(Delta.TransformPosition(Helper->GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ACalmyShipMotion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bCapturedRest) CaptureRest();
	if (HasAuthority())
	{
		IntegrateWater(DeltaSeconds);
		CarryWorldRiders();
	}
}
