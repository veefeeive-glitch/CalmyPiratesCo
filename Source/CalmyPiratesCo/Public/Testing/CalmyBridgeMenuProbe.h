#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyBridgeMenuProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyBridgeMenuProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyBridgeMenuProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    void Photo(const FString& Name);
    FString Role;double Start=0,Since=0;int32 Stage=0;
    FVector Spawn;float Clock=0;bool Steady=false;
};
