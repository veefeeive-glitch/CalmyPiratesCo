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
    Aggressor=nullptr;Provocations.Reset();Reaction.Reset();
    ReleaseLeader();SetActorLocation(Station,false,nullptr,ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}
void ACalmyCrewHelper::AssignLeader(ACalmyCharacter* Pawn)
{
    if(!HasAuthority() || IsDefending() || !IsValid(Pawn) || Pawn==this || Pawn->GetWorld()!=GetWorld() || (IsValid(Leader) && Leader!=Pawn)) return;
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
    Super::Tick(DT);
    if (HasAuthority() && (Aggressor || Assistance == ECalmyHelperState::Defending)) UpdateDefence(DT);
    else UpdateAssistance(DT);
}
int32 ACalmyCrewHelper::GetProvocation(const ACalmyCharacter* Pawn) const
{
    const int32* Count = Provocations.Find(const_cast<ACalmyCharacter*>(Pawn)); return Count ? *Count : 0;
}
bool ACalmyCrewHelper::ReceiveMeleeHit(ACalmyCharacter* Attacker)
{
    if (!Super::ReceiveMeleeHit(Attacker)) return false;
    for (auto It = Provocations.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    int32& Count = Provocations.FindOrAdd(Attacker); Count = FMath::Min(3, Count + 1);
    Reaction = Count == 1 ? TEXT("Misha: Hey! Watch your hands.") : Count == 2 ? TEXT("Misha: Last warning. Stop hitting me.") : TEXT("Misha: Enough!");
    if (Count >= 3)
    {
        ReleaseLeader(); Aggressor = Attacker; DefenceUntil = GetWorld()->GetTimeSeconds() + 8.f;
        DefenceOrigin = GetActorLocation(); Assistance = ECalmyHelperState::Defending;
    }
    ForceNetUpdate(); return true;
}
void ACalmyCrewHelper::UpdateDefence(float DT)
{
    if (!IsValid(Aggressor) || !Aggressor->GetController() || GetWorld()->GetTimeSeconds() >= DefenceUntil ||
        FVector::DistSquared(GetActorLocation(), DefenceOrigin) > FMath::Square(450.f) ||
        (IsValid(Aggressor) && FVector::DistSquared(GetActorLocation(), Aggressor->GetActorLocation()) > FMath::Square(500.f)))
    {
        Provocations.Remove(Aggressor); Aggressor = nullptr; ReleaseLeader(); Reaction = TEXT("Misha: Let's get back to work.");
        GetCharacterMovement()->bOrientRotationToMovement = true; ForceNetUpdate(); return;
    }
    Assistance = ECalmyHelperState::Defending;
    if (!IsControlled() || IsInOcean() || !Aggressor->IsControlled()) { GetCharacterMovement()->StopMovementImmediately(); return; }
    const FVector Delta = Aggressor->GetActorLocation() - GetActorLocation();
    FCollisionQueryParams Q(SCENE_QUERY_STAT(MishaDefence), true, this); Q.AddIgnoredActor(Aggressor);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetInteractionOrigin(), Aggressor->GetInteractionOrigin(), ECC_Visibility, Q))
    { GetCharacterMovement()->StopMovementImmediately(); return; }
    GetCharacterMovement()->bOrientRotationToMovement = false;
    const float TargetYaw = Delta.Rotation().Yaw;
    SetActorRotation(FRotator(0,FMath::FixedTurn(GetActorRotation().Yaw,TargetYaw,180.f*DT),0));
    const FRotator Aim = (Aggressor->GetActorLocation() + FVector(0,0,30) - GetInteractionOrigin()).Rotation();
    if (GetController()) GetController()->SetControlRotation(FRotator(Aim.Pitch,GetActorRotation().Yaw,0));
    if (FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,TargetYaw))>12.f)
    { GetCharacterMovement()->StopMovementImmediately(); return; }
    if (FindMeleeTarget() == Aggressor) { GetCharacterMovement()->StopMovementImmediately(); PerformPunch(); return; }
    FFindFloorResult Floor; GetCharacterMovement()->FindFloor(GetActorLocation() + Delta.GetSafeNormal2D() * 40.f, Floor, false);
    if (Floor.IsWalkableFloor() && Floor.FloorDist <= GetCharacterMovement()->MaxStepHeight + 10.f) AddMovementInput(Delta.GetSafeNormal2D(), .55f, true);
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
    const bool Carrying=IsValid(Leader->HeldCargo) && Visible && FollowDistance<300;
    GetCharacterMovement()->bOrientRotationToMovement=!Carrying;
    if(Carrying)
    {
        // Walk to the opposite face; never claim contact from a following slot
        // behind the holder. Normal swept CharacterMovement still owns the route.
        const auto* Cargo=Leader->HeldCargo.Get();
        const FVector Center=(Cargo->GetGripPoint(false,true)+Cargo->GetGripPoint(true,true))*.5;
        Aim=Center+Cargo->GetActorForwardVector()*34.f;
        Aim.Z=Here.Z;Delta=Aim-Here;
        SetActorRotation((-Cargo->GetActorForwardVector()).Rotation());
    }
    if((Carrying?Delta.Size2D()<3:FollowDistance<145 && Trail.Num()<6) && Visible)
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
    Assistance=StuckTime>1.5?ECalmyHelperState::Blocked:(Carrying?ECalmyHelperState::Assisting:ECalmyHelperState::Following);
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
    AddMovementInput(Direction,Carrying?FMath::Clamp(static_cast<float>(Delta.Size()/45.),.15f,1.f):
        FMath::Min(FMath::Clamp((FollowDistance-105.f)/180.f,.25f,1.f),static_cast<float>(Delta.Size()/70.)),true);
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
    DOREPLIFETIME(ACalmyCrewHelper,Aggressor); DOREPLIFETIME(ACalmyCrewHelper,Reaction);
}
