#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyCharacterProbe.generated.h"
class ACameraActor;
UCLASS()
class CALMYPIRATESCO_API UCalmyCharacterProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyCharacterProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Why);
    FString Role;
    float Age=0,StageTime=0,MaxJointGap=0;
    int32 Stage=0;
    bool Started=false,SawDown=false,SawRecovery=false;
    FVector Initial=FVector::ZeroVector;
    UPROPERTY() TObjectPtr<ACameraActor> ReviewCamera;
};
