#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyVoyageDirector.generated.h"

class ACalmyCharacter;

UENUM(BlueprintType)
enum class ECalmyVoyagePhase : uint8
{
	Briefing,
	Active,
	Outcome
};

/** Late-join snapshot for the G3 ship watch cycle. Server advances; clients present only. */
USTRUCT(BlueprintType)
struct FCalmyVoyageSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Round = 1;
	UPROPERTY(BlueprintReadOnly) ECalmyVoyagePhase Phase = ECalmyVoyagePhase::Briefing;
	UPROPERTY(BlueprintReadOnly) int32 BriefingSeconds = 12;
	UPROPERTY(BlueprintReadOnly) int32 RequiredStandard = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredBulky = 1;
	UPROPERTY(BlueprintReadOnly) bool bLeakArmed = false;
	UPROPERTY(BlueprintReadOnly) bool bShiftComplete = false;
	UPROPERTY(BlueprintReadOnly) bool bShiftFailed = false;
	UPROPERTY(BlueprintReadOnly) int32 FinalPayout = 0;
	UPROPERTY(BlueprintReadOnly) int32 PerformanceStars = 0;
};

/**
 * OceanFreighterR7-only glue: briefing → curated shift + leak complication → outcome → host restart.
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
	/** True while the ship contract timer must stay frozen (briefing or settled outcome). */
	static bool ShouldFreezeShiftTimer(const UWorld* World);
	/** When present, Shift builds a curated Standard+Bulky manifest instead of every crate. */
	static bool UsesCuratedManifest(const UWorld* World);
	bool TryBeginWatch(ACalmyCharacter* Requester);
	bool TryRestartWatch(ACalmyCharacter* Requester, int32 ExpectedRound);
	void NotifyShiftRestarted(int32 NewRound);
	FString BriefingText() const;
	FString OutcomeText() const;
	UPROPERTY(ReplicatedUsing=OnRep_State, BlueprintReadOnly) FCalmyVoyageSnapshot State;
private:
	UFUNCTION() void OnRep_State();
	void EnsureShipFixtures();
	void EnterBriefing(int32 Round);
	void EnterActive();
	void SyncFromShift();
	float BriefingRemaining = 12.f;
};
