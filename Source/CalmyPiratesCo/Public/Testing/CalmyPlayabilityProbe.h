#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyPlayabilityProbe.generated.h"
class UPrimitiveComponent;

/**
 * Player-view playability survey of OceanFreighterR7 (-CalmyPlayabilityProbe=Solo, NullRHI).
 * With every door swung open it samples every floor of the live ship on a 25 cm ship-local grid (runtime collision,
 * player-sized capsule lifted by the step height), links neighbouring floor cells with capsule sweeps, marks unguarded
 * drops, and records which blocked links/cells would open without the S2 guardrails or the S3 details. For each
 * gameplay target (player starts, cargo, delivery zone, leak valve, fuel pump, crew helper, bridge terminal, doors) it
 * stores the floor cells a player can use it from (range + line of sight). Writes cells.csv + targets.json to
 * Saved/Verification/<CalmyReviewDir>; Scripts/analyze_ship_playability.py does reachability, islands and deck maps.
 * An audit: it never changes the map and reports findings instead of hiding them.
 */
UCLASS()
class CALMYPIRATESCO_API UCalmyPlayabilityProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DT) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyPlayabilityProbe, STATGROUP_Tickables); }

private:
	struct FCell
	{
		int32 IX = 0, IY = 0;
		float Z = 0;
		uint8 Stand = 0;     // 1 standable, 2 standable without guardrails, 4 standable without details
		uint8 Pass[3] = {0, 0, 0}; // per variant: bit d set when the link toward direction d (0..3 forward half) is open
		uint8 Drop = 0;      // bit d (0..7) set when walking that way falls more than 1.2 m
	};
	FVector ToWorld(const FVector& Local) const;
	FVector ToLocal(const FVector& World) const;
	FVector LocalDirToWorld(const FVector& Direction) const;
	bool CapsuleClear(const FVector& FloorLocal, uint8 Variant) const;
	bool SweepClear(const FVector& FromFloorLocal, const FVector& ToFloorLocal, uint8 Variant) const;
	void SampleColumn(int32 IX, int32 IY);
	void LinkCell(int32 Index);
	int32 NeighbourCell(const FCell& Cell, int32 DX, int32 DY) const;
	void CollectTargets();
	void AddTarget(const FString& Kind, const FString& Name, const FVector& Local, float Radius, bool bNeedSight, const AActor* Ignore);
	void WriteOutputs();
	void Finish(bool bOK, const FString& Detail);

	FString Dir;
	double Start = 0;
	int32 Stage = 0;
	float StageAge = 0;
	int32 Cursor = 0;
	int32 NX = 0, NY = 0;
	TArray<FCell> Cells;
	TMap<int32, TArray<int32>> Columns;
	TArray<FString> Targets;
	TArray<TWeakObjectPtr<AActor>> Dynamic;
	TWeakObjectPtr<UPrimitiveComponent> Guardrails;
	TWeakObjectPtr<UPrimitiveComponent> Details;
	int32 Doors = 0;
};
