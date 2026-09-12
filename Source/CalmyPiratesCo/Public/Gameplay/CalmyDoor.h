#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyDoor.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class ACalmyCharacter;
UENUM() enum class ECalmyDoorState:uint8 { Closed,Opening,Open,Closing,Blocked };
UCLASS()
class CALMYPIRATESCO_API ACalmyDoor:public AActor
{
    GENERATED_BODY()
public:
    ACalmyDoor();
    UFUNCTION(CallInEditor,BlueprintCallable) void RebuildGeometry();
    UFUNCTION(CallInEditor,BlueprintCallable) bool FitSwingToWorld();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(EditAnywhere,Replicated) int32 DoorId=0;
    UPROPERTY(EditAnywhere) float WidthCm=112;
    UPROPERTY(EditAnywhere) float HeightCm=210;
    UPROPERTY(EditAnywhere) bool bStartOpen=false;
    UPROPERTY(EditAnywhere) float SwingSign=1;
    UPROPERTY(EditAnywhere) float OpenAngle=180;
    UPROPERTY(ReplicatedUsing=OnRep_Pose) float Angle=0;
    UPROPERTY(Replicated) uint32 Revision=0;
    UPROPERTY(Replicated) ECalmyDoorState State=ECalmyDoorState::Closed;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Leaf;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HandleFront;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HandleBack;
    bool RequestToggle(ACalmyCharacter* User,uint32 ExpectedRevision);
    static ACalmyDoor* FindTarget(const ACalmyCharacter* User);
    bool IsOpen() const { return Angle>=OpenAngle-.5f; }
private:
    friend class UCalmyDoorProbe;
    UFUNCTION() void OnRep_Pose();
    void ApplyPose();
    bool PoseBlocked(float Degrees) const;
    float TargetAngle=0;
};
