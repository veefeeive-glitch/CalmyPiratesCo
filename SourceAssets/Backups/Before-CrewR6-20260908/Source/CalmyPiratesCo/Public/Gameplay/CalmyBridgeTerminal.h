#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyBridgeTerminal.generated.h"
class UCanvas;
class UCanvasRenderTarget2D;
class UProceduralMeshComponent;
class ACalmyPlayerController;
class ACameraActor;

// Owner-local presentation only: no gameplay authority, replicated actor or RPC.
UCLASS()
class CALMYPIRATESCO_API ACalmyBridgeTerminal : public AActor
{
    GENERATED_BODY()
public:
    ACalmyBridgeTerminal();
    void Initialize(ACalmyPlayerController* Controller);
    void Open();
    void Refresh();
    void Hover();
    void Click();
    void Step(int32 Direction);
    void Activate();
    bool HasDisplay() const { return Display != nullptr; }
    static int32 RowAt(const FVector2D& Pixel, int32 Count);
    FVector RowWorld(int32 Row) const;
    UPROPERTY() TObjectPtr<ACameraActor> MenuCamera;
private:
    UFUNCTION() void Paint(UCanvas* Canvas,int32 Width,int32 Height);
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Screen;
    UPROPERTY() TObjectPtr<UCanvasRenderTarget2D> Display;
    UPROPERTY() TObjectPtr<ACalmyPlayerController> PC;
    int32 Page=0,Selected=0;
    FVector2D LastMouse=FVector2D(-1,-1);
    int32 Count() const { return Page==1?5:Page==2?2:3; }
    int32 MouseRow() const;
};
