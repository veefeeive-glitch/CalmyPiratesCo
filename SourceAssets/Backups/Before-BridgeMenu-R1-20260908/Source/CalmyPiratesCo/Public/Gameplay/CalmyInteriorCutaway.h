#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyInteriorCutaway.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** Local presentation actor: roof is visible from deck and hides only for the local player inside. */
UCLASS()
class CALMYPIRATESCO_API ACalmyInteriorCutaway : public AActor
{
	GENERATED_BODY()
public:
	ACalmyInteriorCutaway();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	static bool ShouldHideRoof(const FVector& LocalPawnLocation);
	static bool ShouldHideMainDeck(const FVector& LocalPawnLocation);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UStaticMeshComponent>> RoofPanels;
private:
	bool bRoofHidden = false;
	bool bMainDeckHidden = false;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> MainDeckOccluders;
};
