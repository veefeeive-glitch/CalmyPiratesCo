#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyFuelStation.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Core/CalmySession.h"
#include "Core/CalmyAudio.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"

namespace CalmyVoyage
{
	// Ship-local placements checked by the playability survey (Scripts/run_ship_playability_ue.ps1) and real walkers.
	// The leak is the hold 1 flooding drill: the bilge suction valve sits on the aft bulkhead (X -20) beside the foot of
	// the hold stair, where the crew arrives; the water collects around the bilge well in the middle of the tank top.
	static const FVector LeakValveLocal(-1972.f, 150.f, 318.f);
	static const FVector LeakPoolLocal(-500.f, 0.f, 210.f);
	// Fuel is taken at the SB bunker station built on the hold 1 coaming (S3), between its two manifold valves,
	// reached from the side lane of the weather deck.
	static const FVector FuelStationLocal(-1810.f, -800.f, 1150.f);
	static const FVector DeliveryLocal(0.f, -830.f, 1130.f);
	static constexpr float BriefingSeconds = 12.f;
	// HOLD 1 route readability: marks lie on real floor (stair head, stair foot, stowage), never in mid-air.
	static const FVector RouteMarks[] = {
		FVector(-1335.f, 408.f, 1062.f),  // hold 1 stair head, weather deck
		FVector(-1865.f, 408.f, 212.f),   // hold 1 stair foot, tank top
		FVector(-850.f, -250.f, 212.f),   // HOLD 1 stowage
	};
}

FVector ACalmyVoyageDirector::ShipLocalToWorld(const UWorld* World, const FVector& Local)
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(World);
	return Ship ? Ship->GetActorTransform().TransformPosition(Ship->GetRestTransform().InverseTransformPosition(Local)) : Local;
}

FVector ACalmyVoyageDirector::LeakPoolLocal() { return CalmyVoyage::LeakPoolLocal; }
FVector ACalmyVoyageDirector::LeakValveLocal() { return CalmyVoyage::LeakValveLocal; }
FVector ACalmyVoyageDirector::FuelStationLocal() { return CalmyVoyage::FuelStationLocal; }
FVector ACalmyVoyageDirector::DeliveryLocal() { return CalmyVoyage::DeliveryLocal; }

ACalmyVoyageDirector::ACalmyVoyageDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.f);
	ActiveContract = FCalmyVoyageContractStore::StandardG3b();
}

ACalmyVoyageDirector* ACalmyVoyageDirector::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyVoyageDirector> It(World); It; ++It) return *It;
	return nullptr;
}

bool ACalmyVoyageDirector::IsShipMap(const UWorld* World)
{
	if (!World) return false;
	if (World->GetMapName().Contains(TEXT("OceanFreighterR7"))) return true;
	if (World->URL.Map.Contains(TEXT("OceanFreighterR7"))) return true;
	return World->GetName().Contains(TEXT("OceanFreighterR7"));
}

bool ACalmyVoyageDirector::ShouldFreezeShiftTimer(const UWorld* World)
{
	const auto* Voyage = Find(World);
	return Voyage && Voyage->State.Phase != ECalmyVoyagePhase::Active;
}

bool ACalmyVoyageDirector::UsesCuratedManifest(const UWorld* World)
{
	return Find(World) != nullptr;
}

void ACalmyVoyageDirector::ApplyContract(const FCalmyVoyageContract& Contract)
{
	ActiveContract = Contract;
	WatchDurationSeconds = FMath::Clamp(Contract.WatchDurationSeconds, 300, 1800);
	State.ContractId = Contract.ContractId;
	State.ContractName = Contract.DisplayName;
	State.RequiredStandard = FMath::Max(0, Contract.RequiredStandard);
	State.RequiredBulky = FMath::Max(0, Contract.RequiredBulky);
	State.RequiredFragile = FMath::Max(0, Contract.RequiredFragile);
	State.RequiredUnstable = FMath::Max(0, Contract.RequiredUnstable);
	State.WatchSeconds = WatchDurationSeconds;
}

void ACalmyVoyageDirector::BeginPlay()
{
	Super::BeginPlay();
	// Route aids are cosmetic: every machine builds its own copy on the moving ship.
	EnsureHoldRouteAids();
	if (!HasAuthority()) return;
	const bool bAccepted = FCalmyVoyageContractStore::HasAccepted();
	ApplyContract(bAccepted ? FCalmyVoyageContractStore::LoadAccepted() : FCalmyVoyageContractStore::StandardG3b());
	// Consume the port hand-off so a later direct launch of the ship map never reuses a stale contract.
	if (bAccepted) FCalmyVoyageContractStore::ClearAccepted();
	EnsureShipFixtures();
	FCalmyAudio::EnsureWorldAmbience(GetWorld());
	const auto* Shift = ACalmyShift::Find(GetWorld());
	EnterBriefing(Shift ? Shift->State.Round : 1);
}

void ACalmyVoyageDirector::EnsureShipFixtures()
{
	if (!GetWorld()) return;
	if (!ACalmyShift::Find(GetWorld()))
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Shift = GetWorld()->SpawnActor<ACalmyShift>(ShipLocalToWorld(GetWorld(), CalmyVoyage::DeliveryLocal), FRotator::ZeroRotator, Spawn);
		if (Shift)
		{
			Shift->DeliveryZone->SetBoxExtent(FVector(250.f, 90.f, 140.f));
#if WITH_EDITOR
			Shift->SetActorLabel(TEXT("R7_Voyage_Delivery"));
#endif
		}
	}
	if (ActiveContract.bLeak && !ACalmyLeak::Find(GetWorld()))
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Leak = GetWorld()->SpawnActor<ACalmyLeak>(ShipLocalToWorld(GetWorld(), CalmyVoyage::LeakValveLocal), FRotator::ZeroRotator, Spawn);
#if WITH_EDITOR
		if (Leak) Leak->SetActorLabel(TEXT("R7_Voyage_LeakValve"));
#endif
	}
	if (ActiveContract.bFuel && !ACalmyFuelStation::Find(GetWorld()))
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Fuel = GetWorld()->SpawnActor<ACalmyFuelStation>(ShipLocalToWorld(GetWorld(), CalmyVoyage::FuelStationLocal), FRotator::ZeroRotator, Spawn);
#if WITH_EDITOR
		if (Fuel) Fuel->SetActorLabel(TEXT("R7_Voyage_FuelStation"));
#endif
	}
	State.bLeakArmed = ActiveContract.bLeak && ACalmyLeak::Find(GetWorld()) != nullptr;
	State.bFuelStationArmed = ActiveContract.bFuel && ACalmyFuelStation::Find(GetWorld()) != nullptr;
}

void ACalmyVoyageDirector::EnsureHoldRouteAids()
{
	if (!GetWorld() || bRouteAidsSpawned) return;
	bRouteAidsSpawned = true;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* MarkMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_WaterLab_Delivery.M_WaterLab_Delivery"));
	// Marks are placed in ship-local coordinates; a late joiner may arrive after the hull has drifted.
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	const FTransform ShipFrame = Ship ? Ship->GetActorTransform() : FTransform::Identity;
	for (int32 I = 0; I < UE_ARRAY_COUNT(CalmyVoyage::RouteMarks); ++I)
	{
		const FVector P = ShipFrame.TransformPosition(CalmyVoyage::RouteMarks[I]);
		if (Cube)
		{
			auto* Mark = GetWorld()->SpawnActor<AStaticMeshActor>(P, FRotator::ZeroRotator, Spawn);
			if (Mark)
			{
				Mark->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
				Mark->GetStaticMeshComponent()->SetStaticMesh(Cube);
				Mark->SetActorScale3D(FVector(.35f, .35f, .08f));
				if (MarkMat) Mark->GetStaticMeshComponent()->SetMaterial(0, MarkMat);
				Mark->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
#if WITH_EDITOR
				Mark->SetActorLabel(FString::Printf(TEXT("R7_HoldRoute_Mark_%d"), I + 1));
#endif
				ACalmyShipMotion::AttachRider(Mark);
			}
		}
		auto* LightHolder = GetWorld()->SpawnActor<AStaticMeshActor>(P + FVector(0, 0, 90.f), FRotator::ZeroRotator, Spawn);
		if (!LightHolder) continue;
#if WITH_EDITOR
		LightHolder->SetActorLabel(FString::Printf(TEXT("R7_HoldRoute_Light_%d"), I + 1));
#endif
		LightHolder->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		if (Cube)
		{
			LightHolder->GetStaticMeshComponent()->SetStaticMesh(Cube);
			LightHolder->SetActorScale3D(FVector(.05f));
			LightHolder->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			LightHolder->GetStaticMeshComponent()->SetVisibility(false);
		}
		auto* Comp = NewObject<UPointLightComponent>(LightHolder, TEXT("RouteLight"));
		Comp->SetupAttachment(LightHolder->GetRootComponent());
		Comp->RegisterComponent();
		Comp->SetRelativeLocation(FVector::ZeroVector);
		Comp->SetIntensity(1200.f);
		Comp->SetAttenuationRadius(650.f);
		Comp->SetLightColor(FLinearColor(1.f, .92f, .75f));
		Comp->SetCastShadows(false);
		ACalmyShipMotion::AttachRider(LightHolder);
	}
}

void ACalmyVoyageDirector::EnterBriefing(int32 Round)
{
	State.Round = Round;
	State.Phase = ECalmyVoyagePhase::Briefing;
	State.RequiredStandard = FMath::Max(0, ActiveContract.RequiredStandard);
	State.RequiredBulky = FMath::Max(0, ActiveContract.RequiredBulky);
	State.RequiredFragile = FMath::Max(0, ActiveContract.RequiredFragile);
	State.RequiredUnstable = FMath::Max(0, ActiveContract.RequiredUnstable);
	State.ContractId = ActiveContract.ContractId;
	State.ContractName = ActiveContract.DisplayName;
	State.WatchSeconds = WatchDurationSeconds;
	State.bShiftComplete = false;
	State.bShiftFailed = false;
	State.bFuelStarved = false;
	State.FinalPayout = 0;
	State.PerformanceStars = 0;
	State.FuelPercent = 100;
	State.StormPercent = 0;
	State.ActiveSeconds = 0;
	FuelExact = 100.f;
	ActiveAge = 0.f;
	BriefingRemaining = CalmyVoyage::BriefingSeconds;
	State.BriefingSeconds = FMath::CeilToInt(BriefingRemaining);
	State.bLeakArmed = ActiveContract.bLeak && ACalmyLeak::Find(GetWorld()) != nullptr;
	State.bFuelStationArmed = ActiveContract.bFuel && ACalmyFuelStation::Find(GetWorld()) != nullptr;
	if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(0.f);
	if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(0.f);
	if (auto* Shift = ACalmyShift::Find(GetWorld()))
	{
		Shift->ContractDurationSeconds = FMath::Clamp(WatchDurationSeconds, 300, 1800);
		Shift->InitializeManifest();
	}
	ForceNetUpdate();
}

void ACalmyVoyageDirector::EnterActive()
{
	if (State.Phase != ECalmyVoyagePhase::Briefing) return;
	State.Phase = ECalmyVoyagePhase::Active;
	State.BriefingSeconds = 0;
	BriefingRemaining = 0.f;
	ActiveAge = 0.f;
	FuelExact = 100.f;
	State.FuelPercent = 100;
	State.bFuelStarved = false;
	if (auto* Shift = ACalmyShift::Find(GetWorld()))
	{
		Shift->ContractDurationSeconds = FMath::Clamp(WatchDurationSeconds, 300, 1800);
		Shift->InitializeManifest();
	}
	if (ActiveContract.bLeak)
	{
		if (auto* Leak = ACalmyLeak::Find(GetWorld())) Leak->ResetForRound(State.Round);
	}
	ForceNetUpdate();
}

void ACalmyVoyageDirector::ApplyFuelPulse(float AmountPercent)
{
	if (!HasAuthority() || State.Phase != ECalmyVoyagePhase::Active || !ActiveContract.bFuel) return;
	FuelExact = FMath::Clamp(FuelExact + AmountPercent, 0.f, 100.f);
	const int32 Shown = FMath::Clamp(FMath::RoundToInt(FuelExact), 0, 100);
	if (Shown != State.FuelPercent)
	{
		State.FuelPercent = Shown;
		ForceNetUpdate();
	}
}

void ACalmyVoyageDirector::FailFuelStarvation()
{
	if (State.Phase != ECalmyVoyagePhase::Active) return;
	State.Phase = ECalmyVoyagePhase::Outcome;
	State.bShiftComplete = false;
	State.bShiftFailed = true;
	State.bFuelStarved = true;
	State.FinalPayout = 0;
	State.PerformanceStars = 0;
	State.StormPercent = 0;
	if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(0.f);
	if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(0.f);
	ForceNetUpdate();
}

void ACalmyVoyageDirector::UpdateSeaState(float Age)
{
	if (!ActiveContract.bStorm)
	{
		if (State.StormPercent != 0)
		{
			State.StormPercent = 0;
			ForceNetUpdate();
		}
		if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(0.f);
		if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(0.f);
		return;
	}
	float Storm = 0.f;
	if (Age >= 180.f && Age < 300.f) Storm = .55f;
	else if (Age >= 540.f && Age < 660.f) Storm = .85f;
	else if (Age >= 900.f && Age < 980.f) Storm = .4f;
	const int32 Shown = FMath::RoundToInt(Storm * 100.f);
	if (Shown != State.StormPercent)
	{
		State.StormPercent = Shown;
		ForceNetUpdate();
	}
	if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(Storm);
	if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(Storm);
}

void ACalmyVoyageDirector::SyncFromShift()
{
	const auto* Shift = ACalmyShift::Find(GetWorld());
	if (!Shift) return;
	if (State.Phase == ECalmyVoyagePhase::Active)
	{
		if (Shift->State.Phase == ECalmyShiftPhase::Complete)
		{
			State.Phase = ECalmyVoyagePhase::Outcome;
			State.bShiftComplete = true;
			State.bShiftFailed = false;
			State.bFuelStarved = false;
			State.FinalPayout = Shift->State.FinalPayout;
			State.PerformanceStars = Shift->State.PerformanceStars;
			State.StormPercent = 0;
			if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(0.f);
			if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(0.f);
			ForceNetUpdate();
		}
		else if (Shift->State.Phase == ECalmyShiftPhase::Failed)
		{
			State.Phase = ECalmyVoyagePhase::Outcome;
			State.bShiftComplete = false;
			State.bShiftFailed = true;
			State.FinalPayout = 0;
			State.PerformanceStars = 0;
			State.StormPercent = 0;
			if (auto* Ocean = ACalmyOcean::Find(GetWorld())) Ocean->SetStormIntensity(0.f);
			if (auto* Ship = ACalmyShipMotion::Find(GetWorld())) Ship->SetStormBoost(0.f);
			ForceNetUpdate();
		}
	}
}

bool ACalmyVoyageDirector::TryBeginWatch(ACalmyCharacter* Requester)
{
	const auto* PC = IsValid(Requester) ? Cast<APlayerController>(Requester->GetController()) : nullptr;
	if (!HasAuthority() || !PC || !PC->IsLocalController() || Requester->GetWorld() != GetWorld() ||
		State.Phase != ECalmyVoyagePhase::Briefing)
	{
		return false;
	}
	EnterActive();
	return true;
}

bool ACalmyVoyageDirector::TryRestartWatch(ACalmyCharacter* Requester, int32 ExpectedRound)
{
	auto* Shift = ACalmyShift::Find(GetWorld());
	return Shift && Shift->TryRestart(Requester, ExpectedRound);
}

bool ACalmyVoyageDirector::TryReturnToPort(ACalmyCharacter* Requester)
{
	const auto* PC = IsValid(Requester) ? Cast<APlayerController>(Requester->GetController()) : nullptr;
	// One travel per outcome: repeated requests while ServerTravel is pending are ignored.
	if (!HasAuthority() || bTravelRequested || !PC || !PC->IsLocalController() || State.Phase != ECalmyVoyagePhase::Outcome) return false;
	if (!FCalmySession::CanRequestSessionTravel(GetWorld()->GetNetMode(), true)) return false;
	if (!FCalmySession::IsAllowlistedVoyageTravel(GetWorld()->GetMapName(), FCalmySession::PortHubMap())) return false;
	FCalmyVoyageContractStore::ClearAccepted();
	FCalmyAudio::Play2D(this, ECalmySoundCue::ReturnPort);
	bTravelRequested = true;
	if (!FPackageName::DoesPackageExist(FCalmySession::PortHubMap()) ||
		!GetWorld()->ServerTravel(FCalmySession::PortHubMap(), false))
	{
		UE_LOG(LogTemp, Error, TEXT("CALMY_VOYAGE return_to_port_failed"));
		bTravelRequested = false;
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CALMY_VOYAGE return_to_port"));
	return true;
}

void ACalmyVoyageDirector::NotifyShiftRestarted(int32 NewRound)
{
	if (!HasAuthority()) return;
	EnterBriefing(NewRound);
}

void ACalmyVoyageDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PresentAudio();
	if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f) return;
	if (State.Phase == ECalmyVoyagePhase::Briefing)
	{
		BriefingRemaining = FMath::Max(0.f, BriefingRemaining - DeltaSeconds);
		const int32 Seconds = FMath::CeilToInt(BriefingRemaining);
		if (Seconds != State.BriefingSeconds)
		{
			State.BriefingSeconds = Seconds;
			ForceNetUpdate();
		}
		if (BriefingRemaining <= 0.f) EnterActive();
	}
	else if (State.Phase == ECalmyVoyagePhase::Active)
	{
		ActiveAge += DeltaSeconds;
		const int32 ShownAge = FMath::FloorToInt(ActiveAge);
		if (ShownAge != State.ActiveSeconds)
		{
			State.ActiveSeconds = ShownAge;
			ForceNetUpdate();
		}
		UpdateSeaState(ActiveAge);
		if (ActiveContract.bFuel)
		{
			const float DrainPerSecond = 100.f / FMath::Max(300.f, static_cast<float>(WatchDurationSeconds)) *
				(1.f + State.StormPercent / 100.f);
			FuelExact = FMath::Max(0.f, FuelExact - DrainPerSecond * DeltaSeconds);
			const int32 ShownFuel = FMath::Clamp(FMath::RoundToInt(FuelExact), 0, 100);
			if (ShownFuel != State.FuelPercent)
			{
				State.FuelPercent = ShownFuel;
				ForceNetUpdate();
			}
			if (FuelExact <= 0.f) FailFuelStarvation();
		}
	}
	SyncFromShift();
}

FString ACalmyVoyageDirector::BriefingText() const
{
	return FString::Printf(
		TEXT("WATCH BRIEFING %ds | %s | ~%d min | HOLD 1 → green zone | S%d B%d F%d U%d | Storm/leak/fuel per contract"),
		State.BriefingSeconds,
		State.ContractName.IsEmpty() ? TEXT("CONTRACT") : *State.ContractName,
		FMath::Max(1, State.WatchSeconds / 60),
		State.RequiredStandard, State.RequiredBulky, State.RequiredFragile, State.RequiredUnstable);
}

FString ACalmyVoyageDirector::OutcomeText() const
{
	if (State.bShiftComplete)
	{
		return FString::Printf(TEXT("WATCH COMPLETE | Paid $%d | Rating %d/3 | Fuel %d%% | Host: RETURN TO PORT or R for NEW WATCH"),
			State.FinalPayout, State.PerformanceStars, State.FuelPercent);
	}
	if (State.bFuelStarved)
	{
		return TEXT("WATCH FAILED | Fuel exhausted | Host: RETURN TO PORT or R for NEW WATCH");
	}
	if (State.bShiftFailed)
	{
		return TEXT("WATCH FAILED | Time expired | Host: RETURN TO PORT or R for NEW WATCH");
	}
	return TEXT("Watch settled.");
}

void ACalmyVoyageDirector::OnRep_State()
{
	PresentAudio();
}

void ACalmyVoyageDirector::PresentAudio()
{
	// Host and clients hear watch cues and the storm bed from the replicated snapshot, never from server-only calls.
	const uint8 Phase = static_cast<uint8>(State.Phase);
	if (PresentedPhase == 255)
	{
		PresentedPhase = Phase;
		PresentedStorm = State.StormPercent;
		FCalmyAudio::SetStormLevel(GetWorld(), State.StormPercent / 100.f);
		return;
	}
	if (Phase != PresentedPhase)
	{
		if (State.Phase == ECalmyVoyagePhase::Active) FCalmyAudio::Play2D(this, ECalmySoundCue::WatchBegin);
		else if (State.Phase == ECalmyVoyagePhase::Outcome)
			FCalmyAudio::Play2D(this, State.bShiftComplete ? ECalmySoundCue::WatchOutcomeGood : ECalmySoundCue::WatchOutcomeBad);
		PresentedPhase = Phase;
	}
	if (State.StormPercent != PresentedStorm)
	{
		if (State.StormPercent > 50 && PresentedStorm <= 50) FCalmyAudio::Play2D(this, ECalmySoundCue::StormPulse, .7f);
		FCalmyAudio::SetStormLevel(GetWorld(), State.StormPercent / 100.f);
		PresentedStorm = State.StormPercent;
	}
}

void ACalmyVoyageDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyVoyageDirector, State);
}
