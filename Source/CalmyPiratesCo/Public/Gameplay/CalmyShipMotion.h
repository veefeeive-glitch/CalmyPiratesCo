#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyShipMotion.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class ACalmyOcean;
/**
 * G2 ship ride: W1 field + SeaStrength/storm boost, helm intent, boarding ramp.
 * Full Chaos R7 mesh is out of scope; see ACalmyHullProxy for physics test hull.
 */
UCLASS()
class CALMYPIRATESCO_API ACalmyShipMotion : public AActor
{
	GENERATED_BODY()
public:
	ACalmyShipMotion();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	static ACalmyShipMotion* Find(const UWorld* World);
	/** Attaches a gameplay fixture placed in ship-local coordinates so heave, heel and drift carry it. No-op without a ship. */
	static void AttachRider(AActor* Rider);
	const FTransform& GetRestTransform() const { return RestTransform; }
	FVector GetRestDryHullOrigin() const { return RestDryHullOrigin; }
	bool HasRestPose() const { return bCapturedRest; }
	float GetHelmThrottle() const { return HelmThrottle; }
	float GetHelmRudder() const { return HelmRudder; }
	float GetAccumulatedYawDeg() const { return AccumulatedYawDeg; }
	UFUNCTION(BlueprintCallable, Category="Ship Helm") void SetHelmInput(float Throttle, float Rudder);
	UFUNCTION(BlueprintCallable, Category="Ship Motion|Storm") void SetStormBoost(float Boost);
	void GetEffectiveMotionLimits(float& OutHeaveCm, float& OutHeelDeg, float& OutDriftSpeed) const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Proxy;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DeckBase;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BoardingRamp;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="0.2", ClampMax="3")) float BuoyancyLiftRatio = 1.05f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="5", ClampMax="200", Units="cm")) float MaxHeaveCm = 70.f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="0.5", ClampMax="12")) float MaxHeelDeg = 3.5f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="10", ClampMax="400", Units="cm/s")) float MaxDriftSpeed = 55.f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="1", ClampMax="20")) float FollowInterp = 4.f;
	UPROPERTY(EditAnywhere, Category="Ship Motion|Storm", meta=(ClampMin="1", ClampMax="3")) float StormHeaveGain = 1.7f;
	UPROPERTY(EditAnywhere, Category="Ship Motion|Storm", meta=(ClampMin="1", ClampMax="3")) float StormHeelGain = 1.9f;
	UPROPERTY(EditAnywhere, Category="Ship Motion|Storm", meta=(ClampMin="1", ClampMax="3")) float StormDriftGain = 1.6f;
	/** Extra 0..1 boost on top of harbour SeaStrength packet (editor/playtest storm). */
	UPROPERTY(EditAnywhere, Replicated, Category="Ship Motion|Storm", meta=(ClampMin="0", ClampMax="1")) float StormBoost = 0.f;
	UPROPERTY(EditAnywhere, Category="Ship Helm", meta=(ClampMin="0", ClampMax="300", Units="cm/s")) float HelmSurgeSpeed = 95.f;
	UPROPERTY(EditAnywhere, Category="Ship Helm", meta=(ClampMin="0.5", ClampMax="20")) float MaxYawRateDeg = 6.f;
	UPROPERTY(EditAnywhere, Category="Ship Helm", meta=(ClampMin="1", ClampMax="45")) float MaxHelmYawDeg = 18.f;
	/** World-space motion since the last carry: Rider * Delta (or Delta.TransformPosition) keeps a rider fixed to the hull. */
	FTransform GetMotionDelta() const { return LastAppliedTransform.Inverse() * GetActorTransform(); }
	/** Rider carry step: returns true with the motion since InOutLastApplied once it is measurable, and advances
	 *  InOutLastApplied only then, so sub-threshold per-frame heel/drift accumulates instead of being lost. */
	static bool ConsumeRiderDelta(FTransform& InOutLastApplied, const FTransform& Current, FTransform& OutDelta);
private:
	void CaptureRest();
	void IntegrateWater(float DeltaSeconds);
	void CarryWorldRiders();
	FTransform ClampShipTransform(const FTransform& Proposed, float HeaveCm, float HeelDeg, float DriftSpeed) const;
	float ComputeStormBlend() const;
	UPROPERTY(Replicated) float HelmThrottle = 0.f;
	UPROPERTY(Replicated) float HelmRudder = 0.f;
	UPROPERTY(Replicated) float AccumulatedYawDeg = 0.f;
	FTransform RestTransform = FTransform::Identity;
	FTransform LastAppliedTransform = FTransform::Identity;
	FVector RestDryHullOrigin = FVector::ZeroVector;
	FVector LinearVelocity = FVector::ZeroVector;
	FVector AngularVelocityDeg = FVector::ZeroVector;
	bool bCapturedRest = false;
	TWeakObjectPtr<ACalmyOcean> Ocean;
};
