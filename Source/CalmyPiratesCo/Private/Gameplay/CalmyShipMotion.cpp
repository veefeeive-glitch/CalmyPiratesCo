#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/FlySwarmMob.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACalmyShipMotion::ACalmyShipMotion()
{
	bReplicates = true; bAlwaysRelevant = true; SetReplicatingMovement(true); SetNetUpdateFrequency(30.f);
	PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PrePhysics;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot")); SetRootComponent(Root);
	DeckBase = CreateDefaultSubobject<UBoxComponent>(TEXT("DeckBase")); DeckBase->SetupAttachment(Root);
	DeckBase->SetBoxExtent(FVector(6000.f, 1000.f, 40.f)); DeckBase->SetRelativeLocation(FVector(0.f, 0.f, 900.f));
	DeckBase->SetCollisionEnabled(ECollisionEnabled::NoCollision); DeckBase->SetCanEverAffectNavigation(false);
	DeckBase->SetHiddenInGame(true); DeckBase->SetGenerateOverlapEvents(false);
	Proxy = CreateDefaultSubobject<UBoxComponent>(TEXT("BuoyancyProxy")); Proxy->SetupAttachment(Root);
	Proxy->SetBoxExtent(FVector(2800.f, 700.f, 120.f)); Proxy->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision); Proxy->SetHiddenInGame(true); Proxy->SetCanEverAffectNavigation(false);
	BoardingRamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardingRamp")); BoardingRamp->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) BoardingRamp->SetStaticMesh(Cube.Object);
	BoardingRamp->SetRelativeLocation(FVector(0.f, 1180.f, 420.f));
	BoardingRamp->SetRelativeRotation(FRotator(-28.f, 0.f, 0.f));
	BoardingRamp->SetRelativeScale3D(FVector(2.4f, 0.55f, 0.12f));
	BoardingRamp->SetMobility(EComponentMobility::Movable);
	BoardingRamp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoardingRamp->SetCollisionResponseToAllChannels(ECR_Block);
	BoardingRamp->SetCanEverAffectNavigation(false); BoardingRamp->SetGenerateOverlapEvents(false);
	BoardingRamp->CanCharacterStepUpOn = ECB_Yes; BoardingRamp->SetCastShadow(false);
}
ACalmyShipMotion* ACalmyShipMotion::Find(const UWorld* World)
{ if (!World) return nullptr; for (TActorIterator<ACalmyShipMotion> It(World); It; ++It) return *It; return nullptr; }
void ACalmyShipMotion::BeginPlay()
{ Super::BeginPlay(); Ocean = ACalmyOcean::Find(GetWorld()); CaptureRest(); if (ACalmyOcean* O = Ocean.Get()) O->SetShipMotion(this); LinearVelocity = FVector::ZeroVector; AngularVelocityDeg = FVector::ZeroVector; }
void ACalmyShipMotion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{ if (ACalmyOcean* O = Ocean.Get()) O->SetShipMotion(nullptr); Super::EndPlay(EndPlayReason); }
void ACalmyShipMotion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ACalmyShipMotion, HelmThrottle); DOREPLIFETIME(ACalmyShipMotion, HelmRudder); DOREPLIFETIME(ACalmyShipMotion, AccumulatedYawDeg); DOREPLIFETIME(ACalmyShipMotion, StormBoost); }
void ACalmyShipMotion::SetHelmInput(float Throttle, float Rudder)
{ if (!HasAuthority()) return; HelmThrottle = FMath::Clamp(Throttle, -1.f, 1.f); HelmRudder = FMath::Clamp(Rudder, -1.f, 1.f); ForceNetUpdate(); }
void ACalmyShipMotion::SetStormBoost(float Boost)
{ if (!HasAuthority()) return; StormBoost = FMath::Clamp(Boost, 0.f, 1.f); ForceNetUpdate(); }
void ACalmyShipMotion::CaptureRest()
{ RestTransform = GetActorTransform(); LastAppliedTransform = RestTransform; if (const ACalmyOcean* O = Ocean.Get()) RestDryHullOrigin = O->DryHullOrigin; else RestDryHullOrigin = RestTransform.GetLocation(); bCapturedRest = true; }
float ACalmyShipMotion::ComputeStormBlend() const
{
	float Packet = 0.f;
	if (const ACalmyOcean* O = Ocean.Get())
	{
		const float Sea = ACalmyOcean::SeaStrength(O->WaveTime());
		Packet = FMath::Clamp((Sea - 0.65f) / 2.f, 0.f, 1.f);
	}
	return FMath::Clamp(FMath::Max(Packet, StormBoost), 0.f, 1.f);
}
void ACalmyShipMotion::GetEffectiveMotionLimits(float& OutHeaveCm, float& OutHeelDeg, float& OutDriftSpeed) const
{
	const float Storm = ComputeStormBlend();
	OutHeaveCm = MaxHeaveCm * FMath::Lerp(1.f, StormHeaveGain, Storm);
	OutHeelDeg = MaxHeelDeg * FMath::Lerp(1.f, StormHeelGain, Storm);
	OutDriftSpeed = MaxDriftSpeed * FMath::Lerp(1.f, StormDriftGain, Storm);
}
FTransform ACalmyShipMotion::ClampShipTransform(const FTransform& Proposed, float HeaveCm, float HeelDeg, float DriftSpeed) const
{
	FVector Loc = Proposed.GetLocation(); const FVector RestLoc = RestTransform.GetLocation();
	Loc.Z = FMath::Clamp(Loc.Z, RestLoc.Z - HeaveCm, RestLoc.Z + HeaveCm);
	const FVector2D Drift(Loc.X - RestLoc.X, Loc.Y - RestLoc.Y); const float MaxDrift = DriftSpeed * 20.f;
	if (Drift.Size() > MaxDrift) { const FVector2D C = Drift.GetSafeNormal() * MaxDrift; Loc.X = RestLoc.X + C.X; Loc.Y = RestLoc.Y + C.Y; }
	FRotator Rot = Proposed.Rotator(); const FRotator RestRot = RestTransform.Rotator();
	Rot.Pitch = FMath::Clamp(Rot.Pitch, RestRot.Pitch - HeelDeg, RestRot.Pitch + HeelDeg);
	Rot.Roll = FMath::Clamp(Rot.Roll, RestRot.Roll - HeelDeg, RestRot.Roll + HeelDeg);
	Rot.Yaw = FMath::Clamp(Rot.Yaw, RestRot.Yaw - MaxHelmYawDeg, RestRot.Yaw + MaxHelmYawDeg);
	return FTransform(Rot, Loc, RestTransform.GetScale3D());
}
void ACalmyShipMotion::IntegrateWater(float DeltaSeconds)
{
	ACalmyOcean* O = Ocean.Get(); if (!O || !Proxy) return;
	float HeaveCm = MaxHeaveCm, HeelDeg = MaxHeelDeg, DriftSpeed = MaxDriftSpeed; GetEffectiveMotionLimits(HeaveCm, HeelDeg, DriftSpeed);
	const FVector Ext = Proxy->GetUnscaledBoxExtent(); const FVector Rel = Proxy->GetRelativeLocation(); const FTransform Cur = GetActorTransform();
	struct FSample { float Height; float WaterVZ; FVector Flow; }; TArray<FSample, TInlineAllocator<4>> Samples;
	for (float SX : {-1.f, 1.f}) for (float SY : {-1.f, 1.f})
	{
		const FVector World = Cur.TransformPosition(Rel + FVector(SX * Ext.X * .85f, SY * Ext.Y * .85f, -Ext.Z));
		FSample S; S.Height = O->HeightAt(World); S.WaterVZ = O->VerticalVelocityAt(World); S.Flow = O->FlowAt(World); Samples.Add(S);
	}
	float MeanSurface = 0.f, MeanWaterVZ = 0.f; FVector MeanFlow = FVector::ZeroVector;
	for (const FSample& S : Samples) { MeanSurface += S.Height; MeanWaterVZ += S.WaterVZ; MeanFlow += S.Flow; }
	MeanSurface /= Samples.Num(); MeanWaterVZ /= Samples.Num(); MeanFlow /= Samples.Num();
	const FVector ProxyWorld = Cur.TransformPosition(Rel); const float Immersion = MeanSurface - ProxyWorld.Z;
	const float LiftAccel = FMath::Clamp(Immersion * 2.8f * BuoyancyLiftRatio - (LinearVelocity.Z - MeanWaterVZ) * 1.6f, -220.f, 220.f);
	LinearVelocity.Z += LiftAccel * DeltaSeconds;
	const FVector Forward = RestTransform.GetRotation().GetForwardVector();
	const FVector TargetXY = FVector(MeanFlow.X, MeanFlow.Y, 0.f) + Forward * (HelmThrottle * HelmSurgeSpeed);
	LinearVelocity.X = FMath::FInterpTo(LinearVelocity.X, TargetXY.X, DeltaSeconds, 1.4f);
	LinearVelocity.Y = FMath::FInterpTo(LinearVelocity.Y, TargetXY.Y, DeltaSeconds, 1.4f);
	LinearVelocity.X = FMath::Clamp(LinearVelocity.X, -DriftSpeed, DriftSpeed);
	LinearVelocity.Y = FMath::Clamp(LinearVelocity.Y, -DriftSpeed, DriftSpeed);
	LinearVelocity.Z = FMath::Clamp(LinearVelocity.Z, -DriftSpeed, DriftSpeed);
	const float Port = .5f * (Samples[0].Height + Samples[1].Height);
	const float Stbd = .5f * (Samples[2].Height + Samples[3].Height);
	const float Fore = .5f * (Samples[1].Height + Samples[3].Height);
	const float Aft = .5f * (Samples[0].Height + Samples[2].Height);
	const float Beam = FMath::Max(1.f, Ext.Y * 1.7f); const float Length = FMath::Max(1.f, Ext.X * 1.7f);
	const float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2(Stbd - Port, Beam));
	const float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(Aft - Fore, Length));
	const FRotator Now = GetActorRotation(); const FRotator RestRot = RestTransform.Rotator();
	AngularVelocityDeg.X = FMath::FInterpTo(AngularVelocityDeg.X, (FMath::Clamp(TargetRoll, -HeelDeg, HeelDeg) - (Now.Roll - RestRot.Roll)) * 2.f, DeltaSeconds, 3.f);
	AngularVelocityDeg.Y = FMath::FInterpTo(AngularVelocityDeg.Y, (FMath::Clamp(TargetPitch, -HeelDeg, HeelDeg) - (Now.Pitch - RestRot.Pitch)) * 2.f, DeltaSeconds, 3.f);
	AngularVelocityDeg.Z = HelmRudder * MaxYawRateDeg;
	AccumulatedYawDeg = FMath::Clamp(AccumulatedYawDeg + AngularVelocityDeg.Z * DeltaSeconds, -MaxHelmYawDeg, MaxHelmYawDeg);
	FVector NextLoc = GetActorLocation() + LinearVelocity * DeltaSeconds; FRotator NextRot = Now;
	NextRot.Roll += AngularVelocityDeg.X * DeltaSeconds; NextRot.Pitch += AngularVelocityDeg.Y * DeltaSeconds; NextRot.Yaw = RestRot.Yaw + AccumulatedYawDeg;
	NextLoc += (RestTransform.GetLocation() - NextLoc) * 0.04f * DeltaSeconds * 60.f; LinearVelocity *= (1.f - 0.35f * DeltaSeconds);
	const FTransform Clamped = ClampShipTransform(FTransform(NextRot, NextLoc, RestTransform.GetScale3D()), HeaveCm, HeelDeg, DriftSpeed);
	SetActorLocationAndRotation(FMath::VInterpTo(GetActorLocation(), Clamped.GetLocation(), DeltaSeconds, FollowInterp), FMath::QInterpTo(GetActorQuat(), Clamped.GetRotation(), DeltaSeconds, FollowInterp).Rotator(), false, nullptr, ETeleportType::None);
}
void ACalmyShipMotion::CarryWorldRiders()
{
	if (!HasAuthority()) return;
	const FTransform Delta = GetActorTransform() * LastAppliedTransform.Inverse(); LastAppliedTransform = GetActorTransform();
	if (Delta.Equals(FTransform::Identity, 0.05f)) return;
	for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It) It->ApplyShipMotion(Delta);
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
	{
		ACalmyCargo* Cargo = *It; if (!IsValid(Cargo) || Cargo->Carry.bDelivered || Cargo->Carry.Holder) continue;
		UPrimitiveComponent* Mesh = Cast<UPrimitiveComponent>(Cargo->GetRootComponent());
		if (Mesh && Mesh->IsSimulatingPhysics()) continue;
		Cargo->SetActorTransform(Delta * Cargo->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	for (TActorIterator<ACalmyCrewHelper> It(GetWorld()); It; ++It)
	{
		ACalmyCrewHelper* Helper = *It; if (!IsValid(Helper) || !Helper->IsControlled()) continue;
		UCharacterMovementComponent* Move = Helper->GetCharacterMovement();
		if (Move && Move->GetMovementBaseObject()) continue;
		Helper->SetActorLocation(Delta.TransformPosition(Helper->GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);
	}
}
void ACalmyShipMotion::Tick(float DeltaSeconds)
{ Super::Tick(DeltaSeconds); if (!bCapturedRest) CaptureRest(); if (HasAuthority()) { IntegrateWater(DeltaSeconds); CarryWorldRiders(); } }
