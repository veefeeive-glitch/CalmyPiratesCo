#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyFlyTravelProbe.generated.h"
UCLASS()
class UCalmyFlyTravelProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyFlyTravelProbe, STATGROUP_Tickables); }
private:
    void Finish(bool OK, const FString& Detail);
    FString Role;
    FVector Initial = FVector::ZeroVector, Previous = FVector::ZeroVector;
    TArray<FVector> PreviousFlies;
    float Age=0, Walked=0, MaxDistance=0, MaxSpread=0, LastReport=0;
    double MaxStep=0;
    bool Started=false, Ready=false, LeftHold=false, Rested=false, Finished=false;
    TWeakObjectPtr<class ACameraActor> Camera;
    int32 Photos=0;
};
