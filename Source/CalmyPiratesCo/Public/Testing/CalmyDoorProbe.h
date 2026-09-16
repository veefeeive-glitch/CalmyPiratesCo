#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyDoorProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyDoorProbe:public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer)const override;
    virtual void Tick(float DT)override;
    virtual TStatId GetStatId()const override {RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyDoorProbe,STATGROUP_Tickables);}
private:
    FString Role;float Age=0,StageAge=0,RpcAge=0,DragStartAngle=0,DragMoved=0;int32 Stage=0,RpcStage=0;bool Fixture=false;void Finish(bool OK,const FString& Detail);
};
