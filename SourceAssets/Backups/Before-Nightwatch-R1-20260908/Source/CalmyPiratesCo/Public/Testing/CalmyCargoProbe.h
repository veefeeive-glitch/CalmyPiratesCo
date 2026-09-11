#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyCargoProbe.generated.h"
class ACalmyCharacter;

/** Opt-in non-shipping real host/client cargo-condition regression. No fixture teleports or test RPCs. */
UCLASS()
class CALMYPIRATESCO_API UCalmyCargoProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyCargoProbe, STATGROUP_Tickables); }
private:
	void Finish(bool Success, const FString& Detail);
	FString Role;
	TWeakObjectPtr<ACalmyCharacter> Peer;
	float Age = 0;
	float StageTime = 0;
	int32 Stage = 0;
	bool bStarted = false;
};
