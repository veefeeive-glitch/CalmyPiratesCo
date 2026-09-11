#pragma once
#include "Gameplay/CalmyCharacter.h"
#include "CalmyCrewHelper.generated.h"

UENUM(BlueprintType)
enum class ECalmyHelperState : uint8 { Following, Assisting, Blocked, Waiting };

// Server-driven, tangible crew mate. No player slot and no client-owned AI RPC.
UCLASS()
class CALMYPIRATESCO_API ACalmyCrewHelper : public ACalmyCharacter
{
    GENERATED_BODY()
public:
    ACalmyCrewHelper(const FObjectInitializer& Init);
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void AssignLeader(ACalmyCharacter* Pawn);
    void ReleaseLeader();
    void ResetToStation();
    void UpdateAssistance(float DT);
    bool CanAssist(const ACalmyCharacter* Pawn) const;
    UPROPERTY(Replicated,BlueprintReadOnly) TObjectPtr<ACalmyCharacter> Leader;
    UPROPERTY(Replicated,BlueprintReadOnly) ECalmyHelperState Assistance=ECalmyHelperState::Waiting;
    UPROPERTY(Replicated,BlueprintReadOnly) float FollowDistance=0;
private:
    TArray<FVector> Trail;
    FVector LastLeader=FVector::ZeroVector,Previous=FVector::ZeroVector;
    float StuckTime=0;
    FVector Station=FVector::ZeroVector;
};
