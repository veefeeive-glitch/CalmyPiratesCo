#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyCargo.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

ACalmyCrewHelper::ACalmyCrewHelper(const FObjectInitializer& Init):Super(Init)
{
    AIControllerClass=AAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw=false;bAlwaysRelevant=true;
    GetCharacterMovement()->bOrientRotationToMovement=true;
}
void ACalmyCrewHelper::BeginPlay()
{
    Super::BeginPlay();Station=GetActorLocation();
}
void ACalmyCrewHelper::ReleaseLeader()
{
    if(!HasAuthority()) return;
    if(IsValid(Leader) && Leader->CrewHelper==this) {Leader->CrewHelper=nullptr;Leader->ForceNetUpdate();}
    Leader=nullptr;Trail.Reset();StuckTime=0;FollowDistance=0;
    Assistance=ECalmyHelperState::Waiting;
    ConsumeMovementInputVector();GetCharacterMovement()->StopMovementImmediately();ForceNetUpdate();
}
void ACalmyCrewHelper::ResetToStation()
{
    if(!HasAuthority()) return;
    ReleaseLeader();SetActorLocation(Station,false,nullptr,ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}
void ACalmyCrewHelper::AssignLeader(ACalmyCharacter* Pawn)
{
    if(!HasAuthority() || !IsValid(Pawn) || Pawn==this || Pawn->GetWorld()!=GetWorld() || (IsValid(Leader) && Leader!=Pawn)) return;
    Leader=Pawn;LastLeader=Pawn->GetActorLocation();Previous=GetActorLocation();
    Trail.Reset();Trail.Add(LastLeader);StuckTime=0;Assistance=ECalmyHelperState::Following;ForceNetUpdate();
}
bool ACalmyCrewHelper::CanAssist(const ACalmyCharacter* Pawn) const
{
    return Leader==Pawn && IsControlled() && !IsInOcean() && !HeldCargo && (!HasAuthority() || GetController()) &&
        GetCharacterMovement()->IsMovingOnGround() && Assistance!=ECalmyHelperState::Blocked;
}
void ACalmyCrewHelper::Tick(float DT)
{
    Super::Tick(DT);UpdateAssistance(DT);
}
void ACalmyCrewHelper::UpdateAssistance(float DT)
{
    if(!HasAuthority() || DT<=0) return;
    if(!IsValid(Leader) || Leader->IsActorBeingDestroyed())
    { if(Leader || Assistance!=ECalmyHelperState::Waiting) ReleaseLeader();return; }
    const FVector Here=GetActorLocation(),There=Leader->GetActorLocation();
    FollowDistance=FVector::Distance(Here,There);
    if(!IsControlled() || IsInOcean() || !Leader->IsControlled() || Leader->IsInOcean())
    { Assistance=ECalmyHelperState::Waiting;GetCharacterMovement()->StopMovementImmediately();return; }
    // Preserve turns in the leader's actual traversed path; never teleport to catch up.
    if(FVector::Distance(LastLeader,There)>30)
    {
        if(FVector::Distance(LastLeader,There)>500 || Trail.Num()>=128)
        { Assistance=ECalmyHelperState::Blocked;return; }
        Trail.Add(There);LastLeader=There;
    }
    while(Trail.Num()>0 && FVector::Dist2D(Here,Trail[0])<18 && FMath::Abs(Here.Z-Trail[0].Z)<48) Trail.RemoveAt(0);
    FVector Aim=Trail.Num()>0?Trail[0]:There;
    FVector Delta=Aim-Here;Delta.Z=0;
    FCollisionQueryParams SightQ(SCENE_QUERY_STAT(CrewHelperSight),false,this);SightQ.AddIgnoredActor(Leader);
    if(Leader->HeldCargo) SightQ.AddIgnoredActor(Leader->HeldCargo);
    FHitResult Sight;
    const bool Visible=!GetWorld()->LineTraceSingleByChannel(Sight,Here,There,ECC_Visibility,SightQ);
    if(FollowDistance<145 && Trail.Num()<6 && Visible)
    {
        Assistance=Leader->HeldCargo?ECalmyHelperState::Assisting:ECalmyHelperState::Following;
        StuckTime=0;Previous=Here;return;
    }
    FVector Direction=Delta.GetSafeNormal();
    FCollisionQueryParams Q(SCENE_QUERY_STAT(CrewHelperRoute),false,this);Q.AddIgnoredActor(Leader);
    if(Leader->HeldCargo) Q.AddIgnoredActor(Leader->HeldCargo);
    // Do not blindly follow a breadcrumb over a missing deck edge.
    FHitResult Floor;
    const FVector Ahead=Here+Direction*30;
    FFindFloorResult FloorProbe;GetCharacterMovement()->FindFloor(Ahead,FloorProbe,false);
    if(!FloorProbe.IsWalkableFloor() || FloorProbe.FloorDist>GetCharacterMovement()->MaxStepHeight+10)
    { Assistance=ECalmyHelperState::Blocked;GetCharacterMovement()->StopMovementImmediately();return; }
    StuckTime=FVector::DistSquared(Here,Previous)<.2?StuckTime+DT:0;Previous=Here;
    Assistance=StuckTime>1.5?ECalmyHelperState::Blocked:ECalmyHelperState::Following;
    // A small swept side step can free a shoulder caught on a door frame.
    if(StuckTime>.35)
    {
        const FVector Side=FVector::CrossProduct(Direction,FVector::UpVector);
        for(float Sign:{1.f,-1.f})
        {
            FHitResult Hit;const FVector Candidate=Here+Side*Sign*65+Direction*30;
            if(!GetWorld()->SweepSingleByChannel(Hit,Here,Candidate,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,86),Q) &&
                GetWorld()->LineTraceSingleByChannel(Floor,Candidate,Candidate-FVector(0,0,130),ECC_WorldStatic,Q))
            {Direction=(Candidate-Here).GetSafeNormal2D();break;}
        }
    }
    AddMovementInput(Direction,FMath::Min(FMath::Clamp((FollowDistance-105.f)/180.f,.25f,1.f),static_cast<float>(Delta.Size()/70.)),true);
}
void ACalmyCrewHelper::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseLeader();
    if(HasAuthority() && GetController()) GetController()->Destroy();
    Super::EndPlay(Reason);
}
void ACalmyCrewHelper::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACalmyCrewHelper,Leader);DOREPLIFETIME(ACalmyCrewHelper,Assistance);DOREPLIFETIME(ACalmyCrewHelper,FollowDistance);
}
