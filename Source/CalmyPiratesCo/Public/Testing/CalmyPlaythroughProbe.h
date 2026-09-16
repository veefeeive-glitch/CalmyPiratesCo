#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyPlaythroughProbe.generated.h"
class ACalmyCharacter;
class ACalmyCargo;

/**
 * Gameplay confirmation of the R7 watch loop with the real player pawn and its real commands
 * (-CalmyPlaythroughProbe=<legs.json>, from Scripts/playthrough_legs.py; run with a real RHI for frames).
 * Begin watch -> walk down to hold 1 -> hold the bilge valve until the leak is sealed -> pick up the standard crate ->
 * carry it up the hold stairs to the delivery zone and set it down -> walk to the bunker station and refuel.
 * Every leg is walked with MoveIntent along surveyed floor; a leg that stalls for 8 s fails with where and why.
 * Photos land in Saved/Verification/<CalmyReviewDir>/NN-<step>.png; the log line CALMY_PLAYTHROUGH PASS/FAIL is the verdict.
 */
UCLASS()
class CALMYPIRATESCO_API UCalmyPlaythroughProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DT) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyPlaythroughProbe, STATGROUP_Tickables); }

private:
	struct FLeg { FString Name; TArray<FVector> Points; };
	bool LoadLegs();
	/** Walk the pawn along a leg; true when the last point is reached. */
	bool WalkLeg(ACalmyCharacter* Pawn, const FLeg& Leg, float DT);
	void Face(ACalmyCharacter* Pawn, const FVector& WorldTarget) const;
	void Photo(const FString& Name);
	void Step(const FString& Name);
	void Finish(bool bOK, const FString& Detail);
	FVector ToWorld(const FVector& Local) const;
	FVector ToLocal(const FVector& World) const;

	FString Dir;
	TArray<FLeg> Legs;
	double Start = 0;
	int32 Stage = 0, Waypoint = 0, Shots = 0;
	float StageAge = 0, SinceProgress = 0, BestDistance = BIG_NUMBER, PulseAge = 0;
	uint32 Sequence = 0;
	TWeakObjectPtr<ACalmyCargo> Crate;
	TArray<FString> Timeline;
	int32 DeliveredBefore = 0;
	bool bSawLeak = false;
	/** Watertight doors the walk had to open on the way, and the cooldown between taps. */
	int32 DoorsOpened = 0;
	float DoorTapAge = 0;
};
