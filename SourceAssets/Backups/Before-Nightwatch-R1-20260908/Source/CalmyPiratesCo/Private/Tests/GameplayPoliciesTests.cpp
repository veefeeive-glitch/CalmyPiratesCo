#include "Core/GameplayPolicies.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmySpawnPolicyTest,
	"CalmyPiratesCo.Core.SpawnPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmySpawnPolicyTest::RunTest(const FString& Parameters)
{
	FCalmyPlayerSpawnPolicy Policy;
	TestTrue(TEXT("First reservation is accepted"), Policy.TryReserveSpawn(7));
	TestFalse(TEXT("Duplicate reservation is rejected"), Policy.TryReserveSpawn(7));
	TestEqual(TEXT("Exactly one participant is reserved"), Policy.GetSpawnedCount(), 1);
	Policy.ReleaseSpawn(7);
	TestTrue(TEXT("Reconnect can reserve after release"), Policy.TryReserveSpawn(7));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyMovementPolicyTest,
	"CalmyPiratesCo.Core.MovementPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyMovementPolicyTest::RunTest(const FString& Parameters)
{
	const FVector Start(0.0f, 0.0f, 125.0f);
	const FVector Forward = FCalmyMovementPolicy::ComputeServerPosition(Start, FVector2D(0.0f, 1.0f), 1.0f);
	TestTrue(TEXT("Forward intent moves at maximum speed"),
		FMath::IsNearlyEqual(Forward.Y, FCalmyMovementPolicy::MaxSpeedCentimetersPerSecond));
	TestTrue(TEXT("Vertical position is preserved"), FMath::IsNearlyEqual(Forward.Z, Start.Z));

	const FVector Diagonal = FCalmyMovementPolicy::ComputeServerPosition(FVector::ZeroVector, FVector2D(1.0f, 1.0f), 1.0f);
	TestTrue(TEXT("Diagonal input is clamped to unit magnitude"),
		FMath::IsNearlyEqual(FVector2D(Diagonal.X, Diagonal.Y).Size(), FCalmyMovementPolicy::MaxSpeedCentimetersPerSecond, 0.01f));

	const FVector Bounded = FCalmyMovementPolicy::ComputeServerPosition(FVector::ZeroVector, FVector2D(1.0f, 0.0f), 1000.0f);
	TestTrue(TEXT("Movement is clamped to deck bounds"),
		FMath::IsNearlyEqual(Bounded.X, FCalmyMovementPolicy::DeckHalfExtentX));
	TestFalse(TEXT("A crew member inside the stern superstructure stays aboard"),
		FCalmyShipBoundsPolicy::IsPlayerOverboard(FVector(-1650, 600, 90)));
	TestTrue(TEXT("Crossing beyond the stern is overboard"),
		FCalmyShipBoundsPolicy::IsPlayerOverboard(FVector(-1800, 0, 90)));
	TestFalse(TEXT("Lower cargo deck remains inside the hull"),
		FCalmyShipBoundsPolicy::IsCargoOverboard(FVector(0, 0, -300)));
	TestTrue(TEXT("Falling below the keel is overboard"),
		FCalmyShipBoundsPolicy::IsCargoOverboard(FVector(0, 0, -500)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyCarryPolicyTest,
	"CalmyPiratesCo.Core.CarryPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyCarryPolicyTest::RunTest(const FString& Parameters)
{
	FCalmyCarryReservationPolicy Policy;
	TestTrue(TEXT("First grab succeeds"), Policy.TryGrab(11));
	TestFalse(TEXT("Contending grab is rejected"), Policy.TryGrab(22));
	TestEqual(TEXT("First requester remains holder"), Policy.GetHolderClientId(), static_cast<uint64>(11));
	TestFalse(TEXT("Non-holder cannot release"), Policy.TryRelease(22));
	TestTrue(TEXT("Holder can mark cargo thrown"), Policy.TryMarkThrown(11));
	TestTrue(TEXT("Thrown cargo can return to free"), Policy.ReleaseAfterThrow());
	TestEqual(TEXT("Cargo returns to free"), Policy.GetState(), ECalmyCarryState::Free);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyLifecyclePolicyTest,
	"CalmyPiratesCo.Core.PlayerLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyLifecyclePolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Controlled to knocked down is valid"),
		FCalmyPlayerLifecycleTransitions::IsValidTransition(
			ECalmyPlayerLifecycleState::Controlled,
			ECalmyPlayerLifecycleState::KnockedDown));
	TestTrue(TEXT("Knocked down to recovering is valid"),
		FCalmyPlayerLifecycleTransitions::IsValidTransition(
			ECalmyPlayerLifecycleState::KnockedDown,
			ECalmyPlayerLifecycleState::Recovering));
	TestFalse(TEXT("Knocked down cannot skip recovery"),
		FCalmyPlayerLifecycleTransitions::IsValidTransition(
			ECalmyPlayerLifecycleState::KnockedDown,
			ECalmyPlayerLifecycleState::Controlled));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmySessionPolicyTest,
	"CalmyPiratesCo.Core.SessionConvergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmySessionPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Two current ready peers converge"),
		FCalmySessionConvergencePolicy::HasConverged(2, 2, 2));
	TestFalse(TEXT("Missing ready peer does not converge"),
		FCalmySessionConvergencePolicy::HasConverged(2, 2, 1));

	const TArray<uint64> CurrentIds{0, 2};
	const TSet<uint64> StaleReporters{0, 1};
	const TSet<uint64> CurrentReporters{0, 1, 2};
	TestFalse(TEXT("A stale disconnected reporter is rejected"),
		FCalmySessionConvergencePolicy::HaveAllCurrentParticipantsReported(CurrentIds, 2, StaleReporters));
	TestTrue(TEXT("Every current reporter is accepted despite stale history"),
		FCalmySessionConvergencePolicy::HaveAllCurrentParticipantsReported(CurrentIds, 2, CurrentReporters));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyTargetingAndSoakPolicyTest,
	"CalmyPiratesCo.Core.TargetingAndSoak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyTargetingAndSoakPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A legal target in range is accepted"),
		FCalmyInteractionTargetingPolicy::IsLegalTarget(1, 200.0f));
	TestFalse(TEXT("A target beyond 2.2 metres is rejected"),
		FCalmyInteractionTargetingPolicy::IsLegalTarget(1, 221.0f));
	TestTrue(TEXT("A stable ten-minute soak is complete"),
		FCalmyTechnicalSoakPolicy::IsComplete(true, 600.0f, 600.0f));
	TestFalse(TEXT("An unstable soak is incomplete"),
		FCalmyTechnicalSoakPolicy::IsComplete(false, 600.0f, 600.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyEmergencyDirectorSoakTest,
	"CalmyPiratesCo.Gameplay.EmergencyDirectorSeededSoak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyEmergencyDirectorSoakTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Zero seed is normalized"), FCalmyEmergencyDirectorPolicy::NormalizeSeed(0), 1);
	TestEqual(TEXT("Initial schedule is reproducible"),
		FCalmyEmergencyDirectorPolicy::InitialDelaySeconds(203, 7),
		FCalmyEmergencyDirectorPolicy::InitialDelaySeconds(203, 7));
	TestEqual(TEXT("Cooldown schedule is reproducible"),
		FCalmyEmergencyDirectorPolicy::ResolveCooldownSeconds(203, 7, 1),
		FCalmyEmergencyDirectorPolicy::ResolveCooldownSeconds(203, 7, 1));
	TestFalse(TEXT("An active emergency rejects stacking"),
		FCalmyEmergencyDirectorPolicy::CanStartEvent(0, true, 0.f));
	TestFalse(TEXT("The per-round cap rejects another emergency"),
		FCalmyEmergencyDirectorPolicy::CanStartEvent(FCalmyEmergencyDirectorPolicy::MaxEventsPerRound, false, 0.f));

	int32 TotalStarts = 0;
	int32 PeakConcurrent = 0;
	int32 ObservedInitialMin = MAX_int32;
	int32 ObservedInitialMax = 0;
	int32 ObservedCooldownMin = MAX_int32;
	int32 ObservedCooldownMax = 0;
	// 1,024 deterministic ten-minute schedules = 170.67 simulated hours.
	for (int32 Seed = 1; Seed <= 64; ++Seed)
	{
		for (int32 Round = 1; Round <= 16; ++Round)
		{
			int32 Starts = 0;
			int32 Concurrent = 0;
			int32 ActiveRemaining = 0;
			int32 Remaining = FCalmyEmergencyDirectorPolicy::InitialDelaySeconds(Seed, Round);
			ObservedInitialMin = FMath::Min(ObservedInitialMin, Remaining);
			ObservedInitialMax = FMath::Max(ObservedInitialMax, Remaining);
			for (int32 Second = 0; Second < 600; ++Second)
			{
				if (Concurrent > 0)
				{
					if (--ActiveRemaining == 0)
					{
						Concurrent = 0;
						if (Starts < FCalmyEmergencyDirectorPolicy::MaxEventsPerRound)
						{
							Remaining = FCalmyEmergencyDirectorPolicy::ResolveCooldownSeconds(Seed, Round, Starts);
							ObservedCooldownMin = FMath::Min(ObservedCooldownMin, Remaining);
							ObservedCooldownMax = FMath::Max(ObservedCooldownMax, Remaining);
						}
					}
					continue;
				}
				--Remaining;
				if (FCalmyEmergencyDirectorPolicy::CanStartEvent(Starts, false, static_cast<float>(Remaining)))
				{
					++Starts;
					++Concurrent;
					ActiveRemaining = 20;
					PeakConcurrent = FMath::Max(PeakConcurrent, Concurrent);
				}
			}
			TestEqual(TEXT("Each ten-minute seeded schedule reaches the two-event cap"), Starts,
				FCalmyEmergencyDirectorPolicy::MaxEventsPerRound);
			TotalStarts += Starts;
		}
	}
	TestEqual(TEXT("Seeded soak never stacks emergencies"), PeakConcurrent, 1);
	TestEqual(TEXT("Seeded soak start count is bounded"), TotalStarts, 2048);
	TestTrue(TEXT("Initial delays stay in the declared range"),
		ObservedInitialMin >= FCalmyEmergencyDirectorPolicy::InitialDelayMinSeconds &&
		ObservedInitialMax <= FCalmyEmergencyDirectorPolicy::InitialDelayMaxSeconds);
	TestTrue(TEXT("Cooldowns stay in the declared range"),
		ObservedCooldownMin >= FCalmyEmergencyDirectorPolicy::CooldownMinSeconds &&
		ObservedCooldownMax <= FCalmyEmergencyDirectorPolicy::CooldownMaxSeconds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCalmyShiftEconomyPolicyTest,
	"CalmyPiratesCo.Gameplay.ShiftEconomy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyShiftEconomyPolicyTest::RunTest(const FString& Parameters)
{
	const int32 Perfect = FCalmyShiftEconomyPolicy::CalculatePayout(180, 0, 0);
	TestEqual(TEXT("Fast clean shift earns deterministic payout"), Perfect, 1920);
	TestEqual(TEXT("Fast clean shift earns three stars"), FCalmyShiftEconomyPolicy::CalculateStars(180, 0, 0, Perfect), 3);

	const int32 Messy = FCalmyShiftEconomyPolicy::CalculatePayout(20, 4, 2);
	TestEqual(TEXT("Penalties cannot erase successful minimum pay"), Messy, 250);
	TestEqual(TEXT("Messy shift earns one star"), FCalmyShiftEconomyPolicy::CalculateStars(20, 4, 2, Messy), 1);
	TestEqual(TEXT("Client-like invalid values are clamped"), FCalmyShiftEconomyPolicy::CalculatePayout(9999, -5, -2), 2400);
	return true;
}

#endif
