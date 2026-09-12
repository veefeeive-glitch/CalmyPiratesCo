#include "Gameplay/CalmyHullProxy.h"
#include "Gameplay/CalmyOcean.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
ACalmyHullProxy::ACalmyHullProxy()
{
	bReplicates = true; bAlwaysRelevant = true; SetReplicatingMovement(true); SetNetUpdateFrequency(30.f);
	PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PrePhysics;
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull")); SetRootComponent(Hull);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Hull->SetStaticMesh(Cube.Object);
	Hull->SetWorldScale3D(FVector(8.f, 3.f, 1.2f)); Hull->SetMobility(EComponentMobility::Movable);
	Hull->SetSimulatePhysics(false); Hull->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Hull->SetCollisionObjectType(ECC_PhysicsBody); Hull->SetCollisionResponseToAllChannels(ECR_Block);
	Hull->SetCanEverAffectNavigation(false); Hull->SetGenerateOverlapEvents(false);
	Deck = CreateDefaultSubobject<UBoxComponent>(TEXT("Deck")); Deck->SetupAttachment(Hull);
	Deck->SetBoxExtent(FVector(380.f, 140.f, 12.f)); Deck->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Deck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Deck->SetCollisionResponseToAllChannels(ECR_Block);
	Deck->SetCanEverAffectNavigation(false); Deck->SetHiddenInGame(true); Deck->SetGenerateOverlapEvents(false);
	Deck->CanCharacterStepUpOn = ECB_Yes;
}
ACalmyHullProxy* ACalmyHullProxy::Find(const UWorld* World)
{ if (!World) return nullptr; for (TActorIterator<ACalmyHullProxy> It(World); It; ++It) return *It; return nullptr; }
void ACalmyHullProxy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); }
void ACalmyHullProxy::CaptureRest() { RestLocation = GetActorLocation(); bCapturedRest = true; }
void ACalmyHullProxy::ConfigurePhysics()
{
	if (!Hull) return; Hull->SetSimulatePhysics(true); Hull->SetEnableGravity(true);
	Hull->SetMassOverrideInKg(NAME_None, HullMassKg, true); Hull->SetCenterOfMass(CenterOfMassLocal);
	Hull->SetAngularDamping(1.4f); Hull->SetLinearDamping(0.35f); Hull->WakeRigidBody();
}
void ACalmyHullProxy::BeginPlay()
{ Super::BeginPlay(); Ocean = ACalmyOcean::Find(GetWorld()); CaptureRest(); if (HasAuthority()) ConfigurePhysics(); }
void ACalmyHullProxy::SoftHome(float DeltaSeconds)
{
	if (!Hull || !Hull->IsSimulatingPhysics() || HomeSpring <= 0.f) return;
	const FVector Loc = Hull->GetComponentLocation();
	const FVector2D Drift(Loc.X - RestLocation.X, Loc.Y - RestLocation.Y);
	if (Drift.Size() > MaxHomeDriftCm)
	{
		const FVector2D Clamped = Drift.GetSafeNormal() * MaxHomeDriftCm;
		Hull->SetWorldLocation(FVector(RestLocation.X + Clamped.X, RestLocation.Y + Clamped.Y, Loc.Z), false, nullptr, ETeleportType::TeleportPhysics);
	}
	const FVector Pull((RestLocation.X - Loc.X) * HomeSpring, (RestLocation.Y - Loc.Y) * HomeSpring, 0.f);
	Hull->AddForce(Pull * Hull->GetMass() * 60.f * DeltaSeconds, NAME_None, true);
}
void ACalmyHullProxy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds); if (!bCapturedRest) CaptureRest();
	if (!HasAuthority() || !Hull || !Hull->IsSimulatingPhysics()) return;
	if (ACalmyOcean* O = Ocean.Get()) O->ApplyBuoyancy(Hull, BuoyancyLiftRatio, true);
	SoftHome(DeltaSeconds);
}
