#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyInteractionProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyInteractionProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyInteractionProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    FString Role;
    float Age=0,StageAge=0,SoakAge=0,MaxGap=0,Walked=0;
    double WallStart=0;
    int32 Stage=0,RejectedBefore=0,Waypoint=0,LastCycle=-1;
    bool Started=false,FloorCreated=false,SawHeld=false;
    FVector Previous=FVector::ZeroVector;
};
