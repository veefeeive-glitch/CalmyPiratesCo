#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyFlySwarmProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyFlySwarmProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyFlySwarmProbe, STATGROUP_Tickables); }
private:
    void Finish(bool OK, const FString& Detail);
    FString Role;
    float Age = 0.f;
    float ObservedSeconds = 0.f;
    FVector FirstPosition = FVector::ZeroVector;
    bool bObserved = false;
    bool bMoved = false;
    bool bSpawned = false;
    double MaxStepMs = 0.;
    double SumStepMs = 0.;
    int32 Samples = 0;
    TSet<uint32> SeenControllers;
    float AllPeersAge = 0.f;
    int32 ScreenshotStage = 0;
    TWeakObjectPtr<class ACameraActor> ReviewCamera;
};
