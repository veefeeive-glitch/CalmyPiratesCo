#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyWaterInteractionProbe.generated.h"

UCLASS()
class CALMYPIRATESCO_API UCalmyWaterInteractionProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyWaterInteractionProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    FString Role;
    double Age=0;
    int32 Spawned=0,ObservedMask=0,LastShot=-1;
    bool Started=false,Finished=false,Ready=false,VisualPlaced=false,OldEvent=false;
    float PeakHeight=0,PeakVelocity=0,MaxMeshError=0;
    int32 VertexChecks=0;
    TSet<int32> SeenSerials;
};
