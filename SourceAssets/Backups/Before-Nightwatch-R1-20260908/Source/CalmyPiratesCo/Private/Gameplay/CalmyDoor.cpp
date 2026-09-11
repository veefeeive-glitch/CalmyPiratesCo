#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
ACalmyDoor::ACalmyDoor()
{
    bReplicates=true;PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(30);bAlwaysRelevant=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));Hinge->SetupAttachment(RootComponent);
    Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("LeafCollision"));Collision->SetupAttachment(Hinge);Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Leaf=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaf"));Leaf->SetupAttachment(Hinge);Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleFront=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleFront"));HandleFront->SetupAttachment(Hinge);HandleFront->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleBack=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleBack"));HandleBack->SetupAttachment(Hinge);HandleBack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cube.Succeeded()){Leaf->SetStaticMesh(Cube.Object);HandleFront->SetStaticMesh(Cube.Object);HandleBack->SetStaticMesh(Cube.Object);}
}
void ACalmyDoor::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);WidthCm=FMath::Clamp(WidthCm,80.f,160.f);HeightCm=FMath::Clamp(HeightCm,190.f,230.f);
    Collision->SetBoxExtent(FVector(WidthCm*.5f,3.f,HeightCm*.5f));Collision->SetRelativeLocation(FVector(-WidthCm*.5f,0,HeightCm*.5f+2));
    Leaf->SetRelativeLocation(Collision->GetRelativeLocation());Leaf->SetRelativeScale3D(FVector(WidthCm/100.f,.055f,HeightCm/100.f));
    HandleFront->SetRelativeLocation(FVector(-WidthCm+15,-6,105));HandleBack->SetRelativeLocation(FVector(-WidthCm+15,6,105));
    HandleFront->SetRelativeScale3D(FVector(.16,.035,.03));HandleBack->SetRelativeScale3D(FVector(.16,.035,.03));
    Angle=bStartOpen?OpenAngle:0.f;ApplyPose();
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
void ACalmyDoor::BeginPlay(){Super::BeginPlay();if(HasAuthority()){Angle=bStartOpen?OpenAngle:0.f;TargetAngle=Angle;State=bStartOpen?ECalmyDoorState::Open:ECalmyDoorState::Closed;}ApplyPose();}
void ACalmyDoor::ApplyPose(){Hinge->SetRelativeRotation(FRotator(0,Angle*SwingSign,0));}
void ACalmyDoor::OnRep_Pose(){ApplyPose();}
void ACalmyDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ACalmyDoor,DoorId);DOREPLIFETIME(ACalmyDoor,Angle);DOREPLIFETIME(ACalmyDoor,Revision);DOREPLIFETIME(ACalmyDoor,State);}
ACalmyDoor* ACalmyDoor::FindTarget(const ACalmyCharacter* P)
{
    if(!P||!P->IsControlled()||P->HeldCargo||P->IsInOcean())return nullptr;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorAim),true,P);
    if(P->GetWorld()->LineTraceSingleByChannel(Hit,P->GetInteractionOrigin(),P->GetInteractionOrigin()+P->GetBaseAimRotation().Vector()*220.f,ECC_Visibility,Q))return Cast<ACalmyDoor>(Hit.GetActor());
    return nullptr;
}
bool ACalmyDoor::RequestToggle(ACalmyCharacter* P,uint32 Expected)
{
    if(!HasAuthority()||Expected!=Revision||FindTarget(P)!=this||State==ECalmyDoorState::Opening||State==ECalmyDoorState::Closing)return false;
    TargetAngle=State==ECalmyDoorState::Blocked?(TargetAngle>0?0:OpenAngle):(IsOpen()?0:OpenAngle);
    State=TargetAngle>0?ECalmyDoorState::Opening:ECalmyDoorState::Closing;++Revision;ForceNetUpdate();return true;
}
bool ACalmyDoor::PoseBlocked(float Degrees)const
{
    const FQuat Rotation=GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Degrees*SwingSign));
    const FVector Center=GetActorLocation()+Rotation.RotateVector(FVector(-WidthCm*.5f,0,HeightCm*.5f+2));
    FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorSweep),true,this);
    return GetWorld()->OverlapBlockingTestByChannel(Center,Rotation,ECC_Pawn,FCollisionShape::MakeBox(FVector(WidthCm*.5f-1,2.5f,HeightCm*.5f-1)),Q);
}
void ACalmyDoor::Tick(float DT)
{
    Super::Tick(DT);if(!HasAuthority()||(State!=ECalmyDoorState::Opening&&State!=ECalmyDoorState::Closing))return;
    float Remaining=FMath::Min(FMath::Max(DT,0.f),.1f)*90.f;
    while(Remaining>KINDA_SMALL_NUMBER){float Next=FMath::FInterpConstantTo(Angle,TargetAngle,1,FMath::Min(2.f,Remaining));
        if(PoseBlocked(Next)){State=ECalmyDoorState::Blocked;++Revision;ForceNetUpdate();return;}
        Angle=Next;ApplyPose();Remaining-=2.f;
        if(FMath::IsNearlyEqual(Angle,TargetAngle)){State=TargetAngle>0?ECalmyDoorState::Open:ECalmyDoorState::Closed;ForceNetUpdate();break;}}
}
