#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyLeak.generated.h"

class ACalmyCharacter;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UPostProcessComponent;

UENUM(BlueprintType)
enum class ECalmyLeakPhase : uint8 { Idle, Warning, Leaking, Draining, Resolved, Suspended };

/** Current state, never an event replay. One repairer and a bounded, non-stacking schedule per contract generation. */
USTRUCT(BlueprintType)
struct FCalmyLeakSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Round = 1;
	UPROPERTY(BlueprintReadOnly) ECalmyLeakPhase Phase = ECalmyLeakPhase::Idle;
	UPROPERTY(BlueprintReadOnly) int32 Water = 0;
	UPROPERTY(BlueprintReadOnly) int32 Repair = 0;
	UPROPERTY(BlueprintReadOnly) int32 WarningSeconds = 0;
	/** Host-selected session seed and current schedule facts for late join; clients never advance them. */
	UPROPERTY(BlueprintReadOnly) int32 DirectorSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 EventSerial = 0;
	UPROPERTY(BlueprintReadOnly) int32 NextEventSeconds = 0;
	UPROPERTY(BlueprintReadOnly) int32 CooldownSeconds = 0;
	/** Server-derived consequence. Clients only present the replicated value. */
	UPROPERTY(BlueprintReadOnly) bool bPowerDegraded = false;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<ACalmyCharacter> Repairer;
};

/** Opt-in server-owned leak + valve. No client-supplied progress, clock, water or success. */
UCLASS()
class CALMYPIRATESCO_API ACalmyLeak : public AActor
{
	GENERATED_BODY()
public:
	ACalmyLeak();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	static ACalmyLeak* Find(const UWorld* World);
	bool CanRepair(const ACalmyCharacter* Pawn) const;
	bool TryRepairPulse(ACalmyCharacter* Pawn, int32 ExpectedRound);
	void StopRepair(ACalmyCharacter* Pawn);
	void ResetForRound(int32 Round);
	float GetDeckSpeedScale() const { return State.Water >= 50 ? .7f : 1.f; }
	FString GetStatusText() const;
	UPROPERTY(ReplicatedUsing=OnRep_State, BlueprintReadOnly) FCalmyLeakSnapshot State;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Valve;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> EmergencyLight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> TaskLight;
private:
	UFUNCTION() void OnRep_State();
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WaterPatch;
	UPROPERTY() TObjectPtr<UPostProcessComponent> EmergencyGrade;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ValveMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;
	float PhaseAge = 0;
	float WaterAmount = 0;
	float RepairAmount = 0;
	float ScheduleRemaining = 0;
	float CooldownRemaining = 0;
	double RepairLeaseEnd = 0;
	int32 BaseDirectorSeed = 0;
};
