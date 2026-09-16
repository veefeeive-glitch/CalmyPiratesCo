#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/CalmyVoyageContract.h"
#include "Components/BoxComponent.h"
#include "CalmyMissionBoard.generated.h"

class ACalmyCharacter;
class UStaticMeshComponent;

/**
 * PortHub-only mission kiosk. Host/solo picks a contract; accept saves + ServerTravels to OceanFreighterR7.
 * Presentation is owner-local HUD; travel authority stays on the listen host.
 */
UCLASS()
class CALMYPIRATESCO_API ACalmyMissionBoard : public AActor
{
	GENERATED_BODY()
public:
	ACalmyMissionBoard();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	static ACalmyMissionBoard* Find(const UWorld* World);
	static bool IsPortMap(const UWorld* World);
	static const TCHAR* PortMapPath() { return TEXT("/Game/Maps/PortHub"); }
	static const TCHAR* ShipMapPath() { return TEXT("/Game/Maps/OceanFreighterR7"); }
	bool TryStep(ACalmyCharacter* Requester, int32 Direction);
	bool TryAccept(ACalmyCharacter* Requester);
	FString PromptText() const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> UseZone;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pedestal;
	UPROPERTY(ReplicatedUsing=OnRep_Selected) int32 SelectedIndex = 1;
	UPROPERTY(Replicated) FString AcceptedContractId;
private:
	UFUNCTION() void OnRep_Selected();
	TArray<FCalmyVoyageContract> Options;
	bool bTravelRequested = false;
};
