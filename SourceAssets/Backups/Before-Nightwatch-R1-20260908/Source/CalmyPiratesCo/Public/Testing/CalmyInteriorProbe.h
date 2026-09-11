#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyInteriorProbe.generated.h"

/** Opt-in integration probe: each real network role walks through production collision into the cutaway. */
UCLASS()
class CALMYPIRATESCO_API UCalmyInteriorProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyInteriorProbe, STATGROUP_Tickables); }
private:
	void Finish(bool bSuccess, const FString& Detail);
	FString Role;
	int32 Stage = 0;
	float Age = 0;
	float StageTime = 0;
	bool bStarted = false;
};
