#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyTutorial.generated.h"

class ACalmyCargo;
UENUM()
enum class ECalmyTutorialStep : uint8 { Walk, Grab, Deliver, Depart, TravelFailed };

// Team-wide, server-observed onboarding. No client completion/travel RPC.
UCLASS()
class CALMYPIRATESCO_API ACalmyTutorial : public AActor
{
    GENERATED_BODY()
public:
    ACalmyTutorial();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    static bool Enabled(const UWorld* World);
    static ACalmyTutorial* Find(const UWorld* World);
    static const TCHAR* Destination() { return TEXT("/Game/Maps/OceanFreighterR7"); }
    UPROPERTY(Replicated) ECalmyTutorialStep Step = ECalmyTutorialStep::Walk;
    UPROPERTY(Replicated) TObjectPtr<ACalmyCargo> TrainingCargo;
    UPROPERTY(Replicated) int32 DepartureSeconds = 8;
    UPROPERTY(Replicated) int32 WalkedCm = 0;
    FString Instruction() const;
    FVector ObjectiveLocation() const;
private:
    void Advance(ECalmyTutorialStep Next);
    TMap<TWeakObjectPtr<APawn>, FVector> PreviousPositions;
    float WalkDistance = 0, DepartureRemaining = 8;
    bool bTravelRequested = false;
};
