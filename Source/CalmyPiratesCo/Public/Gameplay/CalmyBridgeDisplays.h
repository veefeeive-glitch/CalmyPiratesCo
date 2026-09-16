#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyBridgeDisplays.generated.h"
class UCanvas;
class UCanvasRenderTarget2D;
class UProceduralMeshComponent;
class ACalmyPlayerController;

// Owner-local wheelhouse status screens on the free R7 bridge consoles: ALARMS, WATCH, CREW, SEA.
// Presentation only: reads replicated state, no authority, no replication, no RPC.
UCLASS()
class CALMYPIRATESCO_API ACalmyBridgeDisplays : public AActor
{
    GENERATED_BODY()
public:
    ACalmyBridgeDisplays();
    void Initialize(ACalmyPlayerController* Controller);
    UFUNCTION() void Refresh();
    int32 NumScreens() const { return Targets.Num(); }
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UFUNCTION() void PaintAlarms(UCanvas* Canvas,int32 Width,int32 Height);
    UFUNCTION() void PaintWatch(UCanvas* Canvas,int32 Width,int32 Height);
    UFUNCTION() void PaintCrew(UCanvas* Canvas,int32 Width,int32 Height);
    UFUNCTION() void PaintSea(UCanvas* Canvas,int32 Width,int32 Height);
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Screens;
    UPROPERTY() TArray<TObjectPtr<UCanvasRenderTarget2D>> Targets;
    UPROPERTY() TObjectPtr<ACalmyPlayerController> PC;
    FTimerHandle RefreshTimer;
};
