#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/CalmyVoyageContract.h"
#include "CalmyVoyageDirector.generated.h"

class ACalmyCharacter;

UENUM(BlueprintType)
enum class ECalmyVoyagePhase : uint8
{
	Briefing,
	Active,
	Outcome
};

/** Late-join snapshot for the G3/G4 ship watch cycle. Server advances; clients present only. */
USTRUCT(BlueprintType)
struct FCalmyVoyageSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Round = 1;
	UPROPERTY(BlueprintReadOnly) ECalmyVoyagePhase Phase = ECalmyVoyagePhase::Briefing;
	UPROPERTY(BlueprintReadOnly) int32 BriefingSeconds = 12;
	UPROPERTY(BlueprintReadOnly) int32 RequiredStandard = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredBulky = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredFragile = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredUnstable = 1;
	UPROPERTY(BlueprintReadOnly) bool bLeakArmed = false;
	UPROPERTY(BlueprintReadOnly) bool bFuelStationArmed = false;
	UPROPERTY(BlueprintReadOnly) bool bShiftComplete = false;
	UPROPERTY(BlueprintReadOnly) bool bShiftFailed = false;
	UPROPERTY(BlueprintReadOnly) bool bFuelStarved = false;
	UPROPERTY(BlueprintReadOnly) int32 FinalPayout = 0;
	UPROPERTY(BlueprintReadOnly) int32 PerformanceStars = 0;
	UPROPERTY(BlueprintReadOnly) int32 FuelPercent = 100;
	UPROPERTY(BlueprintReadOnly) int32 StormPercent = 0;
	UPROPERTY(BlueprintReadOnly) int32 ActiveSeconds = 0;
	/** Contract watch length, replicated so clients show the real duration. */
	UPROPERTY(BlueprintReadOnly) int32 WatchSeconds = 1080;
	UPROPERTY(BlueprintReadOnly) FString ContractId;
	UPROPERTY(BlueprintReadOnly) FString ContractName;
};

/**
 * OceanFreighterR7-only glue: port contract → briefing → curated watch + storm/leak/fuel → outcome → return to port.
 * Labs without this actor keep their existing Shift/Leak behaviour.
 */
UCLASS()
class CALMYPIRATESCO_API ACalmyVoyageDirector : public AActor
{
	GENERATED_BODY()
public:
	ACalmyVoyageDirector();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	static ACalmyVoyageDirector* Find(const UWorld* World);
	static bool IsShipMap(const UWorld* World);
	/** R7 ship-local (rest pose, UE cm) -> current world position on the drifting hull. */
	static FVector ShipLocalToWorld(const UWorld* World, const FVector& Local);
	/** Where hold 1 flood water collects (bilge well on the tank top), ship-local. */
	static FVector LeakPoolLocal();
	/** Reviewed R7 task anchors, ship-local (rest pose, UE cm): bilge valve on the hold 1 aft bulkhead, the S3 bunker
	 * station on the weather-deck side lane, and the delivery zone. Checked by CalmyPiratesCo.Ship.ActivityAnchors. */
	static FVector LeakValveLocal();
	static FVector FuelStationLocal();
	static FVector DeliveryLocal();
	/** True while the ship contract timer must stay frozen (briefing or settled outcome). */
	static bool ShouldFreezeShiftTimer(const UWorld* World);
	/** When present, Shift builds a curated manifest instead of every crate. */
	static bool UsesCuratedManifest(const UWorld* World);
	bool TryBeginWatch(ACalmyCharacter* Requester);
	bool TryRestartWatch(ACalmyCharacter* Requester, int32 ExpectedRound);
	bool TryReturnToPort(ACalmyCharacter* Requester);
	void NotifyShiftRestarted(int32 NewRound);
	void ApplyFuelPulse(float AmountPercent);
	FString BriefingText() const;
	FString OutcomeText() const;
	const FCalmyVoyageContract& GetContract() const { return ActiveContract; }
	UPROPERTY(EditAnywhere, Category="Voyage", meta=(ClampMin="300", ClampMax="1800")) int32 WatchDurationSeconds = 1080;
	UPROPERTY(ReplicatedUsing=OnRep_State, BlueprintReadOnly) FCalmyVoyageSnapshot State;
private:
	UFUNCTION() void OnRep_State();
	void EnsureShipFixtures();
	void EnsureHoldRouteAids();
	void ApplyContract(const FCalmyVoyageContract& Contract);
	void EnterBriefing(int32 Round);
	void EnterActive();
	void SyncFromShift();
	void UpdateSeaState(float ActiveAge);
	void FailFuelStarvation();
	void PresentAudio();
	float BriefingRemaining = 12.f;
	float ActiveAge = 0.f;
	float FuelExact = 100.f;
	FCalmyVoyageContract ActiveContract;
	bool bRouteAidsSpawned = false;
	uint8 PresentedPhase = 255;
	int32 PresentedStorm = 0;
	bool bTravelRequested = false;
};
