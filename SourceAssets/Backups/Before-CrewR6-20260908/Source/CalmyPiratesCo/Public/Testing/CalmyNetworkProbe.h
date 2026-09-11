#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyNetworkProbe.generated.h"

class ACalmyCharacter;

/** Opt-in, non-shipping local multi-process regression probe. No test RPCs or authority shortcuts. */
UCLASS()
class CALMYPIRATESCO_API UCalmyNetworkProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyNetworkProbe, STATGROUP_Tickables); }
private:
	FString Role;
	float Age = 0;
	float StageTime = 0;
	int32 Stage = 0;
	FVector StartPosition = FVector::ZeroVector;
	FVector RemoteStart = FVector::ZeroVector;
	TWeakObjectPtr<ACalmyCharacter> Thrower;
	bool bStarted = false;
	bool bMoved = false;
	bool bHeld = false;
	bool bFollowed = false;
	bool bThrown = false;
	bool bKnocked = false;
	bool bRecovering = false;
	bool bRecovered = false;
	void Finish(bool Success, const FString& Details);
};
