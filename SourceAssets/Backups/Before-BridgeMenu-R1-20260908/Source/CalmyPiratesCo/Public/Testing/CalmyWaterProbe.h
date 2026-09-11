#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyWaterProbe.generated.h"
class UStaticMeshComponent;
UCLASS()
class CALMYPIRATESCO_API UCalmyWaterProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyWaterProbe,STATGROUP_Tickables); }
private:
    void Finish(bool bOK,const FString& Detail);
    FString Role;
    float Age=0,StageTime=0;
    int32 Stage=0;
    bool bStarted=false;
    bool bWaterGrabAttempted=false;
    int32 RejectionsBeforeWater=0;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GravityControl;
};
