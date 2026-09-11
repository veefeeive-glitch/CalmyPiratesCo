#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Core/CargoGrip.h"
#include "Core/TeamCarryPace.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyOcean.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

ACalmyCargo::ACalmyCargo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30);
	SetMinNetUpdateFrequency(15);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoMesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetRelativeScale3D(FVector(.7f));
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->SetLinearDamping(.4f);
	Mesh->SetAngularDamping(1.f);
    CargoVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoVisual"));
    CargoVisual->SetupAttachment(Mesh);CargoVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CargoVisual->SetCanEverAffectNavigation(false);
}

void ACalmyCargo::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	OnRep_Kind();
	Mesh->OnComponentHit.AddDynamic(this, &ThisClass::Hit);
	OnRep_Carry();
	OnRep_Condition();
}

void ACalmyCargo::PublishState()
{
	Carry.State = Policy.GetState();
	OnRep_Carry();
	ForceNetUpdate();
}

void ACalmyCargo::OnRep_Carry()
{
	const bool Held = Carry.State == ECalmyCarryState::Held || Carry.State == ECalmyCarryState::Reserved;
	if (Held || Carry.bDelivered)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
	}
	UpdateTint();
}

void ACalmyCargo::OnRep_Kind()
{
    const TCHAR* Name=Kind==ECalmyCargoKind::Bulky?TEXT("Bulky"):Kind==ECalmyCargoKind::Unstable?TEXT("Unstable"):Kind==ECalmyCargoKind::Fragile?TEXT("Fragile"):TEXT("Standard");
    if(auto* Art=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Cargo/R3/SM_Cargo_%s.SM_Cargo_%s"),Name,Name)))
    {
        CargoVisual->SetStaticMesh(Art);
        CargoVisual->SetRelativeLocation(FVector(0,0,-50-Art->GetBoundingBox().Min.Z));
        Mesh->SetVisibility(false);Mesh->SetCastShadow(false);
    }
	if (Kind == ECalmyCargoKind::Bulky) Mesh->SetRelativeScale3D(FVector(1.15f, .9f, .75f));
	else if (Kind != ECalmyCargoKind::Standard) Mesh->SetRelativeScale3D(FVector(.65f));
	Mesh->SetMassOverrideInKg(NAME_None, Kind == ECalmyCargoKind::Bulky ? 32.f : 12.f);
	UpdateTint();
}

void ACalmyCargo::UpdateTint()
{
	if (!Tint) Tint = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	if (!Tint) return;
	FLinearColor Color(.64f, .33f, .10f);
	if (Kind == ECalmyCargoKind::Bulky) Color = FLinearColor(.9f, .65f, .03f);
	if (Kind == ECalmyCargoKind::Unstable) Color = FMath::Lerp(FLinearColor(1, .015f, .01f), FLinearColor(.03f, .3f, .95f), Condition.Stability / 100.f);
	if (Kind == ECalmyCargoKind::Fragile) Color = FMath::Lerp(FLinearColor(.4f, .01f, .015f), FLinearColor(.2f, .85f, .75f), Condition.Integrity / 100.f);
	if (Carry.bDelivered) Color = Condition.Integrity < 100 ? FLinearColor(.9f, .22f, .015f) : FLinearColor(.08f, .65f, .22f);
	Tint->SetVectorParameterValue(TEXT("Tint"), Color);
}

void ACalmyCargo::PlayDamageCue()
{
	if (GetNetMode() == NM_DedicatedServer || !GetWorld()->AllowAudioPlayback()) return;
	// Locally synthesized short impact tone; no copyrighted or editor-only sound asset dependency.
	auto* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(22050);
	Wave->NumChannels = 1;
	Wave->Duration = .18f;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(3969);
	for (int32 I = 0; I < Samples.Num(); ++I)
	{
		const float T = I / 22050.f;
		Samples[I] = static_cast<int16>(7500.f * FMath::Exp(-25.f * T) * FMath::Sin(2.f * PI * (700.f * T - 1000.f * T * T)));
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	UGameplayStatics::PlaySoundAtLocation(this, Wave, GetActorLocation(), .4f);
}

void ACalmyCargo::OnRep_Condition()
{
	// First late-join snapshot presents current damage without replaying historical impact sounds.
	if (bSeenCondition && Condition.DamageEvents > LastSeenDamageEvents) PlayDamageCue();
	LastSeenDamageEvents = Condition.DamageEvents;
	bSeenCondition = true;
	UpdateTint();
}

FVector ACalmyCargo::GetGripPoint(bool bRightHand, bool bFarSide) const
{
	// Per-kind handle-biased locals in actor centimetres; actor scale applies once.
	return GetActorTransform().TransformPosition(
		FCalmyCargoGrip::HandleLocal(static_cast<uint8>(Kind), bRightHand, bFarSide));
}

bool ACalmyCargo::HasNearbyHelper(const ACalmyCharacter* Holder) const
{
	return FindNearbyHelper(Holder)!=nullptr;
}

ACalmyCharacter* ACalmyCargo::FindNearbyHelper(const ACalmyCharacter* Holder) const
{
	if (!IsValid(Holder)) return nullptr;
	ACalmyCharacter* Best=nullptr;
	float BestDistance=FMath::Square(300.f)+1;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
	{
        if(const auto* NPC=Cast<ACalmyCrewHelper>(*It);NPC && !NPC->CanAssist(Holder)) continue;
		if (*It == Holder || (!It->GetController() && !Cast<ACalmyCrewHelper>(*It)) || !It->IsControlled() || It->IsInOcean() || It->HeldCargo ||
			FVector::DistSquared(It->GetActorLocation(), Holder->GetActorLocation()) > FMath::Square(300.f)) continue;
		FHitResult HitResult;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(CalmyHelper), false, Holder);
		Query.AddIgnoredActor(*It);
		Query.AddIgnoredActor(this);
		const float Distance=FVector::DistSquared(It->GetActorLocation(),Holder->GetActorLocation());
		if (Distance<BestDistance && !GetWorld()->LineTraceSingleByChannel(HitResult, Holder->GetActorLocation(), It->GetActorLocation(), ECC_Visibility, Query))
		{ Best=*It;BestDistance=Distance; }
	}
	return Best;
}

bool ACalmyCargo::CanLift(const ACalmyCharacter* Requester) const
{
	return !Carry.bDelivered && (Kind != ECalmyCargoKind::Bulky || HasNearbyHelper(Requester)) &&
		(Kind != ECalmyCargoKind::Unstable || Condition.Stability >= 25);
}

FString ACalmyCargo::GetHandlingHint(const ACalmyCharacter* Viewer) const
{
	switch (Kind)
	{
	case ECalmyCargoKind::Bulky: return HasNearbyHelper(Viewer) ? FString::Printf(TEXT("BULKY | team pace %d%% | stay together; cannot throw"),Condition.TeamPace) : TEXT("BULKY | need free teammate within 3m | H request NPC crew mate");
	case ECalmyCargoKind::Unstable: return FString::Printf(TEXT("UNSTABLE | stability %d%% | Q handoff; rest on deck to recharge"), Condition.Stability);
	case ECalmyCargoKind::Fragile: return FString::Printf(TEXT("FRAGILE | integrity %d%% | avoid hard impacts"), Condition.Integrity);
	default: return TEXT("STANDARD | E grab, G drop, LMB throw");
	}
}

bool ACalmyCargo::TryGrab(ACalmyCharacter* Requester)
{
	if (!HasAuthority() || !IsValid(Requester) || !CanLift(Requester) || !Requester->CanGrab(this) || !Policy.TryGrab(Requester->GetUniqueID())) return false;
	MissingHelperTime = 0;
	Carry.Holder = Requester;
	Requester->HeldCargo = this;
	PublishState();
	SetActorLocation(Requester->GetCarryLocation());
	return true;
}

bool ACalmyCargo::Drop(ACalmyCharacter* Requester)
{
	if (!HasAuthority() || !IsValid(Requester) || Carry.Holder != Requester || !Policy.TryRelease(Requester->GetUniqueID())) return false;
	if (Requester->HeldCargo == this) Requester->HeldCargo = nullptr;
	Carry.Holder = nullptr;
	PublishState();
	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	return true;
}

bool ACalmyCargo::Throw(ACalmyCharacter* Requester)
{
	if (!HasAuthority() || Kind == ECalmyCargoKind::Bulky || !IsValid(Requester) || !Requester->IsControlled() || Carry.Holder != Requester ||
		!Policy.TryMarkThrown(Requester->GetUniqueID())) return false;
	SetActorLocation(Requester->GetCarryLocation());
	Requester->HeldCargo = nullptr;
	ThrowAge = 0;
	RestTime = 0;
	PublishState();
    FRotator Aim=Requester->GetControlRotation();
    Aim.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch),-70.f,70.f);
    if(Aim.ContainsNaN()) Aim=Requester->GetActorRotation();
    PreviousVelocity = Aim.Vector() * 1400.f + FVector(0, 0, 250.f);
	Mesh->SetPhysicsLinearVelocity(PreviousVelocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector(0, 160, 80));
	return true;
}

void ACalmyCargo::MakeFree()
{
	if (Policy.GetState() == ECalmyCarryState::Thrown) Policy.ReleaseAfterThrow();
	else if (Policy.GetState() == ECalmyCarryState::Held) Policy.TryRelease(Policy.GetHolderClientId());
	if (IsValid(Carry.Holder) && Carry.Holder->HeldCargo == this) Carry.Holder->HeldCargo = nullptr;
	Carry.Holder = nullptr;
	PublishState();
}

void ACalmyCargo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Carry.State == ECalmyCarryState::Held)
	{
		if (IsValid(Carry.Holder))
		{
			// Clients only present the replicated holder's transform; outcomes remain server-owned.
			SetActorLocation(Carry.Holder->GetCarryLocation());
			SetActorRotation(Carry.Holder->GetActorRotation());
		}
		else if (HasAuthority()) MakeFree();
	}
    if (!HasAuthority())
    {
        if(!Carry.bDelivered && Carry.State!=ECalmyCarryState::Held)
            if(const auto* Ocean=ACalmyOcean::Find(GetWorld())) Ocean->ApplyBuoyancy(Mesh,Kind==ECalmyCargoKind::Bulky ? 1.15f : 1.8f);
        return;
    }
	UpdateCondition(DeltaSeconds);
	PreviousVelocity = Mesh->GetPhysicsLinearVelocity();
	if (Carry.State == ECalmyCarryState::Thrown)
	{
		ThrowAge += DeltaSeconds;
		PreviousVelocity = Mesh->GetPhysicsLinearVelocity();
		RestTime = PreviousVelocity.Size() < 80.f ? RestTime + DeltaSeconds : 0.f;
		if ((ThrowAge > .5f && RestTime > .35f) || ThrowAge > 5.f) MakeFree();
	}
    if(!Carry.bDelivered && Carry.State != ECalmyCarryState::Held)
    {
        if(const auto* Ocean=ACalmyOcean::Find(GetWorld()))
        {
            Ocean->ApplyBuoyancy(Mesh,Kind==ECalmyCargoKind::Bulky ? 1.15f : 1.8f);
            if(Ocean->IsOutsideSafetyArea(GetActorLocation())) ResetHome();
        }
        else if(FCalmyShipBoundsPolicy::IsCargoOverboard(GetActorLocation())) ResetHome();
    }
}

bool ACalmyCargo::TryHandoff(ACalmyCharacter* Requester, ACalmyCharacter* Receiver)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || Kind != ECalmyCargoKind::Unstable || Carry.State != ECalmyCarryState::Held || Carry.Holder != Requester ||
		!IsValid(Requester) || !Requester->IsControlled() || !IsValid(Receiver) || Cast<ACalmyCrewHelper>(Receiver) || Requester == Receiver || !Receiver->GetController() ||
		!Receiver->CanGrab(this) || FVector::DistSquared(Requester->GetActorLocation(), Receiver->GetActorLocation()) > FMath::Square(250.f) || Now < NextHandoffTime) return false;
	if (!Policy.TryRelease(Requester->GetUniqueID()) || !Policy.TryGrab(Receiver->GetUniqueID())) return false;
	Requester->HeldCargo = nullptr;
	Receiver->HeldCargo = this;
	Carry.Holder = Receiver;
	Stability = FMath::Min(100.f, Stability + 35.f);
	Condition.Stability = FMath::RoundToInt(Stability);
	++Condition.Handoffs;
	NextHandoffTime = Now + 2.f;
	OnRep_Condition();
	PublishState();
	Requester->ForceNetUpdate(); Receiver->ForceNetUpdate();
	return true;
}

void ACalmyCargo::UpdateCondition(float DeltaSeconds)
{
	if (!HasAuthority() || Carry.bDelivered || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
	if (Kind == ECalmyCargoKind::Bulky && Carry.State == ECalmyCarryState::Held)
	{
		const auto* Helper=FindNearbyHelper(Carry.Holder);
		MissingHelperTime = Helper ? 0.f : MissingHelperTime + DeltaSeconds;
		const float Pace=Helper ? FCalmyTeamCarryPace::Evaluate(FVector::Distance(Helper->GetActorLocation(),Carry.Holder->GetActorLocation()),
			(Helper->GetVelocity()-Carry.Holder->GetVelocity()).Size2D()) : .55f;
		const int32 Quantized=FMath::RoundToInt(Pace*20)*5;
		if(Condition.TeamPace!=Quantized) { Condition.TeamPace=Quantized;ForceNetUpdate(); }
		if (MissingHelperTime >= 1.f && IsValid(Carry.Holder))
		{
			++Condition.ForcedDrops;
			Drop(Carry.Holder);
			OnRep_Condition(); ForceNetUpdate();
		}
	}
	if (Kind == ECalmyCargoKind::Unstable)
	{
		if (Carry.State == ECalmyCarryState::Held) Stability = FMath::Max(0.f, Stability - 20.f * DeltaSeconds);
		else if (Carry.State == ECalmyCarryState::Free && Mesh->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(80.f))
			Stability = FMath::Min(100.f, Stability + 25.f * DeltaSeconds);
		const int32 NewStability = FMath::RoundToInt(Stability);
		if (Condition.Stability != NewStability) { Condition.Stability = NewStability; OnRep_Condition(); ForceNetUpdate(); }
		if (Stability <= 0 && Carry.State == ECalmyCarryState::Held && IsValid(Carry.Holder))
		{
			++Condition.ForcedDrops;
			Drop(Carry.Holder);
			OnRep_Condition(); ForceNetUpdate();
		}
	}
}

bool ACalmyCargo::ApplyImpactSpeed(float Speed)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || Kind != ECalmyCargoKind::Fragile || Carry.bDelivered || Carry.State == ECalmyCarryState::Held ||
		!FMath::IsFinite(Speed) || Speed < 650.f || Condition.Integrity <= 0 || Now < NextDamageTime) return false;
	Condition.Integrity = FMath::Max(0, Condition.Integrity - FMath::Clamp(FMath::RoundToInt((Speed - 400.f) / 10.f), 10, 60));
	++Condition.DamageEvents;
	NextDamageTime = Now + .35f;
	OnRep_Condition(); ForceNetUpdate();
	return true;
}

void ACalmyCargo::ResetHome(bool bIncident)
{
	if (!HasAuthority()) return;
	Carry.bDelivered = false;
	if (!bIncident)
	{
		Condition = FCalmyCargoCondition(); Stability = 100; NextDamageTime = NextHandoffTime = -1;
		OnRep_Condition();
	}
	MissingHelperTime = 0;
	MakeFree();
	ThrowAge = RestTime = 0;
	PreviousVelocity = FVector::ZeroVector;
	SetActorLocationAndRotation(HomeLocation, HomeRotation, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	if (bIncident)
	{
		++ResetCount;
		if (auto* Shift = ACalmyShift::Find(GetWorld())) Shift->RecordRescue();
	}
	ForceNetUpdate();
}

void ACalmyCargo::MarkDelivered()
{
	if (!HasAuthority() || Carry.bDelivered) return;
	MakeFree();
	Carry.bDelivered = true;
	PublishState();
}

void ACalmyCargo::Hit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector Impulse, const FHitResult& HitResult)
{
	if (!HasAuthority()) return;
	const float ClosingSpeed = FMath::Max(FMath::Abs(FVector::DotProduct(PreviousVelocity, HitResult.ImpactNormal)),
		Impulse.Size() / FMath::Max(1.f, Mesh->GetMass()));
	ApplyImpactSpeed(ClosingSpeed);
	if (Carry.State != ECalmyCarryState::Thrown) return;
	auto* Player = Cast<ACalmyCharacter>(OtherActor);
	if (Player && !(Player == Carry.Holder && ThrowAge < .25f)) Player->ReceiveCargoImpact(PreviousVelocity.Size(),PreviousVelocity.GetSafeNormal());
}

void ACalmyCargo::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority() && IsValid(Carry.Holder) && Carry.Holder->HeldCargo == this) Carry.Holder->HeldCargo = nullptr;
	Super::EndPlay(Reason);
}

void ACalmyCargo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyCargo, Carry);
	DOREPLIFETIME(ACalmyCargo, CargoId);
	DOREPLIFETIME(ACalmyCargo, ResetCount);
	DOREPLIFETIME(ACalmyCargo, Kind);
	DOREPLIFETIME(ACalmyCargo, Condition);
}
