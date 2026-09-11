#include "Core/GameplayPolicies.h"

bool FCalmyCarryTransitions::IsValidTransition(ECalmyCarryState From, ECalmyCarryState To)
{
	if (From == To)
	{
		return true;
	}

	return
		(From == ECalmyCarryState::Free && To == ECalmyCarryState::Reserved) ||
		(From == ECalmyCarryState::Reserved && To == ECalmyCarryState::Held) ||
		(From == ECalmyCarryState::Reserved && To == ECalmyCarryState::Free) ||
		(From == ECalmyCarryState::Held && To == ECalmyCarryState::Thrown) ||
		(From == ECalmyCarryState::Held && To == ECalmyCarryState::Free) ||
		(From == ECalmyCarryState::Thrown && To == ECalmyCarryState::Free);
}

bool FCalmyPlayerLifecycleTransitions::IsValidTransition(
	ECalmyPlayerLifecycleState From,
	ECalmyPlayerLifecycleState To)
{
	if (From == To)
	{
		return true;
	}

	return
		(From == ECalmyPlayerLifecycleState::Controlled && To == ECalmyPlayerLifecycleState::KnockedDown) ||
		(From == ECalmyPlayerLifecycleState::KnockedDown && To == ECalmyPlayerLifecycleState::Recovering) ||
		(From == ECalmyPlayerLifecycleState::Recovering && To == ECalmyPlayerLifecycleState::Controlled);
}

bool FCalmyPlayerSpawnPolicy::TryReserveSpawn(uint64 ClientId)
{
	if (ServedClientIds.Contains(ClientId))
	{
		return false;
	}

	ServedClientIds.Add(ClientId);
	return true;
}

bool FCalmyPlayerSpawnPolicy::HasSpawned(uint64 ClientId) const
{
	return ServedClientIds.Contains(ClientId);
}

void FCalmyPlayerSpawnPolicy::ReleaseSpawn(uint64 ClientId)
{
	ServedClientIds.Remove(ClientId);
}

int32 FCalmyPlayerSpawnPolicy::GetSpawnedCount() const
{
	return ServedClientIds.Num();
}

FVector FCalmyMovementPolicy::ComputeServerPosition(
	const FVector& CurrentPosition,
	const FVector2D& MoveIntent,
	float DeltaTimeSeconds)
{
	const FVector2D ClampedIntent = MoveIntent.GetClampedToMaxSize(1.0f);
	if (MoveIntent.ContainsNaN() || !FMath::IsFinite(DeltaTimeSeconds))
	{
		return CurrentPosition;
	}
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTimeSeconds);
	const FVector Delta(
		ClampedIntent.X * MaxSpeedCentimetersPerSecond * SafeDeltaTime,
		ClampedIntent.Y * MaxSpeedCentimetersPerSecond * SafeDeltaTime,
		0.0f);

	FVector Next = CurrentPosition + Delta;
	Next.X = FMath::Clamp(Next.X, -DeckHalfExtentX, DeckHalfExtentX);
	Next.Y = FMath::Clamp(Next.Y, -DeckHalfExtentY, DeckHalfExtentY);
	Next.Z = CurrentPosition.Z;
	return Next;
}

bool FCalmyShipBoundsPolicy::IsPlayerOverboard(const FVector& Location)
{
	return Location.ContainsNaN() || !FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y) || !FMath::IsFinite(Location.Z) ||
		Location.Z < PlayerKeelZ || Location.X < SternX || Location.X > BowX || FMath::Abs(Location.Y) > SideY;
}

bool FCalmyShipBoundsPolicy::IsCargoOverboard(const FVector& Location)
{
	return Location.ContainsNaN() || !FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y) || !FMath::IsFinite(Location.Z) ||
		Location.Z < CargoKeelZ || Location.X < SternX || Location.X > BowX || FMath::Abs(Location.Y) > SideY;
}

bool FCalmyCarryReservationPolicy::TryReserve(uint64 RequesterId)
{
	if (State != ECalmyCarryState::Free ||
		!FCalmyCarryTransitions::IsValidTransition(State, ECalmyCarryState::Reserved))
	{
		return false;
	}

	State = ECalmyCarryState::Reserved;
	HolderClientId = RequesterId;
	bHasHolder = true;
	return true;
}

bool FCalmyCarryReservationPolicy::ConfirmHeld(uint64 RequesterId)
{
	if (State != ECalmyCarryState::Reserved || !bHasHolder || HolderClientId != RequesterId ||
		!FCalmyCarryTransitions::IsValidTransition(State, ECalmyCarryState::Held))
	{
		return false;
	}

	State = ECalmyCarryState::Held;
	return true;
}

bool FCalmyCarryReservationPolicy::TryGrab(uint64 RequesterId)
{
	return TryReserve(RequesterId) && ConfirmHeld(RequesterId);
}

bool FCalmyCarryReservationPolicy::TryRelease(uint64 RequesterId)
{
	if (State != ECalmyCarryState::Held || !bHasHolder || HolderClientId != RequesterId ||
		!FCalmyCarryTransitions::IsValidTransition(State, ECalmyCarryState::Free))
	{
		return false;
	}

	State = ECalmyCarryState::Free;
	bHasHolder = false;
	HolderClientId = 0;
	return true;
}

bool FCalmyCarryReservationPolicy::TryMarkThrown(uint64 RequesterId)
{
	if (State != ECalmyCarryState::Held || !bHasHolder || HolderClientId != RequesterId ||
		!FCalmyCarryTransitions::IsValidTransition(State, ECalmyCarryState::Thrown))
	{
		return false;
	}

	State = ECalmyCarryState::Thrown;
	return true;
}

bool FCalmyCarryReservationPolicy::ReleaseAfterThrow()
{
	if (State != ECalmyCarryState::Thrown ||
		!FCalmyCarryTransitions::IsValidTransition(State, ECalmyCarryState::Free))
	{
		return false;
	}

	State = ECalmyCarryState::Free;
	bHasHolder = false;
	HolderClientId = 0;
	return true;
}

bool FCalmyInteractionTargetingPolicy::IsLegalTarget(
	int32 TargetId,
	float DistanceCentimeters,
	float MaximumRangeCentimeters)
{
	return TargetId > 0 &&
		MaximumRangeCentimeters > 0.0f &&
		DistanceCentimeters >= 0.0f &&
		DistanceCentimeters <= MaximumRangeCentimeters;
}

bool FCalmySessionConvergencePolicy::HasConverged(
	int32 CurrentParticipantCount,
	int32 ExpectedParticipantCount,
	int32 ReadyParticipantCount)
{
	return ExpectedParticipantCount > 0 &&
		CurrentParticipantCount == ExpectedParticipantCount &&
		ReadyParticipantCount == ExpectedParticipantCount;
}

bool FCalmySessionConvergencePolicy::HaveAllCurrentParticipantsReported(
	const TArray<uint64>& CurrentParticipantIds,
	int32 ExpectedParticipantCount,
	const TSet<uint64>& ReportedParticipantIds)
{
	if (ExpectedParticipantCount <= 0 || CurrentParticipantIds.Num() != ExpectedParticipantCount)
	{
		return false;
	}

	for (uint64 ParticipantId : CurrentParticipantIds)
	{
		if (!ReportedParticipantIds.Contains(ParticipantId))
		{
			return false;
		}
	}

	return true;
}

float FCalmyTechnicalSoakPolicy::ResolveDurationSeconds(int32 RequestedSeconds)
{
	return RequestedSeconds > 0 ? static_cast<float>(RequestedSeconds) : DefaultDurationSeconds;
}

bool FCalmyTechnicalSoakPolicy::IsComplete(
	bool bStableThroughout,
	float SecondsCompleted,
	float RequiredSeconds)
{
	return bStableThroughout && SecondsCompleted >= RequiredSeconds;
}

namespace
{
	uint32 MixEmergencySeed(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7feb352dU;
		Value ^= Value >> 15;
		Value *= 0x846ca68bU;
		Value ^= Value >> 16;
		return Value;
	}

	int32 SampleEmergencySeconds(int32 Seed, int32 Round, int32 Serial, int32 Minimum, int32 Maximum)
	{
		const uint32 Key = static_cast<uint32>(FCalmyEmergencyDirectorPolicy::NormalizeSeed(Seed)) ^
			(static_cast<uint32>(FMath::Max(1, Round)) * 0x9e3779b9U) ^
			(static_cast<uint32>(FMath::Max(0, Serial)) * 0x85ebca6bU);
		return Minimum + static_cast<int32>(MixEmergencySeed(Key) % static_cast<uint32>(Maximum - Minimum + 1));
	}
}

int32 FCalmyEmergencyDirectorPolicy::NormalizeSeed(int32 Seed)
{
	const uint32 Positive = static_cast<uint32>(Seed) & 0x7fffffffU;
	return static_cast<int32>(Positive == 0 ? 1U : Positive);
}

int32 FCalmyEmergencyDirectorPolicy::InitialDelaySeconds(int32 Seed, int32 Round)
{
	return SampleEmergencySeconds(Seed, Round, 0, InitialDelayMinSeconds, InitialDelayMaxSeconds);
}

int32 FCalmyEmergencyDirectorPolicy::ResolveCooldownSeconds(int32 Seed, int32 Round, int32 EventSerial)
{
	return SampleEmergencySeconds(Seed, Round, FMath::Max(1, EventSerial), CooldownMinSeconds, CooldownMaxSeconds);
}

bool FCalmyEmergencyDirectorPolicy::CanStartEvent(int32 EventsStarted, bool bEventActive, float RemainingSeconds)
{
	return EventsStarted >= 0 && EventsStarted < MaxEventsPerRound && !bEventActive &&
		FMath::IsFinite(RemainingSeconds) && RemainingSeconds <= 0.f;
}

int32 FCalmyShiftEconomyPolicy::CalculatePayout(int32 SecondsRemaining, int32 Rescues, int32 DamagedDelivered)
{
	const int32 SafeTime = FMath::Clamp(SecondsRemaining, 0, DefaultDurationSeconds);
	const int32 Gross = BasePayout + SafeTime * 4 - FMath::Max(0, Rescues) * 125 - FMath::Max(0, DamagedDelivered) * 300;
	return FMath::Max(MinimumSuccessfulPayout, Gross);
}

int32 FCalmyShiftEconomyPolicy::CalculateStars(int32 SecondsRemaining, int32 Rescues, int32 DamagedDelivered, int32 Payout)
{
	if (Rescues <= 0 && DamagedDelivered <= 0 && SecondsRemaining >= 120) return 3;
	if (Payout >= BasePayout && DamagedDelivered <= 1) return 2;
	return 1;
}

bool FCalmyCommandGate::Accept(uint32 Sequence, double ServerTimeSeconds)
{
	if (!FMath::IsFinite(ServerTimeSeconds) || ServerTimeSeconds < 0.0 || Sequence == 0 || Sequence <= LastSequence)
	{
		return false;
	}
	LastSequence = Sequence;
	if (LastAcceptedTime >= 0.0 && ServerTimeSeconds - LastAcceptedTime < 0.12)
	{
		return false;
	}
	LastAcceptedTime = ServerTimeSeconds;
	return true;
}
