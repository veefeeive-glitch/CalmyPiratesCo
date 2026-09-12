#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyHullProxy.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class ACalmyOcean;
UCLASS()
class CALMYPIRATESCO_API ACalmyHullProxy : public AActor
{
	GENERATED_BODY()
public:
	ACalmyHullProxy();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	static ACalmyHullProxy* Find(const UWorld* World);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Deck;
	UPROPERTY(EditAnywhere, Category="Hull Proxy", meta=(ClampMin="50", ClampMax="20000", Units="kg")) float HullMassKg = 1200.f;
	UPROPERTY(EditAnywhere, Category="Hull Proxy") FVector CenterOfMassLocal = FVector(0.f, 0.f, -40.f);
	UPROPERTY(EditAnywhere, Category="Hull Proxy", meta=(ClampMin="1", ClampMax="3")) float BuoyancyLiftRatio = 1.35f;
	UPROPERTY(EditAnywhere, Category="Hull Proxy", meta=(ClampMin="0", ClampMax="1")) float HomeSpring = 0.02f;
	UPROPERTY(EditAnywhere, Category="Hull Proxy", meta=(ClampMin="100", ClampMax="5000", Units="cm")) float MaxHomeDriftCm = 1800.f;
	FVector GetRestLocation() const { return RestLocation; }
	bool HasRestPose() const { return bCapturedRest; }
private:
	void CaptureRest();
	void ConfigurePhysics();
	void SoftHome(float DeltaSeconds);
	FVector RestLocation = FVector::ZeroVector;
	bool bCapturedRest = false;
	TWeakObjectPtr<ACalmyOcean> Ocean;
};
