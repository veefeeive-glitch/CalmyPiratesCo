#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CalmyCrewPresentation.generated.h"
class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class ACalmyCharacter;

// Collision-independent presentation: Chaos articulated limbs follow a kinematic
// torso. CharacterMovement + server lifecycle remain the gameplay authority.
UCLASS(ClassGroup=Calmy)
class CALMYPIRATESCO_API UCalmyCrewPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    UCalmyCrewPresentation();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    void UpdatePose(float DT);
    void ResetBodies();
    int32 SimulatedPartCount() const;
    float MaximumJointSeparation() const;
    bool HasFiniteBodies() const;
    float GetFallBlend() const { return FallBlend; }
    float GetHandContactError() const { return HandContactError; }
private:
    UPROPERTY() TObjectPtr<ACalmyCharacter> Crew;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<UPhysicsConstraintComponent>> Joints;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ViewArms;
    float HandContactError=0;
    void UpdateViewArms(float DT);
    FVector LastPosition=FVector::ZeroVector;
    float Phase=0,FallBlend=0,LastLifecycle=-1;
    float CarryBlend=0,MoveBlend=0;
    bool bRigBound=false;
};
