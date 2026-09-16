#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyHudReviewProbe.generated.h"

/** -CalmyHudReview=<Label>: photographs the minimal HUD, the wheelhouse screens and a crowded HUD; checks the layout never overlaps. */
UCLASS()
class CALMYPIRATESCO_API UCalmyHudReviewProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual void Tick(float DT) override;
    virtual void Deinitialize() override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyHudReviewProbe,STATGROUP_Tickables); }
private:
    void Finish(bool OK,const FString& Detail);
    void Photo(const FString& Name);
    FString Label;double Start=0,Since=0;int32 Stage=0;float SavedScale=-1;int32 Shots=0;
    TWeakObjectPtr<class ACameraActor> ReviewCamera;
};
