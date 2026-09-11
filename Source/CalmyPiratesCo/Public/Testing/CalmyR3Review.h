#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyR3Review.generated.h"
class ACameraActor;
UCLASS()
class CALMYPIRATESCO_API UCalmyR3Review : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyR3Review,STATGROUP_Tickables);}
private:
    float Age=0,StageTime=0,MaxHandError=0,MaxMovingHandError=0,PhotoAt=-1;
    int32 Stage=0;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    void Photo(const FString& Name);
};
