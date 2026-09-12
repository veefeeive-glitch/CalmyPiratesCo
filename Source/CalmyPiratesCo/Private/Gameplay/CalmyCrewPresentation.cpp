#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Core/CrewGait.h"
#include "Core/CrewArmIK.h"
#include "Core/CrewHand.h"
#include "Core/CrewRecovery.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyOcean.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

namespace CrewRig
{
    static const TCHAR* Names[]={TEXT("Torso"),TEXT("Head"),TEXT("UpperArmL"),TEXT("ForearmL"),TEXT("UpperArmR"),TEXT("ForearmR"),TEXT("ThighL"),TEXT("ShinL"),TEXT("ThighR"),TEXT("ShinR")};
    static const FVector Bind[]={FVector(0,0,-6),FVector(0,0,51),FVector(0,-27,44),FVector(0,-29,17),FVector(0,27,44),FVector(0,29,17),FVector(0,-12,-6),FVector(0,-12,-44),FVector(0,12,-6),FVector(0,12,-44)};
    static const int32 Parent[]={0,0,0,2,0,4,0,6,0,8};
}
UCalmyCrewPresentation::UCalmyCrewPresentation()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
    SetIsReplicatedByDefault(true);
}
void UCalmyCrewPresentation::BeginPlay()
{
    Super::BeginPlay();Crew=Cast<ACalmyCharacter>(GetOwner());
    if(!Crew) return; // Dedicated servers need the authoritative falling bodies too.
    for(int32 I=0;I<10;++I)
    {
        auto* M=NewObject<UStaticMeshComponent>(Crew,FName(*FString::Printf(TEXT("Deckhand_%s"),CrewRig::Names[I])));
        M->SetupAttachment(Crew->GetRootComponent());M->SetMobility(EComponentMobility::Movable);
        const TCHAR* Variant=Cast<ACalmyCrewHelper>(Crew)?TEXT("Misha"):TEXT("Deckhand");
        M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/CrewR8/SM_%s_%s.SM_%s_%s"),Variant,CrewRig::Names[I],Variant,CrewRig::Names[I])));
        M->SetRelativeLocation(CrewRig::Bind[I]);M->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        M->SetCollisionObjectType(ECC_PhysicsBody);M->SetCollisionResponseToAllChannels(ECR_Ignore);
        // Retain a well-defined kinematic simulation shape for the torso.
        M->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
        M->SetGenerateOverlapEvents(false);M->SetCanEverAffectNavigation(false);
        M->BodyInstance.bAutoWeld=false;
        M->BodyInstance.PositionSolverIterationCount=16;M->BodyInstance.VelocitySolverIterationCount=4;
        // Camera is chest-mounted: hide its own housing/vest and head only for
        // the owner, retaining arms, trousers and boots plus the full shadow.
        M->SetOwnerNoSee(I<=5);M->SetCastHiddenShadow(true);
        Crew->AddInstanceComponent(M);M->RegisterComponent();
        M->SetMassOverrideInKg(NAME_None,I==0?24.f:(I==1?4.f:3.f));
        M->SetLinearDamping(2.f);M->SetAngularDamping(4.f);
        M->SetSimulatePhysics(I>0);Parts.Add(M);
    }
    for(int32 I=1;I<10;++I)
    {
        auto* J=NewObject<UPhysicsConstraintComponent>(Crew,FName(*FString::Printf(TEXT("CrewJoint%d"),I)));
        J->SetupAttachment(Crew->GetRootComponent());J->SetRelativeLocation(CrewRig::Bind[I]);
        Crew->AddInstanceComponent(J);J->RegisterComponent();
        J->SetDisableCollision(true);
        J->SetLinearXLimit(LCM_Locked,0);J->SetLinearYLimit(LCM_Locked,0);J->SetLinearZLimit(LCM_Locked,0);
        J->SetAngularSwing1Limit(ACM_Limited,I==1?45.f:85.f);
        J->SetAngularSwing2Limit(ACM_Limited,I==1?45.f:100.f);
        J->SetAngularTwistLimit(ACM_Limited,I==1?55.f:65.f);
        J->SetAngularDriveMode(EAngularDriveMode::SLERP);
        J->SetOrientationDriveSLERP(true);J->SetAngularVelocityDriveSLERP(true);
        J->SetAngularDriveAccelerationMode(true);J->SetAngularDriveParams(1200,90,0);
        J->SetProjectionEnabled(true);J->SetProjectionParams(.5f,.3f,2,15);
        Joints.Add(J);
    }
    LastPosition=Crew->GetActorLocation();
    for(int32 I=2;I<=5;++I)
    {
        auto* M=NewObject<UStaticMeshComponent>(Crew,FName(*FString::Printf(TEXT("ViewArm%d"),I)));
        M->SetupAttachment(Crew->GetRootComponent());M->SetStaticMesh(Parts[I]->GetStaticMesh());
        M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetOnlyOwnerSee(true);M->SetCastShadow(false);
        M->SetCanEverAffectNavigation(false);Crew->AddInstanceComponent(M);M->RegisterComponent();ViewArms.Add(M);
    }
}
void UCalmyCrewPresentation::ResetBodies()
{
    if(Parts.Num()!=10) return;
    for(int32 I=0;I<10;++I)
    {
        Parts[I]->SetWorldLocationAndRotation(Crew->GetActorTransform().TransformPosition(CrewRig::Bind[I]),Crew->GetActorQuat(),false,nullptr,ETeleportType::TeleportPhysics);
        // An attached kinematic root may already have this scene transform from
        // moving the capsule, so SetWorldLocation skips its physics update. Flush
        // the old kinematic target explicitly: otherwise it sweeps across the map
        // next substep and tears the reset limbs apart (also relevant to rescue).
        Parts[I]->GetBodyInstance()->SetBodyTransform(Parts[I]->GetComponentTransform(),ETeleportType::TeleportPhysics);
        if(Parts[I]->IsSimulatingPhysics()) { Parts[I]->SetPhysicsLinearVelocity(FVector::ZeroVector);Parts[I]->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
    }
    FallBlend=0;
    LastPosition=Crew->GetActorLocation();
}
void UCalmyCrewPresentation::TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(DT,Type,Fn);UpdatePose(DT);
}
void UCalmyCrewPresentation::UpdatePose(float DT)
{
    if(!Crew || Parts.Num()!=10 || DT<=0) return;
    if(!bRigBound)
    {
        // Runtime-loaded static meshes may create Chaos bodies asynchronously.
        // Binding in BeginPlay while both bodies are invalid creates NO joint.
        for(const auto& P:Parts) if(!P->GetBodyInstance()->IsValidBodyInstance()) return;
        ResetBodies();
        for(int32 I=1;I<10;++I)
        {
            Joints[I-1]->SetWorldLocation(Crew->GetActorTransform().TransformPosition(CrewRig::Bind[I]));
            Joints[I-1]->SetConstrainedComponents(Parts[CrewRig::Parent[I]],NAME_None,Parts[I],NAME_None);
            if(!Joints[I-1]->ConstraintInstance.IsValidConstraintInstance()) return;
        }
        bRigBound=true;UE_LOG(LogTemp,Display,TEXT("CREW_RIG_READY joints=9"));
    }
    if(FVector::Distance(LastPosition,Crew->GetActorLocation())>350 || !HasFiniteBodies()) ResetBodies();
    LastPosition=Crew->GetActorLocation();
    const bool Down=Crew->Lifecycle==ECalmyPlayerLifecycleState::KnockedDown;
    const bool Recover=Crew->Lifecycle==ECalmyPlayerLifecycleState::Recovering;
    UpdatePhysicalRoot(DT);
    const float Speed=FMath::Clamp(Crew->GetVelocity().Size2D()/450.f,0.f,1.f);
    const bool Grounded=Crew->GetCharacterMovement()->IsMovingOnGround();
    const FVector LocalVelocity=Crew->GetActorQuat().UnrotateVector(Crew->GetVelocity());
    Phase=FMath::Fmod(Phase+FMath::Min(DT,.1f)*Crew->GetVelocity().Size2D()*(2*PI/200.f),2*PI);
    MoveBlend=FMath::FInterpTo(MoveBlend,Grounded?Speed:0.f,DT,9.f);
    const auto* Helper=Cast<ACalmyCrewHelper>(Crew);
    const bool Helping=Helper && Helper->Assistance==ECalmyHelperState::Assisting;
    CarryBlend=FMath::FInterpTo(CarryBlend,(Crew->HeldCargo||Helping)?1.f:0.f,DT,7.f);
    // Guided torso must not outrun the physical hips in a single frame.
    FallBlend=FMath::FInterpTo(FallBlend,Down?1.f:0.f,DT,Recover?3.f:3.5f);
    const float S=FMath::Sin(Phase),Bob=(1-FMath::Cos(Phase*2))*.55f*MoveBlend*(1-FallBlend);
    FRotator Lean(-LocalVelocity.X*.009f-CarryBlend*3.f-FallBlend*55.f,0,FMath::Clamp(LocalVelocity.Y*.012f,-4.f,4.f));
    FVector Root=CrewRig::Bind[0]+FVector(FallBlend*-10.f,0,Bob-FallBlend*28.f);
    if(!bPhysicalFall) Parts[0]->SetWorldLocationAndRotation(Crew->GetActorTransform().TransformPosition(Root),Crew->GetActorQuat()*Lean.Quaternion(),false,nullptr,ETeleportType::None);
    const bool Swimming=Crew->IsInOcean();
    for(int32 I=1;I<10;++I)
    {
        FRotator Pose=FRotator::ZeroRotator;
        const float Sign=(I==2||I==3||I==6||I==7)?1.f:-1.f;
        if(I==1) Pose=FRotator(0,FMath::Sin(Phase*.35f)*5.f,FMath::Sin(Phase*.8f)*3.f);
        else if(I==2||I==4) Pose=FRotator(FMath::Lerp(Swimming?45.f+S*Sign*55.f:-S*Sign*27.f*MoveBlend,65.f,CarryBlend),0,Sign*(Swimming?35.f:8.f));
        else if(I==3||I==5) Pose=FRotator(FMath::Lerp(15.f+FMath::Max(0.f,S*Sign)*20.f*MoveBlend,28.f,CarryBlend),0,0);
        else if(Swimming) Pose=FRotator((I==6||I==8)?S*Sign*18.f:-FMath::Max(0.f,S*Sign)*25.f,0,0);
        else
        {
            const float LegPhase=FMath::Fmod(Phase+(Sign<0?PI:0),2*PI);
            const float Cycle=LegPhase/PI;
            const float Stride=FMath::Lerp(18.f,30.f,Speed)*MoveBlend;
            const float Forward=(Cycle<1?1-2*Cycle:2*Cycle-3)*Stride*(LocalVelocity.X<0?-1.f:1.f);
            const float Lift=Cycle>1?FMath::Sin((Cycle-1)*PI)*12.f*MoveBlend:0.f;
            FVector Foot=Crew->GetActorTransform().TransformPosition(FVector(Forward,Sign>0?-12:12,-88));
            FHitResult Floor;FCollisionQueryParams Q(SCENE_QUERY_STAT(CrewFootPlant),false,Crew);
            float LegDown=80.f-Lift;
            if(Grounded && GetWorld()->LineTraceSingleByChannel(Floor,Foot+FVector(0,0,48),Foot-FVector(0,0,40),ECC_WorldStatic,Q) && Floor.ImpactNormal.Z>.7)
                LegDown=FMath::Clamp(Crew->GetActorLocation().Z-6.f-Floor.ImpactPoint.Z-Lift,64.f,81.f);
            const auto Leg=FCalmyLegPose::Solve(Forward,LegDown);
            Pose=FRotator((I==6||I==8)?Leg.Hip:Leg.Knee,0,(I==6||I==8)?FMath::Clamp(LocalVelocity.Y*.02f,-8.f,8.f):0.f);
        }
        // Protective arm bend remains motor-driven while the torso falls freely.
        if(Down) Pose=FRotator((I==3||I==5)?-45.f:0.f,0,0);
        if(Recover) Pose=FRotator::ZeroRotator;
        Joints[I-1]->SetAngularOrientationTarget(Pose);
        Joints[I-1]->SetAngularDriveParams(Down?18.f:(Recover?300.f:1200.f),Down?8.f:90.f,0);
    }
    UpdateCarryArms(DT);
    UpdateViewArms(DT);
    LastLifecycle=static_cast<float>(Crew->Lifecycle);
}
void UCalmyCrewPresentation::UpdateCarryArms(float DT)
{
    WorldHandContactError=0;
    const auto* Helper=Cast<ACalmyCrewHelper>(Crew);
    const auto* Cargo=IsValid(Crew->HeldCargo)?Crew->HeldCargo.Get():
        (Helper && IsValid(Helper->Leader) && Helper->Assistance==ECalmyHelperState::Assisting?Helper->Leader->HeldCargo.Get():nullptr);
    const float Punch = Crew->GetPunchBlend();
    if((!IsValid(Cargo) && Punch<=0.f) || !Crew->IsControlled()) return;
    const bool FarSide=Helper!=nullptr;
    // These are target orientations for the real Chaos limbs, not replacement
    // hidden meshes. Unreal's positive pitch pointed the old fixed pose backwards.
    for(int32 Side=0;Side<2;++Side)
    {
        if(!IsValid(Cargo) && Side!=1) continue;
        const int32 Upper=2+Side*2,Lower=Upper+1;
        const FVector BindUpper=CrewRig::Bind[Lower]-CrewRig::Bind[Upper];
        const FVector Shoulder=Parts[0]->GetComponentTransform().TransformPosition(CrewRig::Bind[Upper]-CrewRig::Bind[0]);
        const FVector Target=IsValid(Cargo)?Cargo->GetGripPoint(Side==1,FarSide):
            FMath::Lerp(Shoulder+Crew->GetActorQuat().RotateVector(FVector(20,3,-55)),
                Shoulder+Crew->GetBaseAimRotation().Vector()*63.f,Punch);
        const FVector Reach=Shoulder+(Target-Shoulder).GetClampedToMaxSize(BindUpper.Size()+34.f-.1f);
        const FVector Pole=Crew->GetActorQuat().RotateVector(FVector(0,Side?.35f:-.35f,-1));
        const FVector Elbow=FCalmyArmIK::Elbow(Shoulder,Reach,Pole,BindUpper.Size(),34.f);
        const FQuat RootQ=Parts[0]->GetComponentQuat();
        const FQuat UpperQ=RootQ*FQuat::FindBetweenNormals(BindUpper.GetSafeNormal(),RootQ.UnrotateVector(Elbow-Shoulder).GetSafeNormal());
        const FQuat LowerQ=UpperQ*FQuat::FindBetweenNormals(FVector(0,0,-1),UpperQ.UnrotateVector(Reach-Elbow).GetSafeNormal());
        // Constraint was bound as body1=parent, body2=child. Chaos drive wants
        // frame1 relative to frame2 (inverse of our child-in-parent IK pose).
        const FRotator Curl=IsValid(Cargo)?FCalmyCrewHand::CarryFingerCurl(Side==1):FRotator::ZeroRotator;
        Joints[Upper-1]->SetAngularOrientationTarget((UpperQ.Inverse()*RootQ).Rotator());
        Joints[Lower-1]->SetAngularOrientationTarget(((LowerQ.Inverse()*UpperQ)*Curl.Quaternion()).Rotator());
        Joints[Upper-1]->SetAngularDriveParams(12000,240,0);
        Joints[Lower-1]->SetAngularDriveParams(12000,240,0);
        const FVector Actual=FCalmyCrewHand::ContactWorld(Parts[Lower],true);
        if(IsValid(Cargo)) WorldHandContactError=FMath::Max(WorldHandContactError,static_cast<float>(FVector::Distance(Actual,Target)));
    }
}

void UCalmyCrewPresentation::UpdateViewArms(float DT)
{
    if(ViewArms.Num()!=4) return;
    if(bPhysicalFall)
    {
        // Owner sees the actual falling arms, not a separate standing IK pose.
        for(int32 I=0;I<4;++I) ViewArms[I]->SetWorldTransform(Parts[I+2]->GetComponentTransform());
        return;
    }
    const FTransform T=Crew->GetActorTransform();HandContactError=0;
    for(int32 Side=0;Side<2;++Side)
    {
        const float Sign=Side?1.f:-1.f;
        // A forward shoulder pose while carrying keeps the chest camera clear.
        const FVector Shoulder=T.TransformPosition(FVector(Crew->HeldCargo?22:3,Sign*25,(Crew->HeldCargo?30:32)-FallBlend*20));
        // Relaxed low hands at rest, not two large fists permanently covering the view.
        FVector Target=T.TransformPosition(FVector(40+FMath::Sin(Phase)*Sign*4*MoveBlend,Sign*26,-15));
        if(Crew->IsInOcean()) Target=T.TransformPosition(FVector(44+FMath::Sin(Phase)*Sign*12,Sign*(25+FMath::Cos(Phase)*8),8));
        if(Crew->HeldCargo)
        {
            Target=Crew->HeldCargo->GetGripPoint(Side==1);
        }
        else if (Side == 1)
        {
            const FVector Fist = Shoulder + Crew->GetBaseAimRotation().Vector() * 62.f;
            Target = FMath::Lerp(Target, Fist, Crew->GetPunchBlend());
        }
        const FVector Reach=Shoulder+(Target-Shoulder).GetClampedToMaxSize(63.9f);
        const FVector Elbow=FCalmyArmIK::Elbow(Shoulder,Reach,T.TransformVectorNoScale(FVector(0,Sign*.4f,-1)));
        auto Place=[&](UStaticMeshComponent* M,const FVector& A,const FVector& B,float Base)
        {
            const FVector D=B-A;
            const FQuat Q=T.GetRotation()*FQuat::FindBetweenNormals(FVector(0,0,-1),T.InverseTransformVectorNoScale(D).GetSafeNormal());
            M->SetWorldLocationAndRotation(A,Q);M->SetWorldScale3D(FVector(1,1,D.Size()/Base));
            M->SetVisibility(!Cast<ACalmyCrewHelper>(Crew));
        };
        Place(ViewArms[Side*2],Shoulder,Elbow,27);
        Place(ViewArms[Side*2+1],Elbow,Reach,34);
        if(Crew->HeldCargo) HandContactError=FMath::Max(HandContactError,static_cast<float>(FVector::Distance(Reach,Target)));
    }
}
void UCalmyCrewPresentation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCalmyCrewPresentation,FallRootPosition);
    DOREPLIFETIME(UCalmyCrewPresentation,FallRootRotation);
    DOREPLIFETIME(UCalmyCrewPresentation,FallCapsulePosition);
    DOREPLIFETIME(UCalmyCrewPresentation,FallEpoch);
}

FTransform UCalmyCrewPresentation::GetChestTransform() const
{
    return Parts.Num()==10?Parts[0]->GetComponentTransform():GetOwner()->GetActorTransform();
}

void UCalmyCrewPresentation::UpdatePhysicalRoot(float DT)
{
    const bool Falling=Crew->Lifecycle==ECalmyPlayerLifecycleState::KnockedDown ||
        (bPhysicalFall && Crew->Lifecycle==ECalmyPlayerLifecycleState::Recovering);
    if(Falling && !bPhysicalFall)
    {
        bPhysicalFall=true;RecoveryAge=0;
        // Async-created simulated static meshes may still be attached. Moving
        // the recovery capsule must not teleport those limbs away from the root.
        for(auto& P:Parts) P->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Crew->GetCharacterMovement()->DisableMovement();
        Crew->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if(Crew->HasAuthority())
        {
            Parts[0]->SetSimulatePhysics(true);
            for(auto& P:Parts)
            {
                P->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
                P->SetUseCCD(true);
                P->SetPhysicsLinearVelocity(FallVelocity);
            }
            Parts[0]->SetPhysicsAngularVelocityInRadians(Crew->GetActorRightVector()*2.2f);
            FallEpoch=Crew->KnockdownCount;
        }
        UE_LOG(LogTemp,Display,TEXT("CREW_PHYSICAL_FALL authority=%d epoch=%d"),Crew->HasAuthority(),Crew->KnockdownCount);
    }
    if(!Falling && bPhysicalFall)
    {
        Parts[0]->SetSimulatePhysics(false);
        Parts[0]->AttachToComponent(Crew->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
        for(auto& P:Parts)
        {
            P->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
            P->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
        }
        Crew->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Crew->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        bPhysicalFall=false;FallBlend=0;
        // Snap to authored binds after the recovery motor; NDOP contacts on larger
        // meshes (CrewR8) may stop a few centimetres short of the tight thresholds.
        ResetBodies();
    }
    if(!bPhysicalFall) return;
    if(Crew->HasAuthority())
    {
        if(Crew->Lifecycle==ECalmyPlayerLifecycleState::KnockedDown)
        {
            if(const auto* Ocean=ACalmyOcean::Find(GetWorld())) for(auto& P:Parts)Ocean->ApplyBuoyancy(P);
            // Capsule is a non-colliding network anchor, not an upright wall
            // preventing the body from rolling. No position reset / home teleport.
            Crew->SetActorLocation(Parts[0]->GetComponentLocation()-CrewRig::Bind[0]);
        }
        else
        {
            RecoveryAge+=DT;
            const FVector Target=Crew->GetActorTransform().TransformPosition(CrewRig::Bind[0]);
            const FVector Error=Target-Parts[0]->GetComponentLocation();
            // Slightly stronger pull than the R6B-tuned motor so larger NDOP shells can settle.
            const FVector Accel=Error*140.f-Parts[0]->GetPhysicsLinearVelocity()*28.f+FVector(0,0,980.f*52.f/24.f);
            Parts[0]->WakeRigidBody();
            Parts[0]->AddForce(Accel.GetClampedToMaxSize(4500),NAME_None,true);
            FQuat ErrorQ=Crew->GetActorQuat()*Parts[0]->GetComponentQuat().Inverse();
            if(ErrorQ.W<0) ErrorQ=ErrorQ*-1.f;
            FVector Axis;float Angle;ErrorQ.ToAxisAndAngle(Axis,Angle);
            Parts[0]->AddTorqueInRadians((Axis*Angle*200.f-Parts[0]->GetPhysicsAngularVelocityInRadians()*28.f).GetClampedToMaxSize(1200),NAME_None,true);
        }
        FallRootPosition=Parts[0]->GetComponentLocation();FallRootRotation=Parts[0]->GetComponentRotation();
        FallCapsulePosition=Crew->GetActorLocation();
    }
    else if(FallEpoch==Crew->KnockdownCount && FallEpoch>0)
    {
        // Root comes only from the server; client limbs cannot move gameplay.
        Crew->SetActorLocation(FallCapsulePosition);
        Parts[0]->SetWorldLocationAndRotation(FMath::VInterpTo(Parts[0]->GetComponentLocation(),FallRootPosition,DT,20),
            FMath::QInterpTo(Parts[0]->GetComponentQuat(),FallRootRotation.Quaternion(),DT,20));
    }
}

bool UCalmyCrewPresentation::FindRecoveryPosition(FVector& Position) const
{
    if(!bPhysicalFall || !Crew->HasAuthority() || Parts[0]->GetPhysicsLinearVelocity().Size()>(RecoveryAge>2.f?180.f:100.f)) return false;
    // Stand at the fallen body, never teleport sideways through a wall or to home.
    const auto* Capsule=Crew->GetCapsuleComponent();
    return FCalmyCrewRecovery::FindStandingCapsule(GetWorld(),Crew,Parts[0]->GetComponentLocation(),Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight(),Position);
}
bool UCalmyCrewPresentation::PrepareRecovery()
{
    // A body that rolled off the deck must not wait forever for a seabed floor.
    // Hand back to surface swimming once buoyancy has brought its torso up.
    const auto* Ocean=ACalmyOcean::Find(GetWorld());
    if(bPhysicalFall && Crew->HasAuthority() && Ocean)
    {
        const FVector Root=Parts[0]->GetComponentLocation();const float Water=Ocean->HeightAt(Root);
        if(!Ocean->IsInsideDryHull(Root) && Root.Z<Water+10 && Root.Z>Water-90 && Parts[0]->GetPhysicsLinearVelocity().Size()<200)
        {
            Crew->SetActorLocation(FVector(Root.X,Root.Y,Water-20));ResetAfterRescue();
            Crew->GetCharacterMovement()->SetMovementMode(MOVE_Custom,1);
            UE_LOG(LogTemp,Display,TEXT("CREW_FALL_WATER_HANDOFF no_rescue=1"));return true;
        }
    }
    FVector Position;if(!FindRecoveryPosition(Position)) return false;
    Crew->SetActorLocation(Position);RecoveryAge=0;
    Crew->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    // Floor NDOP contacts on larger authored meshes fight the recovery motor; keep
    // limb-limb blocking but let the torso rotate/translate toward the bind.
    for(auto& P:Parts)
    {
        P->WakeRigidBody();
        P->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Overlap);
    }
    return true;
}
bool UCalmyCrewPresentation::CanFinishRecovery() const
{
    if(!bPhysicalFall) return true;
    FVector Position;
    if(!FindRecoveryPosition(Position)) return false;
    const float Dist=FVector::Distance(Parts[0]->GetComponentLocation(),Crew->GetActorTransform().TransformPosition(CrewRig::Bind[0]));
    const float Deg=FMath::RadiansToDegrees(Parts[0]->GetComponentQuat().AngularDistance(Crew->GetActorQuat()));
    // Tight R6B thresholds first.
    if(RecoveryAge>.7f && Dist<5.f && Deg<12.f) return true;
    // Bounded soft finish: valid stand pose after motor window (CrewR8 NDOP shells).
    if(RecoveryAge>2.f && Dist<28.f && Deg<40.f)
    {
        UE_LOG(LogTemp,Display,TEXT("CREW_RECOVERY_SOFT_FINISH age=%.2f dist_cm=%.2f deg=%.1f"),RecoveryAge,Dist,Deg);
        return true;
    }
    return false;
}
void UCalmyCrewPresentation::ResetAfterRescue()
{
    if(Parts.Num()!=10) return;
    Parts[0]->SetSimulatePhysics(false);
    Parts[0]->AttachToComponent(Crew->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
    bPhysicalFall=false;
    for(auto& P:Parts) P->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
    Crew->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ResetBodies();
}
int32 UCalmyCrewPresentation::SimulatedPartCount() const
{
    int32 N=0;for(const auto& P:Parts) if(P && P->IsSimulatingPhysics()) ++N;return N;
}
bool UCalmyCrewPresentation::HasFiniteBodies() const
{
    for(const auto& P:Parts) if(!P || P->GetComponentLocation().ContainsNaN() || P->GetComponentRotation().ContainsNaN() ||
        P->GetPhysicsLinearVelocity().ContainsNaN() || P->GetPhysicsAngularVelocityInRadians().ContainsNaN()) return false;
    return Parts.Num()==10;
}
float UCalmyCrewPresentation::MaximumJointSeparation() const
{
    float Max=0;
    for(int32 I=1;I<Parts.Num();++I)
    {
        const FVector Offset=CrewRig::Bind[I]-CrewRig::Bind[CrewRig::Parent[I]];
        const FVector Expected=Parts[CrewRig::Parent[I]]->GetComponentTransform().TransformPosition(Offset);
        Max=FMath::Max(Max,static_cast<float>(FVector::Distance(Expected,Parts[I]->GetComponentLocation())));
        if(FVector::Distance(Expected,Parts[I]->GetComponentLocation())>8)
            UE_LOG(LogTemp,Warning,TEXT("CREW_JOINT %d parent=%s child=%s expected=%s parentrot=%s joint=%s"),I,
                *Parts[CrewRig::Parent[I]]->GetComponentLocation().ToString(),*Parts[I]->GetComponentLocation().ToString(),*Expected.ToString(),
                *Parts[CrewRig::Parent[I]]->GetComponentRotation().ToString(),*Joints[I-1]->GetComponentLocation().ToString());
    }
    return Max;
}
