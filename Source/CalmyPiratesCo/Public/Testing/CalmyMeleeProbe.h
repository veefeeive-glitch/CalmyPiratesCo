#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyMeleeProbe.generated.h"
UCLASS()
class UCalmyMeleeProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyMeleeProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    void Photo(const FString& Name);
    FString Role;
    float Age=0,Since=0,CounterAge=0;
    int32 Stage=0;
    bool Started=false,PlacedHelper=false,PlacedAttacker=false,MovedToFlies=false;
    bool SawCounter=false,SawScatter=false,ReplaySent=false;
    bool CounterPhotoTaken=false;
    TWeakObjectPtr<class ACalmyCharacter> Active;
    int32 ActiveRejections=0;
};
