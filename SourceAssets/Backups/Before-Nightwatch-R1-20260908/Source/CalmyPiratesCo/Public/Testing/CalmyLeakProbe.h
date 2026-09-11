#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyLeakProbe.generated.h"
class ACalmyCharacter;

/** Real processes and ordinary movement/RPC intents; no teleports or gameplay state fixtures. */
UCLASS()
class CALMYPIRATESCO_API UCalmyLeakProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyLeakProbe, STATGROUP_Tickables); }
private:
	void Finish(bool Success, const FString& Detail);
	FString Role;
	TWeakObjectPtr<ACalmyCharacter> DepartingWorker;
	float Age = 0;
	float StageTime = 0;
	int32 Stage = 0;
	int32 CargoIndex = 1;
	int32 SavedProgress = 0;
	int32 NewRoundInitialSeconds = 0;
	bool bStarted = false;
	bool bSawWarning = false;
	bool bSawPowerDegraded = false;
	bool bSawDirectorSnapshot = false;
};
