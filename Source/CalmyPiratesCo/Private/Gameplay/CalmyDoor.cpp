#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Core/CalmyAudio.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "UObject/ConstructorHelpers.h"

namespace CalmyDoorVisual
{
	// SM_WatertightDoor kit: body 118×16×210 cm, pivot floor-center of leaf.
	constexpr float KitWidthCm=118.f;
	constexpr float KitHeightCm=210.f;
	constexpr float KitThicknessCm=16.f;
	// R7 openings are larger than doors.json; oversize + jamb/sill embed kills Nightwatch light leaks.
	constexpr float SealOversizeCm=24.f;
	constexpr float VisualThicknessCm=10.f;
	// Negative Z seats the kit pivot into the sill (V1 +4 left a floor gap on dark frames).
	constexpr float FloorNudgeCm=-3.f;
	// Embed past hinge into jamb so the closed leaf doesn't leave a dark hinge-side slit.
	constexpr float HingeEmbedCm=8.f;
}

ACalmyDoor::ACalmyDoor()
{
    bReplicates=true;PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(30);bAlwaysRelevant=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));Hinge->SetupAttachment(RootComponent);
    Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("LeafCollision"));Collision->SetupAttachment(Hinge);Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Leaf=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaf"));Leaf->SetupAttachment(Hinge);Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleFront=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleFront"));HandleFront->SetupAttachment(Hinge);HandleFront->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleBack=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleBack"));HandleBack->SetupAttachment(Hinge);HandleBack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Watertight(TEXT("/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/StaticMeshes/SM_WatertightDoor.SM_WatertightDoor"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Watertight.Succeeded())
    {
        Leaf->SetStaticMesh(Watertight.Object);
        bWatertightLeaf=true;
    }
    else if(Cube.Succeeded())
    {
        Leaf->SetStaticMesh(Cube.Object);
    }
    if(Cube.Succeeded()){HandleFront->SetStaticMesh(Cube.Object);HandleBack->SetStaticMesh(Cube.Object);}
}
void ACalmyDoor::ApplyDoorMaterials()
{
    if(!Leaf)return;
    // Drop map-era blue cube overrides; keep kit slots or ship working metals.
    if(bWatertightLeaf)
    {
        if(UMaterialInterface* Blue=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_EquipmentBlue.MAT_Interior_EquipmentBlue")))
            Leaf->SetMaterial(0,Blue);
        if(UMaterialInterface* Dark=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Machinery.MAT_Interior_Machinery")))
            if(Leaf->GetNumMaterials()>1)Leaf->SetMaterial(1,Dark);
        if(UMaterialInterface* Safety=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Safety.MAT_Interior_Safety")))
            if(Leaf->GetNumMaterials()>2)Leaf->SetMaterial(2,Safety);
    }
    else if(UMaterialInterface* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Ships/OceanFreighterR7/MetalR1B/MI_MAT_Steel_PBR_2.MI_MAT_Steel_PBR_2")))
    {
        Leaf->SetMaterial(0,Steel);
    }
}
void ACalmyDoor::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);WidthCm=FMath::Clamp(WidthCm,80.f,160.f);HeightCm=FMath::Clamp(HeightCm,190.f,230.f);
    if(Leaf&&Leaf->GetStaticMesh()&&Leaf->GetStaticMesh()->GetName().Contains(TEXT("Watertight")))bWatertightLeaf=true;
    Collision->SetBoxExtent(FVector(WidthCm*.5f,3.f,HeightCm*.5f));Collision->SetRelativeLocation(FVector(-WidthCm*.5f,0,HeightCm*.5f+2));
    if(bWatertightLeaf)
    {
        using namespace CalmyDoorVisual;
        const float VisualW=WidthCm+SealOversizeCm;
        const float VisualH=HeightCm+SealOversizeCm;
        Leaf->SetRelativeScale3D(FVector(VisualW/KitWidthCm,VisualThicknessCm/KitThicknessCm,VisualH/KitHeightCm));
        // Hinge at free-edge X=0; embed into jamb + floor nudge to kill light leaks.
        Leaf->SetRelativeLocation(FVector(-VisualW*.5f+HingeEmbedCm,0.f,FloorNudgeCm));
        Leaf->SetRelativeRotation(FRotator::ZeroRotator);
        HandleFront->SetVisibility(false);HandleBack->SetVisibility(false);
        HandleFront->SetHiddenInGame(true);HandleBack->SetHiddenInGame(true);
        HandleFront->SetRelativeScale3D(FVector(.001f));HandleBack->SetRelativeScale3D(FVector(.001f));
    }
    else
    {
        Leaf->SetRelativeLocation(Collision->GetRelativeLocation());Leaf->SetRelativeScale3D(FVector(WidthCm/100.f,.055f,HeightCm/100.f));
        HandleFront->SetVisibility(true);HandleBack->SetVisibility(true);
        HandleFront->SetHiddenInGame(false);HandleBack->SetHiddenInGame(false);
        HandleFront->SetRelativeLocation(FVector(-WidthCm+15,-6,105));HandleBack->SetRelativeLocation(FVector(-WidthCm+15,6,105));
        HandleFront->SetRelativeScale3D(FVector(.16,.035,.03));HandleBack->SetRelativeScale3D(FVector(.16,.035,.03));
    }
    ApplyDoorMaterials();
    Angle=bStartOpen?OpenAngle:0.f;ApplyPose(Angle);
}
bool ACalmyDoor::FitSwingToWorld()
{
    const FTransform Original=GetActorTransform();
    const float BaseSign=FMath::Abs(GetActorRotation().Yaw)<45?1.f:-1.f;
    // Try both hinge jambs and both wall faces. Prefer full fold-back before 90-degree swing.
    for(float Limit:{180.f,90.f})for(int32 Side=0;Side<2;++Side)for(int32 Flip=0;Flip<2;++Flip)
    {
        const FVector Offset(-WidthCm*Flip,Side?26.f*BaseSign:0.f,0);
        SetActorLocationAndRotation(Original.TransformPosition(Offset),Original.Rotator()+FRotator(0,Flip*180.f,0));
        SwingSign=BaseSign*(Side?-1.f:1.f)*(Flip?-1.f:1.f);OpenAngle=Limit;
        bool Clear=true;for(float A=0;A<=Limit;A+=1)if(PoseBlocked(A)){Clear=false;break;}
        if(Clear){RebuildGeometry();return true;}
    }
    SetActorTransform(Original);SwingSign=BaseSign;OpenAngle=180;RebuildGeometry();return false;
}
void ACalmyDoor::RebuildGeometry(){OnConstruction(GetActorTransform());}
void ACalmyDoor::BeginPlay()
{
    Super::BeginPlay();
    if(HasAuthority()){SnapTo(bStartOpen?OpenAngle:0.f);State=bStartOpen?ECalmyDoorState::Open:ECalmyDoorState::Closed;}
    DisplayAngle=Angle;
    LastHeardState=State;
    HeardImpactSerial=ImpactSerial;
    ApplyDoorMaterials();
    ApplyPose(Angle);
}
void ACalmyDoor::ApplyPose(float Degrees){Hinge->SetRelativeRotation(FRotator(0,Degrees*SwingSign,0));}
FVector ACalmyDoor::GetGripLocation() const
{
    return GetActorTransform().TransformPosition(FRotator(0,DisplayAngle*SwingSign,0).RotateVector(FVector(-WidthCm*.85f,0,105)));
}
float ACalmyDoor::PushFromMouse(const FVector& ViewForward,const FVector& ViewRight,float MouseX,float MouseY,const FVector& HingeToGrip,float SwingSign)
{
    // Moving the mouse "forward" pushes along the view, sideways along the view's right; only the component along
    // the leaf's swing direction at the handle turns it.
    const FVector Tangent=FVector(-HingeToGrip.Y,HingeToGrip.X,0).GetSafeNormal()*FMath::Sign(SwingSign==0?1.f:SwingSign);
    const FVector Drag=FVector(ViewForward.X,ViewForward.Y,0).GetSafeNormal()*MouseY+FVector(ViewRight.X,ViewRight.Y,0).GetSafeNormal()*MouseX;
    return FMath::Clamp(float(FVector::DotProduct(Drag,Tangent))*.08f,-1.f,1.f);
}
float ACalmyDoor::ServerClock() const
{
    const auto* GS=GetWorld()?GetWorld()->GetGameState():nullptr;
    return GS?float(GS->GetServerWorldTimeSeconds()):(GetWorld()?GetWorld()->GetTimeSeconds():0.f);
}
void ACalmyDoor::Wake(){SettledSeconds=0;SetActorTickEnabled(true);}
void ACalmyDoor::SnapTo(float Degrees)
{
    Motion.Angle=FMath::Clamp(Degrees,0.f,OpenAngle);Motion.Velocity=0;bAuto=false;
    Angle=DisplayAngle=Motion.Angle;AngularVelocity=0;ApplyPose(Angle);
    State=Angle<=0?ECalmyDoorState::Closed:IsOpen()?ECalmyDoorState::Open:ECalmyDoorState::Ajar;
    Wake();ForceNetUpdate();
}
void ACalmyDoor::OnRep_Pose(){Wake();}
void ACalmyDoor::OnRep_State(){LastHeardState=State;Wake();}
void ACalmyDoor::OnRep_Impact()
{
    if(ImpactSerial==HeardImpactSerial)return;
    HeardImpactSerial=ImpactSerial;
    // Late joiners and old updates stay silent.
    if(ServerClock()-ImpactServerTime<1.f)PlayImpact(ImpactKind,ImpactSpeed);
}
void ACalmyDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACalmyDoor,DoorId);DOREPLIFETIME(ACalmyDoor,Angle);DOREPLIFETIME(ACalmyDoor,AngularVelocity);DOREPLIFETIME(ACalmyDoor,Revision);
    DOREPLIFETIME(ACalmyDoor,State);DOREPLIFETIME(ACalmyDoor,Holder);
    DOREPLIFETIME(ACalmyDoor,ImpactSerial);DOREPLIFETIME(ACalmyDoor,ImpactKind);DOREPLIFETIME(ACalmyDoor,ImpactSpeed);DOREPLIFETIME(ACalmyDoor,ImpactServerTime);
}
ACalmyDoor* ACalmyDoor::FindTarget(const ACalmyCharacter* P)
{
    if(!P||!P->IsControlled()||P->HeldCargo||P->IsInOcean())return nullptr;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorAim),true,P);
    if(P->GetWorld()->LineTraceSingleByChannel(Hit,P->GetInteractionOrigin(),P->GetInteractionOrigin()+P->GetBaseAimRotation().Vector()*220.f,ECC_Visibility,Q))return Cast<ACalmyDoor>(Hit.GetActor());
    return nullptr;
}
bool ACalmyDoor::RequestToggle(ACalmyCharacter* P,uint32 Expected)
{
    if(!HasAuthority()||Expected!=Revision||FindTarget(P)!=this)return false;
    if(IsValid(Holder)&&Holder!=P)return false;
    // A blocked leaf reverses; otherwise the tap sends it to whichever end it is further from.
    const bool bOpen=State==ECalmyDoorState::Blocked?AutoTargetAngle<=0:Angle<OpenAngle*.5f;
    bAuto=true;AutoTargetAngle=bOpen?OpenAngle:0.f;Holder=nullptr;
    State=bOpen?ECalmyDoorState::Opening:ECalmyDoorState::Closing;LastHeardState=State;++Revision;
    Wake();ForceNetUpdate();
    return true;
}
void ACalmyDoor::SetAutoTarget(bool bOpen)
{
    if(!HasAuthority())return;
    bAuto=true;AutoTargetAngle=bOpen?OpenAngle:0.f;Holder=nullptr;
    State=bOpen?ECalmyDoorState::Opening:ECalmyDoorState::Closing;LastHeardState=State;++Revision;
    Wake();ForceNetUpdate();
}
bool ACalmyDoor::TryGrab(ACalmyCharacter* User)
{
    if(!HasAuthority()||!IsValid(User)||FindTarget(User)!=this)return false;
    if(IsValid(Holder)&&Holder!=User)return false;
    Holder=User;bAuto=false;DragPush=0;DragLeaseEnd=-1;
    Wake();ForceNetUpdate();
    return true;
}
void ACalmyDoor::ReleaseGrab(ACalmyCharacter* User)
{
    if(!HasAuthority()||Holder!=User)return;
    Holder=nullptr;DragPush=0;DragLeaseEnd=-1;
    Wake();ForceNetUpdate();
}
bool ACalmyDoor::ApplyDrag(ACalmyCharacter* User,float Push)
{
    if(!HasAuthority()||!IsValid(User)||Holder!=User||!FMath::IsFinite(Push))return false;
    DragPush=FMath::Clamp(Push,-1.f,1.f);DragLeaseEnd=GetWorld()->GetTimeSeconds()+DragLeaseSeconds;
    Wake();
    return true;
}
bool ACalmyDoor::PoseBlocked(float Degrees)const
{
    const FQuat Rotation=GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Degrees*SwingSign));
    const FVector Center=GetActorLocation()+Rotation.RotateVector(FVector(-WidthCm*.5f,0,HeightCm*.5f+2));
    FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorSweep),true,this);
    return GetWorld()->OverlapBlockingTestByChannel(Center,Rotation,ECC_Pawn,FCollisionShape::MakeBox(FVector(WidthCm*.5f-1,2.5f,HeightCm*.5f-1)),Q);
}
FString ACalmyDoor::PoseBlocker(float Degrees)const
{
    const FQuat Rotation=GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Degrees*SwingSign));
    const FVector Center=GetActorLocation()+Rotation.RotateVector(FVector(-WidthCm*.5f,0,HeightCm*.5f+2));
    FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorSweep),true,this);
    TArray<FOverlapResult> Hits;
    GetWorld()->OverlapMultiByChannel(Hits,Center,Rotation,ECC_Pawn,FCollisionShape::MakeBox(FVector(WidthCm*.5f-1,2.5f,HeightCm*.5f-1)),Q);
    TArray<FString> Names;
    for(const FOverlapResult& Hit:Hits)
    {
        if(!Hit.bBlockingHit)continue;
        const UPrimitiveComponent* Component=Hit.GetComponent();const auto* Mesh=Cast<UStaticMeshComponent>(Component);
        const FString MeshName=Mesh&&Mesh->GetStaticMesh()?TEXT("/")+Mesh->GetStaticMesh()->GetName():FString();
        Names.AddUnique((Hit.GetActor()?Hit.GetActor()->GetActorNameOrLabel():TEXT("?"))+TEXT("/")+(Component?Component->GetName():TEXT("?"))+MeshName);
    }
    return Names.IsEmpty()?TEXT("none"):FString::Join(Names,TEXT(","));
}
void ACalmyDoor::EmitImpact(ECalmyDoorImpact Kind,float Speed)
{
    ++ImpactSerial;HeardImpactSerial=ImpactSerial;ImpactKind=Kind;ImpactSpeed=Speed;ImpactServerTime=ServerClock();
    PlayImpact(Kind,Speed);ForceNetUpdate();
}
void ACalmyDoor::PlayImpact(ECalmyDoorImpact Kind,float Speed)
{
    const float K=FMath::Clamp(Speed/FCalmyDoorMotion::MaxDragSpeed,0.f,1.f);
    const FVector Edge=GetGripLocation();
    switch(Kind)
    {
    case ECalmyDoorImpact::Latch:FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorLatch,Edge,.5f+.4f*K,1.f-.1f*K);break;
    case ECalmyDoorImpact::Slam:FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorSlam,Edge,.7f+.3f*K,.92f);
        FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorLatch,Edge,.55f,.85f);break;
    case ECalmyDoorImpact::Knock:FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorThud,Edge,.35f+.5f*K,1.f);break;
    case ECalmyDoorImpact::Bump:FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorThud,Edge,.25f+.45f*K,1.12f);break;
    default:break;
    }
}
void ACalmyDoor::Simulate(float DT)
{
    if(Holder&&(!IsValid(Holder)||Holder->GetWorld()!=GetWorld()||!Holder->IsControlled()||Holder->HeldCargo||
        FVector::DistSquared(Holder->GetActorLocation(),GetGripLocation())>FMath::Square(HoldRangeCm)))
    {Holder=nullptr;DragPush=0;DragLeaseEnd=-1;ForceNetUpdate();}
    const bool bDrag=Holder!=nullptr&&GetWorld()->GetTimeSeconds()<DragLeaseEnd;
    if(Holder)bAuto=false;
    FCalmyDoorMotion Next=Motion;float Impact=0;
    const FCalmyDoorMotion::EEvent Event=Next.Step(DT,OpenAngle,bDrag,DragPush,bAuto,AutoTargetAngle,Impact);
    if(!FMath::IsNearlyEqual(Next.Angle,Motion.Angle,.001f)&&PoseBlocked(Next.Angle))
    {
        // Something is in the swing (a player, cargo): the heavy leaf thuds and stops dead against it, and stays
        // Blocked until the next tap or grab instead of rebounding into another state.
        const float Speed=FMath::Abs(Motion.Velocity);
        Motion.Velocity=0;
        if(Speed>15.f)EmitImpact(ECalmyDoorImpact::Bump,Speed);
        if(bAuto){bAuto=false;State=ECalmyDoorState::Blocked;LastHeardState=State;++Revision;ForceNetUpdate();}
    }
    else
    {
        Motion=Next;
        if(Event==FCalmyDoorMotion::EEvent::Latched)EmitImpact(ECalmyDoorImpact::Latch,Impact);
        else if(Event==FCalmyDoorMotion::EEvent::Slammed)EmitImpact(ECalmyDoorImpact::Slam,Impact);
        else if(Event==FCalmyDoorMotion::EEvent::HitOpenStop)EmitImpact(ECalmyDoorImpact::Knock,Impact);
    }
    if(bAuto&&Motion.IsSettled()&&FMath::IsNearlyEqual(Motion.Angle,FMath::Clamp(AutoTargetAngle,0.f,OpenAngle),.05f))bAuto=false;

    ECalmyDoorState NextState=State;
    if(Motion.Velocity>0)NextState=ECalmyDoorState::Opening;
    else if(Motion.Velocity<0)NextState=ECalmyDoorState::Closing;
    else if(Motion.Angle<=0)NextState=ECalmyDoorState::Closed;
    else if(Motion.Angle>=OpenAngle-2.f)NextState=ECalmyDoorState::Open;
    else if(!bAuto&&State!=ECalmyDoorState::Blocked)NextState=ECalmyDoorState::Ajar;
    if(NextState!=State){State=NextState;LastHeardState=State;ForceNetUpdate();}
    if(Angle!=Motion.Angle||AngularVelocity!=Motion.Velocity)
    {
        Angle=DisplayAngle=Motion.Angle;AngularVelocity=Motion.Velocity;ApplyPose(Angle);
        if(Motion.Velocity==0)ForceNetUpdate();
    }
    const bool bIdle=Motion.IsSettled()&&!bAuto&&!Holder;
    SettledSeconds=bIdle?SettledSeconds+DT:0;
    // Resting doors cost nothing: 61 leaves no longer tick every frame.
    if(SettledSeconds>.5f)SetActorTickEnabled(false);
}
void ACalmyDoor::Present(float DT)
{
    const float Goal=FMath::Clamp(Angle+AngularVelocity*.06f,0.f,OpenAngle);
    DisplayAngle=FMath::FInterpTo(DisplayAngle,Goal,DT,16.f);
    ApplyPose(DisplayAngle);
    const bool bIdle=AngularVelocity==0&&FMath::IsNearlyEqual(DisplayAngle,Angle,.05f);
    SettledSeconds=bIdle?SettledSeconds+DT:0;
    if(SettledSeconds>.5f){DisplayAngle=Angle;ApplyPose(Angle);SetActorTickEnabled(false);}
}
void ACalmyDoor::Tick(float DT)
{
    Super::Tick(DT);
    if(HasAuthority())Simulate(DT);else Present(DT);
    // Creak grains follow the swing speed on every machine, so a slow push groans and a fast swing rattles.
    const float Speed=FMath::Abs(HasAuthority()?Motion.Velocity:AngularVelocity);
    CreakCountdown-=DT;
    if(Speed>8.f&&CreakCountdown<=0)
    {
        const float K=FMath::Clamp(Speed/FCalmyDoorMotion::MaxDragSpeed,0.f,1.f);
        FCalmyAudio::PlayAt(this,ECalmySoundCue::DoorCreak,GetGripLocation(),.16f+.5f*K,.82f+.3f*K+FMath::FRandRange(-.04f,.04f));
        CreakCountdown=FMath::Lerp(.28f,.1f,K);
    }
}
