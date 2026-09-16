#include "Gameplay/CalmyFuelStation.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Core/CalmyAudio.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACalmyFuelStation::ACalmyFuelStation()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.f);
	Pump = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuelPump"));
	SetRootComponent(Pump);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/M_Cargo.M_Cargo"));
	Pump->SetStaticMesh(Cube.Object);
	Pump->SetRelativeScale3D(FVector(.55f, .55f, 1.1f));
	Pump->SetMaterial(0, Material.Object);
	Pump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TaskLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FuelTaskLight"));
	TaskLight->SetupAttachment(Pump);
	TaskLight->SetRelativeLocation(FVector(0, 0, 180));
	// A marker, not a floodlight: the old 42 W green lamp washed the whole bunker station and the deck around it.
	TaskLight->SetLightColor(FLinearColor(.25f, .85f, .4f));
	TaskLight->SetIntensity(900.f);
	TaskLight->SetAttenuationRadius(280.f);
	TaskLight->SetCastShadows(false);
}

ACalmyFuelStation* ACalmyFuelStation::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyFuelStation> It(World); It; ++It) return *It;
	return nullptr;
}

void ACalmyFuelStation::BeginPlay()
{
	Super::BeginPlay();
	// Spawned in ship-local coordinates: without this the pump drifts metres away from the deck in a storm.
	ACalmyShipMotion::AttachRider(this);
	// On R7 the S3 bunker station (manifold, valves, hose rack) is the pump: the placeholder block would float in it.
	if (ACalmyVoyageDirector::IsShipMap(GetWorld())) Pump->SetVisibility(false);
}

void ACalmyFuelStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyFuelStation, Refueler);
}

bool ACalmyFuelStation::CanRefuel(const ACalmyCharacter* Pawn) const
{
	if (!IsValid(Pawn) || Pawn->GetWorld() != GetWorld() || Pawn->HeldCargo || !Pawn->IsControlled() || Pawn->IsInOcean()) return false;
	const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	if (!Voyage || Voyage->State.Phase != ECalmyVoyagePhase::Active || Voyage->State.FuelPercent >= 99) return false;
	return FVector::Dist(Pawn->GetActorLocation(), GetActorLocation()) <= 220.f;
}

bool ACalmyFuelStation::TryRefuelPulse(ACalmyCharacter* Pawn)
{
	if (!HasAuthority() || !CanRefuel(Pawn)) return false;
	const double Now = GetWorld()->GetTimeSeconds();
	if (IsValid(Refueler) && Refueler != Pawn && Now < RefuelLeaseEnd) return false;
	Refueler = Pawn;
	RefuelLeaseEnd = Now + .45;
	if (auto* Voyage = ACalmyVoyageDirector::Find(GetWorld()))
	{
		Voyage->ApplyFuelPulse(4.f);
	}
	ForceNetUpdate();
	return true;
}

void ACalmyFuelStation::StopRefuel(ACalmyCharacter* Pawn)
{
	if (!HasAuthority() || Refueler != Pawn) return;
	Refueler = nullptr;
	ForceNetUpdate();
}

FString ACalmyFuelStation::GetStatusText() const
{
	const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	const int32 Fuel = Voyage ? Voyage->State.FuelPercent : 0;
	if (IsValid(Refueler)) return FString::Printf(TEXT("REFUELING  ·  FUEL %d%%"), Fuel);
	return FString::Printf(TEXT("FUEL STATION  ·  %d%%  ·  Hold F"), Fuel);
}

void ACalmyFuelStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	const bool bActive = Voyage && Voyage->State.Phase == ECalmyVoyagePhase::Active;
	// Presentation from replicated state on every machine: clients see the task light and hear the pump too.
	TaskLight->SetVisibility(bActive && (IsValid(Refueler) || Voyage->State.FuelPercent < 85));
	if (bActive && IsValid(Refueler))
	{
		NextPulseSound -= DeltaSeconds;
		if (NextPulseSound <= 0.f) { NextPulseSound = .45f; FCalmyAudio::PlayAt(this, ECalmySoundCue::FuelPulse, GetActorLocation(), .7f); }
	}
	else NextPulseSound = 0.f;
	if (HasAuthority() && IsValid(Refueler) && GetWorld()->GetTimeSeconds() > RefuelLeaseEnd + .35)
	{
		Refueler = nullptr;
		ForceNetUpdate();
	}
}
