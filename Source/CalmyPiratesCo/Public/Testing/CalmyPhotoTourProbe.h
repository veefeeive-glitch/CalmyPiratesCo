#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmyPhotoTourProbe.generated.h"
class ACameraActor;

/**
 * Visual review tour of OceanFreighterR7 (-CalmyPhotoTour=<doors|shots.json>, needs a real RHI).
 * "doors": every CalmyDoor photographed closed from both faces (eye 1.9 m out, 1.6 m high) and open from face a.
 * A JSON file: {"shots":[{"name","eye":[x,y,z],"target":[x,y,z],"fov":70,"doors":"open|closed|keep"}]} in ship-local cm.
 * The camera rides the ship frame; frames land in Saved/Verification/<CalmyReviewDir>/<name>.png.
 */
UCLASS()
class CALMYPIRATESCO_API UCalmyPhotoTourProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DT) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmyPhotoTourProbe, STATGROUP_Tickables); }

private:
	struct FShot { FString Name; FVector Eye, Target; float Fov = 70; int32 Doors = -1; };
	bool LoadShots();
	void Finish(bool bOK, const FString& Detail);
	FString Dir, Source;
	TArray<FShot> Shots;
	TWeakObjectPtr<ACameraActor> Camera;
	double Start = 0, Since = 0;
	int32 Stage = 0, Index = 0, Taken = 0, DoorState = -2;
};
