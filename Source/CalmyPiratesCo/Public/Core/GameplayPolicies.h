#pragma once

#include "CoreMinimal.h"
#include "GameplayPolicies.generated.h"

UENUM(BlueprintType)
enum class ECalmyCarryState : uint8
{
	Free,
	Reserved,
	Held,
	Thrown
};

UENUM(BlueprintType)
enum class ECalmyPlayerLifecycleState : uint8
{
	Controlled,
	KnockedDown,
	Recovering
};

class CALMYPIRATESCO_API FCalmyCarryTransitions
{
public:
	static bool IsValidTransition(ECalmyCarryState From, ECalmyCarryState To);
};

class CALMYPIRATESCO_API FCalmyPlayerLifecycleTransitions
{
public:
	static bool IsValidTransition(ECalmyPlayerLifecycleState From, ECalmyPlayerLifecycleState To);
};

/** Pure host-side admission policy. A network participant may own at most one live spawn reservation. */
class CALMYPIRATESCO_API FCalmyPlayerSpawnPolicy
{
public:
	bool TryReserveSpawn(uint64 ClientId);
	bool HasSpawned(uint64 ClientId) const;
	void ReleaseSpawn(uint64 ClientId);
	int32 GetSpawnedCount() const;

private:
	TSet<uint64> ServedClientIds;
};

/** Pure server movement calculation. Unreal units are centimetres. */
class CALMYPIRATESCO_API FCalmyMovementPolicy
{
public:
	static constexpr float MaxSpeedCentimetersPerSecond = 450.0f;
	static constexpr float DeckHalfExtentX = 850.0f;
	static constexpr float DeckHalfExtentY = 450.0f;

	static FVector ComputeServerPosition(
		const FVector& CurrentPosition,
		const FVector2D& MoveIntent,
		float DeltaTimeSeconds);
};

/** Authoritative playable footprint shared by player and cargo rescue checks. */
class CALMYPIRATESCO_API FCalmyShipBoundsPolicy
{
public:
	static constexpr float SternX = -1780.f;
	static constexpr float BowX = 1900.f;
	static constexpr float SideY = 1000.f;
	static constexpr float PlayerKeelZ = -430.f;
	static constexpr float CargoKeelZ = -450.f;
	static bool IsPlayerOverboard(const FVector& Location);
	static bool IsCargoOverboard(const FVector& Location);
};

/** Single-holder carry reservation owned exclusively by the host/server. */
class CALMYPIRATESCO_API FCalmyCarryReservationPolicy
{
public:
	bool TryReserve(uint64 RequesterId);
	bool ConfirmHeld(uint64 RequesterId);
	bool TryGrab(uint64 RequesterId);
	bool TryRelease(uint64 RequesterId);
	bool TryMarkThrown(uint64 RequesterId);
	bool ReleaseAfterThrow();

	ECalmyCarryState GetState() const { return State; }
	bool HasHolder() const { return bHasHolder; }
	uint64 GetHolderClientId() const { return HolderClientId; }

private:
	ECalmyCarryState State = ECalmyCarryState::Free;
	uint64 HolderClientId = 0;
	bool bHasHolder = false;
};

class CALMYPIRATESCO_API FCalmyInteractionTargetingPolicy
{
public:
	static constexpr float DefaultMaximumRangeCentimeters = 220.0f;

	static bool IsLegalTarget(
		int32 TargetId,
		float DistanceCentimeters,
		float MaximumRangeCentimeters = DefaultMaximumRangeCentimeters);
};

class CALMYPIRATESCO_API FCalmySessionConvergencePolicy
{
public:
	static bool HasConverged(
		int32 CurrentParticipantCount,
		int32 ExpectedParticipantCount,
		int32 ReadyParticipantCount);

	static bool HaveAllCurrentParticipantsReported(
		const TArray<uint64>& CurrentParticipantIds,
		int32 ExpectedParticipantCount,
		const TSet<uint64>& ReportedParticipantIds);
};

class CALMYPIRATESCO_API FCalmyTechnicalSoakPolicy
{
public:
	static constexpr float DefaultDurationSeconds = 600.0f;

	static float ResolveDurationSeconds(int32 RequestedSeconds);
	static bool IsComplete(bool bStableThroughout, float SecondsCompleted, float RequiredSeconds);
};

/** Pure deterministic timing policy. The host owns the seed and publishes only current schedule facts. */
class CALMYPIRATESCO_API FCalmyEmergencyDirectorPolicy
{
public:
	static constexpr int32 MaxEventsPerRound = 2;
	static constexpr int32 InitialDelayMinSeconds = 18;
	static constexpr int32 InitialDelayMaxSeconds = 24;
	static constexpr int32 CooldownMinSeconds = 45;
	static constexpr int32 CooldownMaxSeconds = 60;

	static int32 NormalizeSeed(int32 Seed);
	static int32 InitialDelaySeconds(int32 Seed, int32 Round);
	static int32 ResolveCooldownSeconds(int32 Seed, int32 Round, int32 EventSerial);
	static bool CanStartEvent(int32 EventsStarted, bool bEventActive, float RemainingSeconds);
};

/** Pure server-side contract economy. Clients display the replicated result but never calculate rewards. */
class CALMYPIRATESCO_API FCalmyShiftEconomyPolicy
{
public:
	static constexpr int32 DefaultDurationSeconds = 300;
	static constexpr int32 BasePayout = 1200;
	static constexpr int32 MinimumSuccessfulPayout = 250;

	static int32 CalculatePayout(int32 SecondsRemaining, int32 Rescues, int32 DamagedDelivered);
	static int32 CalculateStars(int32 SecondsRemaining, int32 Rescues, int32 DamagedDelivered, int32 Payout);
};

/** Server melee damage. Crew punching crew is a harmless shove (hit flash only): no knockout, rescue cost or teleport.
 *  Misha still answers provocation, and crew punches still provoke Misha. */
class CALMYPIRATESCO_API FCalmyMeleePolicy
{
public:
	static float DamageFor(bool bAttackerIsHelper, bool bVictimIsHelper);
};

/** Per-pawn server command gate. Consumes rejected sequences too, so retries cannot toggle state. */
class CALMYPIRATESCO_API FCalmyCommandGate
{
public:
	bool Accept(uint32 Sequence, double ServerTimeSeconds);
private:
	uint32 LastSequence = 0;
	double LastAcceptedTime = -1.0;
};
