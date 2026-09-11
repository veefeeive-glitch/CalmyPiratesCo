#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyLeak.h"
#include "Core/GameplayPolicies.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

ACalmyShift::ACalmyShift()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	bReplicates = true;
	bAlwaysRelevant = true;
	DeliveryZone = CreateDefaultSubobject<UBoxComponent>(TEXT("DeliveryZone"));
	SetRootComponent(DeliveryZone);
	DeliveryZone->SetBoxExtent(FVector(170, 180, 120));
	DeliveryZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

ACalmyShift* ACalmyShift::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyShift> It(World); It; ++It) return *It;
	return nullptr;
}

void ACalmyShift::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) InitializeManifest();
}

void ACalmyShift::InitializeManifest()
{
	if (!HasAuthority()) return;
	Manifest.Reset();
	SettledTime.Reset();
	AcceptedIds.Reset();
	State.Required = 0;
	State.Delivered = 0;
	State.DamagedDelivered = 0;
	RemainingSecondsExact = static_cast<float>(FMath::Max(30, ContractDurationSeconds));
	State.SecondsRemaining = FMath::CeilToInt(RemainingSecondsExact);
	State.FinalPayout = 0;
	State.PerformanceStars = 0;
	State.Phase = ECalmyShiftPhase::Loading;
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
	{
		if (It->CargoId <= 0 || Manifest.Contains(It->CargoId)) State.Phase = ECalmyShiftPhase::InvalidManifest;
		Manifest.Add(It->CargoId, *It);
	}
	State.Required = Manifest.Num();
	if (!State.Required) State.Phase = ECalmyShiftPhase::InvalidManifest;
	ForceNetUpdate();
}

bool ACalmyShift::IsEligible(const ACalmyCargo* Cargo) const
{
	if (!IsValid(Cargo) || Cargo->GetWorld() != GetWorld() || State.Phase != ECalmyShiftPhase::Loading ||
		Cargo->Carry.bDelivered || AcceptedIds.Contains(Cargo->CargoId) || Cargo->Carry.State != ECalmyCarryState::Free || Cargo->Carry.Holder) return false;
	const auto* Entry = Manifest.Find(Cargo->CargoId);
	if (!Entry || Entry->Get() != Cargo || Cargo->Mesh->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(60.f) ||
		Cargo->Mesh->GetPhysicsAngularVelocityInDegrees().SizeSquared() > FMath::Square(45.f)) return false;
	// The entire world-space cargo bounds must fit inside the zone, not just its pivot.
	const FBox Box = Cargo->Mesh->Bounds.GetBox();
	const FTransform Transform = DeliveryZone->GetComponentTransform();
	const FVector Extent = DeliveryZone->GetUnscaledBoxExtent();
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector P((Corner & 1) ? Box.Max.X : Box.Min.X, (Corner & 2) ? Box.Max.Y : Box.Min.Y,
			(Corner & 4) ? Box.Max.Z : Box.Min.Z);
		const FVector Local = Transform.InverseTransformPosition(P).GetAbs();
		if (Local.X > Extent.X || Local.Y > Extent.Y || Local.Z > Extent.Z) return false;
	}
	return true;
}

bool ACalmyShift::TryAccept(ACalmyCargo* Cargo)
{
	if (!HasAuthority() || !IsEligible(Cargo) || SettledTime.FindRef(Cargo->CargoId) < .75f) return false;
	Cargo->MarkDelivered();
	AcceptedIds.Add(Cargo->CargoId);
	++State.Delivered;
	if (Cargo->Condition.Integrity < 100) ++State.DamagedDelivered;
	if (State.Delivered == State.Required)
	{
		State.Phase = ECalmyShiftPhase::Complete;
		State.FinalPayout = FCalmyShiftEconomyPolicy::CalculatePayout(State.SecondsRemaining, State.Rescues, State.DamagedDelivered);
		State.PerformanceStars = FCalmyShiftEconomyPolicy::CalculateStars(
			State.SecondsRemaining, State.Rescues, State.DamagedDelivered, State.FinalPayout);
	}
	ForceNetUpdate();
	return true;
}

void ACalmyShift::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || State.Phase != ECalmyShiftPhase::Loading || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f) return;
	const int32 PreviousSeconds = State.SecondsRemaining;
	if(!ACalmyTutorial::Enabled(GetWorld())) RemainingSecondsExact = FMath::Max(0.f, RemainingSecondsExact - DeltaSeconds);
	State.SecondsRemaining = FMath::CeilToInt(RemainingSecondsExact);
	if (State.SecondsRemaining != PreviousSeconds) ForceNetUpdate();
	if (RemainingSecondsExact <= 0.f)
	{
		State.Phase = ECalmyShiftPhase::Failed;
		State.FinalPayout = 0;
		State.PerformanceStars = 0;
		ForceNetUpdate();
		return;
	}
	for (const auto& Entry : Manifest)
	{
		ACalmyCargo* Cargo = Entry.Value.Get();
		if (!Cargo) { State.Phase = ECalmyShiftPhase::InvalidManifest; ForceNetUpdate(); return; }
		float& Time = SettledTime.FindOrAdd(Entry.Key);
		Time = IsEligible(Cargo) ? Time + DeltaSeconds : 0.f;
		TryAccept(Cargo);
	}
}

bool ACalmyShift::TryRestart(ACalmyCharacter* Requester, int32 ExpectedRound)
{
	// Local listen host / solo only. A remote client's owned pawn is still NOT allowed to restart.
	const auto* PC = IsValid(Requester) ? Cast<APlayerController>(Requester->GetController()) : nullptr;
	if (!HasAuthority() || !PC || !PC->IsLocalController() || Requester->GetWorld() != GetWorld() ||
		(State.Phase != ECalmyShiftPhase::Complete && State.Phase != ECalmyShiftPhase::Failed) || State.Round != ExpectedRound) return false;
	for (auto& Entry : Manifest) if (Entry.Value.IsValid()) Entry.Value->ResetHome(false);
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
    {
        if(Cast<ACalmyCrewHelper>(*It)) continue;
        // Crew remains in the world; clear the job at a round boundary.
        if(IsValid(It->CrewHelper)) It->CrewHelper->ReleaseLeader();
        It->CrewHelper=nullptr;It->ForceNetUpdate();It->ReturnToDeck(false);
    }
    for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It) It->ResetToStation();
	++State.Round;
	State.Rescues = 0;
	InitializeManifest();
	if (auto* Leak = ACalmyLeak::Find(GetWorld())) Leak->ResetForRound(State.Round);
	return true;
}

void ACalmyShift::RecordRescue()
{
	if (HasAuthority() && State.Phase == ECalmyShiftPhase::Loading) { ++State.Rescues; ForceNetUpdate(); }
}

void ACalmyShift::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyShift, State);
}
