#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmySessionProbe.generated.h"

UCLASS()
class CALMYPIRATESCO_API UCalmySessionProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmySessionProbe, STATGROUP_Tickables); }
private:
	void Finish(bool OK, const FString& Detail);
	FString Role;
	int32 Stage = 0;
	float Age = 0.f;
	int32 ExpectedPeers = 3;
	bool bStarted = false;
	float ReadyAt = -1.f;
	int32 PeakCrew = 0;
};
