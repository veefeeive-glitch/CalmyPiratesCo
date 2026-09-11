#pragma once
#include "CoreMinimal.h"
#include "Core/GameplayPolicies.h"
#include "GameFramework/Actor.h"
#include "CalmyCargo.generated.h"

class ACalmyCharacter;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ECalmyCargoKind : uint8 { Standard, Bulky, Unstable, Fragile };

USTRUCT(BlueprintType)
struct FCalmyCargoCondition
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Stability = 100;
	UPROPERTY(BlueprintReadOnly) int32 Integrity = 100;
	UPROPERTY(BlueprintReadOnly) int32 DamageEvents = 0;
	UPROPERTY(BlueprintReadOnly) int32 Handoffs = 0;
	UPROPERTY(BlueprintReadOnly) int32 ForcedDrops = 0;
	UPROPERTY(BlueprintReadOnly) int32 TeamPace = 100;
};

/** State and holder replicate together; late joiners receive the current snapshot. */
USTRUCT(BlueprintType)
struct FCalmyCarrySnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) ECalmyCarryState State = ECalmyCarryState::Free;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<ACalmyCharacter> Holder = nullptr;
	UPROPERTY(BlueprintReadOnly) bool bDelivered = false;
};

UCLASS()
class CALMYPIRATESCO_API ACalmyCargo : public AActor
{
	GENERATED_BODY()
public:
	ACalmyCargo();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	bool TryGrab(ACalmyCharacter* Requester);
	bool Drop(ACalmyCharacter* Requester);
	bool Throw(ACalmyCharacter* Requester);
	bool TryHandoff(ACalmyCharacter* Requester, ACalmyCharacter* Receiver);
	bool HasNearbyHelper(const ACalmyCharacter* Holder) const;
	bool CanLift(const ACalmyCharacter* Requester) const;
	bool ApplyImpactSpeed(float Speed);
	void UpdateCondition(float DeltaSeconds);
	FString GetHandlingHint(const ACalmyCharacter* Viewer) const;
	float GetCarrySpeed() const { return Kind == ECalmyCargoKind::Bulky ? 260.f * FMath::Clamp(Condition.TeamPace / 100.f, .55f, 1.f) : 450.f; }
	float GetCarryRadius() const { return Kind == ECalmyCargoKind::Bulky ? 68.f : 38.f; }
	void ResetHome(bool bIncident = true);
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Kind, BlueprintReadOnly) ECalmyCargoKind Kind = ECalmyCargoKind::Standard;
	UPROPERTY(ReplicatedUsing=OnRep_Condition, BlueprintReadOnly) FCalmyCargoCondition Condition;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 ResetCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> CargoVisual;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly) int32 CargoId = 1;
	UPROPERTY(ReplicatedUsing=OnRep_Carry, BlueprintReadOnly) FCalmyCarrySnapshot Carry;

private:
	friend class ACalmyShift;
	ACalmyCharacter* FindNearbyHelper(const ACalmyCharacter* Holder) const;
	void MarkDelivered();
	UFUNCTION() void OnRep_Carry();
	UFUNCTION() void OnRep_Kind();
	UFUNCTION() void OnRep_Condition();
	void UpdateTint();
	void PlayDamageCue();
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Tint;
	UFUNCTION() void Hit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& HitResult);
	void PublishState();
	void MakeFree();
	FCalmyCarryReservationPolicy Policy;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;
	FVector PreviousVelocity = FVector::ZeroVector;
	float ThrowAge = 0;
	float RestTime = 0;
	float Stability = 100;
	float MissingHelperTime = 0;
	double NextDamageTime = -1;
	double NextHandoffTime = -1;
	int32 LastSeenDamageEvents = 0;
	bool bSeenCondition = false;
};
