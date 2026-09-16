#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/CalmyDoorMotion.h"
#include "CalmyDoor.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class ACalmyCharacter;
// Ajar is appended so existing replicated values keep their meaning.
UENUM() enum class ECalmyDoorState:uint8 { Closed,Opening,Open,Closing,Blocked,Ajar };
UENUM() enum class ECalmyDoorImpact:uint8 { None,Latch,Slam,Knock,Bump };
/**
 * Heavy watertight door (Amnesia-style). The server owns the leaf: a tap swings it fully on its own, holding E lets
 * one player push/pull it with the mouse through FCalmyDoorMotion (inertia, limited acceleration, hinge friction),
 * and the leaf latches, slams, knocks the stop or bumps into anything in its swing. Clients present the replicated
 * angle smoothly and play creak/impact sounds from replicated motion; nothing about the pose is client-authored.
 */
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
    UPROPERTY(Replicated) float AngularVelocity=0;
    UPROPERTY(Replicated) uint32 Revision=0;
    UPROPERTY(ReplicatedUsing=OnRep_State) ECalmyDoorState State=ECalmyDoorState::Closed;
    /** Pawn currently holding the leaf. Server-owned; cleared on release, range loss, cargo pickup or disconnect. */
    UPROPERTY(Replicated) TObjectPtr<ACalmyCharacter> Holder;
    UPROPERTY(ReplicatedUsing=OnRep_Impact) uint16 ImpactSerial=0;
    UPROPERTY(Replicated) ECalmyDoorImpact ImpactKind=ECalmyDoorImpact::None;
    UPROPERTY(Replicated) float ImpactSpeed=0;
    UPROPERTY(Replicated) float ImpactServerTime=-1;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Leaf;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HandleFront;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HandleBack;
    /** Tap E: swing fully open or shut. Server only; ExpectedRevision rejects stale or duplicate taps. */
    bool RequestToggle(ACalmyCharacter* User,uint32 ExpectedRevision);
    /** Hold E: take the leaf. Server re-checks the aim at this door; one holder at a time. */
    bool TryGrab(ACalmyCharacter* User);
    void ReleaseGrab(ACalmyCharacter* User);
    /** Holder's push in [-1,1] (positive opens). Expires after DragLeaseSeconds so lost packets let the leaf coast. */
    bool ApplyDrag(ACalmyCharacter* User,float Push);
    /** Server tools/probes: swing fully open or shut exactly like a tap, without a user. */
    void SetAutoTarget(bool bOpen);
    /** Server tools/tests: place the leaf at an angle at rest. */
    void SnapTo(float Degrees);
    static ACalmyDoor* FindTarget(const ACalmyCharacter* User);
    bool IsOpen() const { return Angle>=OpenAngle-2.f; }
    bool IsClosed() const { return State==ECalmyDoorState::Closed; }
    /** Handle position on the free edge of the leaf at the current angle. */
    FVector GetGripLocation() const;
    /** Mouse motion (camera-relative) -> push along the leaf's swing direction at the grip, positive opens. */
    static float PushFromMouse(const FVector& ViewForward,const FVector& ViewRight,float MouseX,float MouseY,const FVector& HingeToGrip,float SwingSign);
    static constexpr float DragLeaseSeconds=.25f;
    static constexpr float HoldRangeCm=300.f;
private:
    friend class UCalmyDoorProbe;
    friend class UCalmyTraversalProbe;
    UFUNCTION() void OnRep_Pose();
    UFUNCTION() void OnRep_State();
    UFUNCTION() void OnRep_Impact();
    void ApplyPose(float Degrees);
    void ApplyDoorMaterials();
    bool PoseBlocked(float Degrees) const;
    /** Audit: actor/component/mesh names of everything that blocks the leaf at this angle ("none" when clear). */
    FString PoseBlocker(float Degrees) const;
    void Simulate(float DT);
    void Present(float DT);
    void EmitImpact(ECalmyDoorImpact Kind,float Speed);
    void PlayImpact(ECalmyDoorImpact Kind,float Speed);
    void Wake();
    float ServerClock() const;
    FCalmyDoorMotion Motion;
    bool bAuto=false;
    float AutoTargetAngle=0;
    float DragPush=0;
    float DragLeaseEnd=-1;
    float DisplayAngle=0;
    float CreakCountdown=0;
    float SettledSeconds=0;
    bool bWatertightLeaf=false;
    ECalmyDoorState LastHeardState=ECalmyDoorState::Closed;
    uint16 HeardImpactSerial=0;
};
