#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyShipProbe.generated.h"
class ACameraActor;

// Opt-in real-pawn R7 route. No teleports, lifecycle bypass or collision substitutions.
UCLASS()
class CALMYPIRATESCO_API UCalmyShipProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DT) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyShipProbe, STATGROUP_Tickables); }
private:
    void Finish(bool Success,const FString& Detail);
    void Photo(const FString& Name);
    FString Role;
    TArray<FVector> Route;
    FVector Previous=FVector::ZeroVector;
    int32 Point=0,Stage=0,DryChecks=0;
    float Age=0,StageTime=0,PointTime=0,Walked=0,MaxJoint=0;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
};
