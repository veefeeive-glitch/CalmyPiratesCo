#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyShipMotion.generated.h"

class UBoxComponent;
class ACalmyOcean;

/**
 * G2 ship ride for OceanFreighterR7: waterline samples use the same HeightAt /
 * VerticalVelocityAt / FlowAt field as cargo buoyancy (W1 disturbances included).
 * A clamped replicated root carries attached ship actors; dry-hull queries map
 * into the authored rest frame. Not Chaos on the full R7 triangle mesh.
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

	const FTransform& GetRestTransform() const { return RestTransform; }
	FVector GetRestDryHullOrigin() const { return RestDryHullOrigin; }
	bool HasRestPose() const { return bCapturedRest; }

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Proxy;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DeckBase;

	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="0.2", ClampMax="3")) float BuoyancyLiftRatio = 1.05f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="5", ClampMax="200", Units="cm")) float MaxHeaveCm = 70.f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="0.5", ClampMax="12")) float MaxHeelDeg = 3.5f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="10", ClampMax="400", Units="cm/s")) float MaxDriftSpeed = 55.f;
	UPROPERTY(EditAnywhere, Category="Ship Motion", meta=(ClampMin="1", ClampMax="20")) float FollowInterp = 4.f;

	/** Last applied pose; used to carry world-space riders (flies, resting cargo). */
	FTransform GetMotionDelta() const { return GetActorTransform() * LastAppliedTransform.Inverse(); }

private:
	void CaptureRest();
	void IntegrateWater(float DeltaSeconds);
	void CarryWorldRiders();
	FTransform ClampShipTransform(const FTransform& Proposed) const;

	FTransform RestTransform = FTransform::Identity;
	FTransform LastAppliedTransform = FTransform::Identity;
	FVector RestDryHullOrigin = FVector::ZeroVector;
	FVector LinearVelocity = FVector::ZeroVector;
	FVector AngularVelocityDeg = FVector::ZeroVector;
	bool bCapturedRest = false;
	TWeakObjectPtr<ACalmyOcean> Ocean;
};
