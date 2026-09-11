#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyHelperProbe.generated.h"
UCLASS()
class CALMYPIRATESCO_API UCalmyHelperProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyHelperProbe,STATGROUP_Tickables);}
private:
    void Finish(bool OK,const FString& Why);
    FString Role;float Age=0,StageTime=0;int32 Stage=0,LowestTeamPace=100;
    bool Started=false,SawHelper=false,SawHeld=false;
};
