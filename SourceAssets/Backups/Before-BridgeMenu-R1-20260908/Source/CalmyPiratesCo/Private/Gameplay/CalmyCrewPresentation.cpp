#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Core/CrewGait.h"
#include "Core/CrewArmIK.h"
#include "Gameplay/CalmyCargo.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Engine/StaticMesh.h"

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
}
void UCalmyCrewPresentation::BeginPlay()
{
    Super::BeginPlay();Crew=Cast<ACalmyCharacter>(GetOwner());
    if(!Crew || GetNetMode()==NM_DedicatedServer) return;
    for(int32 I=0;I<10;++I)
    {
        auto* M=NewObject<UStaticMeshComponent>(Crew,FName(*FString::Printf(TEXT("Deckhand_%s"),CrewRig::Names[I])));
        M->SetupAttachment(Crew->GetRootComponent());M->SetMobility(EComponentMobility::Movable);
        const TCHAR* Variant=Cast<ACalmyCrewHelper>(Crew)?TEXT("Misha"):TEXT("Deckhand");
        M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/CrewNightwatchR5/SM_%s_%s.SM_%s_%s"),Variant,CrewRig::Names[I],Variant,CrewRig::Names[I])));
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
        if(I>0) { Parts[I]->SetPhysicsLinearVelocity(FVector::ZeroVector);Parts[I]->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
    }
    FallBlend=0;
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
    Parts[0]->SetWorldLocationAndRotation(Crew->GetActorTransform().TransformPosition(Root),Crew->GetActorQuat()*Lean.Quaternion(),false,nullptr,ETeleportType::None);
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
        if(Down || Recover) Pose=FRotator::ZeroRotator;
        Joints[I-1]->SetAngularOrientationTarget(Pose);
        Joints[I-1]->SetAngularDriveParams(Down?18.f:(Recover?300.f:1200.f),Down?8.f:90.f,0);
        if(Down && LastLifecycle!=static_cast<float>(Crew->Lifecycle))
            Parts[I]->AddImpulse(FVector(60,Sign*90,100),NAME_None,true);
    }
    UpdateViewArms(DT);
    LastLifecycle=static_cast<float>(Crew->Lifecycle);
}
void UCalmyCrewPresentation::UpdateViewArms(float DT)
{
    if(ViewArms.Num()!=4) return;
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
            const FTransform C=Crew->HeldCargo->GetActorTransform();
            Target=C.TransformPosition(FVector(-48,Sign*30,12));
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
        Place(ViewArms[Side*2+1],Elbow,Reach,29);
        if(Crew->HeldCargo) HandContactError=FMath::Max(HandContactError,static_cast<float>(FVector::Distance(Reach,Target)));
    }
}
int32 UCalmyCrewPresentation::SimulatedPartCount() const
{
    int32 N=0;for(const auto& P:Parts) if(P && P->IsSimulatingPhysics()) ++N;return N;
}
bool UCalmyCrewPresentation::HasFiniteBodies() const
{
    for(const auto& P:Parts) if(!P || P->GetComponentLocation().ContainsNaN() || P->GetPhysicsLinearVelocity().ContainsNaN()) return false;
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
