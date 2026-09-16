#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyAmbientBeds.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/** Quiet looping wind/hull/sea beds. Spawned once per ship or port map. */
UCLASS()
class CALMYPIRATESCO_API ACalmyAmbientBeds : public AActor
{
	GENERATED_BODY()
public:
	ACalmyAmbientBeds();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	static ACalmyAmbientBeds* Find(const UWorld* World);
	static ACalmyAmbientBeds* Ensure(UWorld* World);
	void SetStormLevel(float Storm01);
	void SetLeakAlarm(bool bActive);
private:
	void RebuildBeds();
	void Feed(USoundWaveProcedural* Wave, int32 Kind, float Gain);
	UPROPERTY() TObjectPtr<UAudioComponent> Wind;
	UPROPERTY() TObjectPtr<UAudioComponent> Hull;
	UPROPERTY() TObjectPtr<UAudioComponent> Sea;
	UPROPERTY() TObjectPtr<UAudioComponent> Alarm;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> WindWave;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> HullWave;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> SeaWave;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> AlarmWave;
	float Storm01 = 0.f;
	bool bLeakAlarm = false;
	float Phase = 0.f;
	FRandomStream Noise;
};
