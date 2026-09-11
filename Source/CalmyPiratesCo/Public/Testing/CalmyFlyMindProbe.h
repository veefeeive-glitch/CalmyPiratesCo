#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyFlyMindProbe.generated.h"
UCLASS()
class UCalmyFlyMindProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float Dt) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyFlyMindProbe, STATGROUP_Tickables); }
private:
    void Finish(bool OK, const FString& Detail);
    FString Role;
    float Age = 0, StageAge = 0, PeersAge = 0;
    int32 Stage = 0;
    bool bStarted = false, bFarRejected = false, bInvalidRejected = false, bSawRecall = false;
    TSet<uint32> PlacedPlayers;
};
