#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyShiftProbe.generated.h"

/** Opt-in development-only integration test. Two explicitly logged server fixtures force falls;
 * movement, grabs, throws, delivery, ownership rejection and replay protection use production paths. */
UCLASS()
class CALMYPIRATESCO_API UCalmyShiftProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyShiftProbe, STATGROUP_Tickables); }
private:
	void Finish(bool bSuccess, const FString& Details);
	FString Role;
	int32 Stage = 0;
	int32 CargoIndex = 1;
	float Age = 0;
	float StageTime = 0;
	bool bStarted = false;
};
