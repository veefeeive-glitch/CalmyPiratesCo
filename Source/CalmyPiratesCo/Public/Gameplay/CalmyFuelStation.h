#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyFuelStation.generated.h"

class ACalmyCharacter;
class UStaticMeshComponent;
class UPointLightComponent;

/** Hold-F refill point for voyage fuel meter. Server-owned progress only. */
UCLASS()
class CALMYPIRATESCO_API ACalmyFuelStation : public AActor
{
	GENERATED_BODY()
public:
	ACalmyFuelStation();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	static ACalmyFuelStation* Find(const UWorld* World);
	bool CanRefuel(const ACalmyCharacter* Pawn) const;
	bool TryRefuelPulse(ACalmyCharacter* Pawn);
	void StopRefuel(ACalmyCharacter* Pawn);
	FString GetStatusText() const;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Pump;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> TaskLight;
	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<ACalmyCharacter> Refueler;
private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	double RefuelLeaseEnd = 0;
	float NextPulseSound = 0.f;
};
