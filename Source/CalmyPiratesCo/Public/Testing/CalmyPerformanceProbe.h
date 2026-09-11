#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyPerformanceProbe.generated.h"
UCLASS()
class UCalmyPerformanceProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyPerformanceProbe,STATGROUP_Tickables); }
private:
    TWeakObjectPtr<class ACameraActor> Camera;
    FString Label;
    double Start=0, Previous=0;
    TArray<double> Frames,Game,Render,Gpu,Brain;
    bool Finished=false;
};
