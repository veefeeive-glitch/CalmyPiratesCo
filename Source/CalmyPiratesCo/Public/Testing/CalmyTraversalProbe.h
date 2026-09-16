#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyTraversalProbe.generated.h"
class ACharacter;

/**
 * In-game walkability audit of OceanFreighterR7 (-CalmyTraversalProbe=Solo). Replays the 166 historical traversal
 * routes forward and backward with real CharacterMovement capsules on the live, drifting ship with every door open,
 * then pushes a capsule outward from every route point in four ship directions to find unguarded edges (falls).
 * Writes Saved/Verification/<CalmyReviewDir>/traversal-report.json. An audit: failures are findings, not hidden.
 */
UCLASS()
class CALMYPIRATESCO_API UCalmyTraversalProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DT) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyTraversalProbe, STATGROUP_Tickables); }

private:
	struct FRoute { FString Name; TArray<FVector> Points; };
	struct FJob { int32 Route = -1; bool bReverse = false; bool bEdge = false; FVector EdgePoint = FVector::ZeroVector; };
	struct FWalker
	{
		TWeakObjectPtr<ACharacter> Body;
		int32 Job = -1;
		int32 Segment = 1;
		float SegmentAge = 0, SegmentLimit = 0, BestDistance = 0, SinceProgress = 0, FallingSeconds = 0;
		int32 EdgeDir = 0;
		float EdgeAge = 0;
		TArray<FString> EdgeFalls;
	};

	bool LoadRoutes();
	ACharacter* SpawnWalker(const FVector& ShipLocalFloor);
	FVector ToWorld(const FVector& ShipLocal) const;
	FVector ToLocal(const FVector& World) const;
	FVector ShipDirToWorld(const FVector& ShipLocalDir) const;
	void StartJob(FWalker& Walker);
	void StepWalk(FWalker& Walker, float DT);
	void StepEdge(FWalker& Walker, float DT);
	void EndJob(FWalker& Walker, bool bPass, const FString& Reason, const FString& Hit, const FVector& Local);
	FString BlockingHitName(ACharacter* Body, const FVector& WorldDir) const;
	void WriteReport() const;
	void Finish(bool bOK, const FString& Detail);

	FString Dir;
	double Start = 0;
	int32 Stage = 0;
	float StageAge = 0;
	int32 Parallel = 12;
	TArray<FRoute> Routes;
	TArray<FJob> Jobs;
	int32 NextJob = 0;
	TArray<FWalker> Walkers;
	TArray<FString> Results;
	int32 WalkTotal = 0, WalkPassed = 0, EdgeTotal = 0, EdgePointsWithFalls = 0;
};
