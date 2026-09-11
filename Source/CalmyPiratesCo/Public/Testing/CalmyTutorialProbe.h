#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyTutorialProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyTutorialProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyTutorialProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    void Capture(const FString& Name);
    FString Role;
    float Age=0,HeldAge=0,Retry=0,ShipWalk=0;
    int32 LastStep=-1,Shot=-1;
    bool Started=false,Ready=false,MenuSeen=false;
    FVector Previous=FVector::ZeroVector;
};
