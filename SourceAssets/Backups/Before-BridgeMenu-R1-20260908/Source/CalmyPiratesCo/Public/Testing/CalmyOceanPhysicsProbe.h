#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyOceanPhysicsProbe.generated.h"

class ACalmyCharacter;
class ACalmyCargo;
class FJsonValue;
struct FOceanWalk
{
    ACalmyCharacter* Pawn=nullptr;
    ACalmyCharacter* Helper=nullptr;
    ACalmyCargo* Cargo=nullptr;
    bool Bulky=false, Grabbed=false;
    int32 HelperTarget=1;
    float CargoOverlapTime=0;
    FString CargoContact;
    FVector CargoPosition=FVector::ZeroVector;
    FQuat CargoRotation=FQuat::Identity;
    FString Name;
    TArray<FVector> Points;
    int32 Target=1;
    float Age=0, Still=0;
    FVector Previous=FVector::ZeroVector;
    bool Done=false;
    bool ExpectedBlocked=false;
    float NextSample=0;
    TArray<TSharedPtr<FJsonValue>> Samples;
};

/** Opt-in, non-shipping collision laboratory. Never active in normal gameplay. */
UCLASS()
class CALMYPIRATESCO_API UCalmyOceanPhysicsProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyOceanPhysicsProbe, STATGROUP_Tickables); }
private:
    void Start();
    void Complete(FOceanWalk& Walk, bool Passed, const FString& Reason);
    void Finish();
    void SpawnWalk(FOceanWalk& Walk);
    TArray<FOceanWalk> Walks;
    TArray<TSharedPtr<class FJsonValue>> Results;
    FString SourceHash;
    bool Started=false, Finished=false;
    bool CarryAudit=false;
    bool NpcAudit=false;
};
