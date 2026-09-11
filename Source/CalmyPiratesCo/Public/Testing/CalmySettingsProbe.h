#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmySettingsProbe.generated.h"
UCLASS()
class UCalmySettingsProbe:public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual bool IsTickableWhenPaused() const override{return true;}
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(CalmySettingsProbe,STATGROUP_Tickables);}
private:
    void Finish(bool OK,const FString& Detail);
    void Press(class ACalmyPlayerController* PC,FName Action);
    FString Role,Dir;
    int32 Stage=0;
    double Start=0,Since=0;
    FVector PawnPosition=FVector::ZeroVector;
    FIntPoint OldResolution;
    int32 OldMode=0;
};
