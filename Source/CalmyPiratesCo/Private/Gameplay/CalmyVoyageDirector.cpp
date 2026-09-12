#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

namespace CalmyVoyage
{
	// Weather-deck valve near the green delivery zone so emergency work is reachable without art edits.
	static const FVector LeakLocation(220.f, -620.f, 1130.f);
	static constexpr float BriefingSeconds = 12.f;
}

ACalmyVoyageDirector::ACalmyVoyageDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.f);
}

ACalmyVoyageDirector* ACalmyVoyageDirector::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyVoyageDirector> It(World); It; ++It) return *It;
	return nullptr;
}

bool ACalmyVoyageDirector::IsShipMap(const UWorld* World)
{
	return World && World->GetMapName().Contains(TEXT("OceanFreighterR7"));
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

void ACalmyVoyageDirector::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	EnsureShipFixtures();
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
		auto* Shift = GetWorld()->SpawnActor<ACalmyShift>(FVector(0.f, -830.f, 1130.f), FRotator::ZeroRotator, Spawn);
		if (Shift)
		{
			Shift->DeliveryZone->SetBoxExtent(FVector(250.f, 90.f, 140.f));
			Shift->SetActorLabel(TEXT("R7_Voyage_Delivery"));
		}
	}
	if (!ACalmyLeak::Find(GetWorld()))
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Leak = GetWorld()->SpawnActor<ACalmyLeak>(CalmyVoyage::LeakLocation, FRotator::ZeroRotator, Spawn);
		if (Leak) Leak->SetActorLabel(TEXT("R7_Voyage_LeakValve"));
	}
	State.bLeakArmed = ACalmyLeak::Find(GetWorld()) != nullptr;
}

void ACalmyVoyageDirector::EnterBriefing(int32 Round)
{
	State.Round = Round;
	State.Phase = ECalmyVoyagePhase::Briefing;
	State.RequiredStandard = 1;
	State.RequiredBulky = 1;
	State.bShiftComplete = false;
	State.bShiftFailed = false;
	State.FinalPayout = 0;
	State.PerformanceStars = 0;
	BriefingRemaining = CalmyVoyage::BriefingSeconds;
	State.BriefingSeconds = FMath::CeilToInt(BriefingRemaining);
	State.bLeakArmed = ACalmyLeak::Find(GetWorld()) != nullptr;
	if (auto* Shift = ACalmyShift::Find(GetWorld()))
	{
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
	if (auto* Shift = ACalmyShift::Find(GetWorld()))
	{
		if (Shift->State.Phase == ECalmyShiftPhase::InvalidManifest || Shift->State.Required <= 0)
		{
			Shift->InitializeManifest();
		}
	}
	if (auto* Leak = ACalmyLeak::Find(GetWorld()))
	{
		Leak->ResetForRound(State.Round);
	}
	ForceNetUpdate();
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
			State.FinalPayout = Shift->State.FinalPayout;
			State.PerformanceStars = Shift->State.PerformanceStars;
			ForceNetUpdate();
		}
		else if (Shift->State.Phase == ECalmyShiftPhase::Failed)
		{
			State.Phase = ECalmyVoyagePhase::Outcome;
			State.bShiftComplete = false;
			State.bShiftFailed = true;
			State.FinalPayout = 0;
			State.PerformanceStars = 0;
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

void ACalmyVoyageDirector::NotifyShiftRestarted(int32 NewRound)
{
	if (!HasAuthority()) return;
	EnterBriefing(NewRound);
}

void ACalmyVoyageDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	SyncFromShift();
}

FString ACalmyVoyageDirector::BriefingText() const
{
	return FString::Printf(
		TEXT("WATCH BRIEFING %ds | Deliver 1 STANDARD + 1 BULKY to the green zone | Misha helps heavy (H) | Leak valve mid-watch (F)"),
		State.BriefingSeconds);
}

FString ACalmyVoyageDirector::OutcomeText() const
{
	if (State.bShiftComplete)
	{
		return FString::Printf(TEXT("WATCH COMPLETE | Paid $%d | Rating %d/3 | Host: R or bridge NEW WATCH"),
			State.FinalPayout, State.PerformanceStars);
	}
	if (State.bShiftFailed)
	{
		return TEXT("WATCH FAILED | Time expired | Host: R or bridge NEW WATCH");
	}
	return TEXT("Watch settled.");
}

void ACalmyVoyageDirector::OnRep_State()
{
}

void ACalmyVoyageDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyVoyageDirector, State);
}
