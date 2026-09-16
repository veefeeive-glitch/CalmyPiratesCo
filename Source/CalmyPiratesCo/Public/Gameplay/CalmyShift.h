#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyShift.generated.h"

class ACalmyCargo;
class ACalmyCharacter;
class UBoxComponent;

UENUM(BlueprintType)
enum class ECalmyShiftPhase : uint8 { Loading, Complete, Failed, InvalidManifest };

/** One replicated snapshot: late joiners do not reconstruct a contract from event history. */
USTRUCT(BlueprintType)
struct FCalmyShiftSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Round = 1;
	UPROPERTY(BlueprintReadOnly) int32 Required = 0;
	UPROPERTY(BlueprintReadOnly) int32 Delivered = 0;
	UPROPERTY(BlueprintReadOnly) int32 Rescues = 0;
	UPROPERTY(BlueprintReadOnly) int32 DamagedDelivered = 0;
	UPROPERTY(BlueprintReadOnly) int32 SecondsRemaining = 300;
	UPROPERTY(BlueprintReadOnly) int32 FinalPayout = 0;
	UPROPERTY(BlueprintReadOnly) int32 PerformanceStars = 0;
	UPROPERTY(BlueprintReadOnly) ECalmyShiftPhase Phase = ECalmyShiftPhase::Loading;
};

/** Opt-in map actor. Delivery is determined by the server's physical cargo, never a client score RPC. */
UCLASS()
class CALMYPIRATESCO_API ACalmyShift : public AActor
{
	GENERATED_BODY()
public:
	ACalmyShift();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeManifest();
	bool TryAccept(ACalmyCargo* Cargo);
	bool TryRestart(ACalmyCharacter* Requester, int32 ExpectedRound);
	void RecordRescue();
	static ACalmyShift* Find(const UWorld* World);
	/** Tutorial lesson and a non-active voyage both stop the contract clock. */
	static bool FreezesContractTimer(bool bTutorialLesson, bool bVoyageFrozen) { return bTutorialLesson || bVoyageFrozen; }
	/** Only a voyage briefing/outcome blocks delivery; the tutorial still accepts its training crate. */
	static bool AcceptsDeliveries(bool bTutorialLesson, bool bVoyageFrozen) { return !bVoyageFrozen; }
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> DeliveryZone;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract", meta=(ClampMin="30", ClampMax="1800")) int32 ContractDurationSeconds = 300;
	UPROPERTY(Replicated, BlueprintReadOnly) FCalmyShiftSnapshot State;
private:
	friend class UCalmyPlaythroughProbe;
	bool IsEligible(const ACalmyCargo* Cargo) const;
	TMap<int32, TWeakObjectPtr<ACalmyCargo>> Manifest;
	TMap<int32, float> SettledTime;
	TSet<int32> AcceptedIds;
	float RemainingSecondsExact = 300.f;
};
