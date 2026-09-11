#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CalmyCrewPresentation.generated.h"
class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class ACalmyCharacter;

// Walking uses CharacterMovement. During a fall the server simulates the whole
// articulated body; clients receive a root snapshot and simulate cosmetic limbs.
UCLASS(ClassGroup=Calmy)
class CALMYPIRATESCO_API UCalmyCrewPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    UCalmyCrewPresentation();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void UpdatePose(float DT);
    void ResetBodies();
    int32 SimulatedPartCount() const;
    float MaximumJointSeparation() const;
    bool HasFiniteBodies() const;
    float GetFallBlend() const { return FallBlend; }
    float GetHandContactError() const { return HandContactError; }
    float GetWorldHandContactError() const { return WorldHandContactError; }
    bool IsPhysicalFall() const { return bPhysicalFall; }
    bool PrepareRecovery();
    bool CanFinishRecovery() const;
    void ResetAfterRescue();
    void SetFallVelocity(FVector Velocity) { FallVelocity=Velocity.GetClampedToMaxSize(1200); }
    FTransform GetChestTransform() const;
private:
    // Server -> all peers snapshot, no client RPC/pose authority. Epoch rejects
    // a previous fall; late join receives current root/capsule and lifecycle.
    // Component and its bodies are destroyed with the pawn on disconnect.
    UPROPERTY(Replicated) FVector_NetQuantize10 FallRootPosition;
    UPROPERTY(Replicated) FRotator FallRootRotation;
    UPROPERTY(Replicated) FVector_NetQuantize10 FallCapsulePosition;
    UPROPERTY(Replicated) int32 FallEpoch=0;
    bool bPhysicalFall=false;
    FVector FallVelocity=FVector::ZeroVector;
    float RecoveryAge=0;
    void UpdatePhysicalRoot(float DT);
    bool FindRecoveryPosition(FVector& Position) const;
    UPROPERTY() TObjectPtr<ACalmyCharacter> Crew;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<UPhysicsConstraintComponent>> Joints;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ViewArms;
    float HandContactError=0;
    float WorldHandContactError=0;
    void UpdateCarryArms(float DT);
    void UpdateViewArms(float DT);
    FVector LastPosition=FVector::ZeroVector;
    float Phase=0,FallBlend=0,LastLifecycle=-1;
    float CarryBlend=0,MoveBlend=0;
    bool bRigBound=false;
};
