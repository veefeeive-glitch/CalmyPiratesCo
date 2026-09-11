#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyPauseProbe.generated.h"

UCLASS()
class CALMYPIRATESCO_API UCalmyPauseProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyPauseProbe, STATGROUP_Tickables); }
private:
    void Finish(bool bPass, const FString& Detail);
    FString Role;
    double Start = 0, StageStart = 0;
    float WorldTimeAtOpen = 0;
    int32 Stage = 0;
};
