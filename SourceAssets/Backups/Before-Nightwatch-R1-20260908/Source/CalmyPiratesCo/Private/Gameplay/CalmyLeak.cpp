#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyShift.h"
#include "Core/GameplayPolicies.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACalmyLeak::ACalmyLeak()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10);
	Valve = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RepairValve"));
	SetRootComponent(Valve);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/M_Cargo.M_Cargo"));
	Valve->SetStaticMesh(Cube.Object);
	Valve->SetRelativeScale3D(FVector(.75f, .75f, .85f));
	Valve->SetMaterial(0, Material.Object);
	Valve->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterPatch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterLevelIndicator"));
	WaterPatch->SetupAttachment(Valve);
	WaterPatch->SetUsingAbsoluteScale(true);
	WaterPatch->SetStaticMesh(Cube.Object);
	WaterPatch->SetMaterial(0, Material.Object);
	WaterPatch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterPatch->SetCastShadow(false);
	WaterPatch->SetVisibility(false);
	EmergencyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmergencyValveLight"));
	EmergencyLight->SetupAttachment(Valve);
	EmergencyLight->SetRelativeLocation(FVector(0, 0, 260));
	EmergencyLight->SetLightColor(FLinearColor(1.f, .08f, .025f));
	EmergencyLight->SetIntensity(6500.f);
	EmergencyLight->SetAttenuationRadius(750.f);
	EmergencyLight->SetSourceRadius(45.f);
	EmergencyLight->SetCastShadows(false);
	EmergencyLight->SetVisibility(false);
	TaskLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmergencyTaskLight"));
	TaskLight->SetupAttachment(Valve);
	TaskLight->SetLightColor(FLinearColor(.15f, .7f, 1.f));
	TaskLight->SetIntensity(5200.f);
	TaskLight->SetAttenuationRadius(700.f);
	TaskLight->SetSourceRadius(60.f);
	TaskLight->SetCastShadows(false);
	TaskLight->SetVisibility(false);
	EmergencyGrade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("EmergencyPowerGrade"));
	EmergencyGrade->SetupAttachment(Valve);
	EmergencyGrade->bUnbound = true;
	EmergencyGrade->Priority = 10.f;
	EmergencyGrade->BlendWeight = 0.f;
	EmergencyGrade->Settings.bOverride_ColorGain = true;
	EmergencyGrade->Settings.ColorGain = FVector4(.48f, .54f, .66f, 1.f);
	EmergencyGrade->Settings.bOverride_ColorSaturation = true;
	EmergencyGrade->Settings.ColorSaturation = FVector4(.72f, .7f, .66f, 1.f);
	EmergencyGrade->Settings.bOverride_VignetteIntensity = true;
	EmergencyGrade->Settings.VignetteIntensity = .42f;
}

ACalmyLeak* ACalmyLeak::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyLeak> It(World); It; ++It) return *It;
	return nullptr;
}

void ACalmyLeak::BeginPlay()
{
	Super::BeginPlay();
	ValveMaterial = Valve->CreateDynamicMaterialInstance(0);
	WaterMaterial = WaterPatch->CreateDynamicMaterialInstance(0);
	if (WaterMaterial) WaterMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.015f, .22f, .4f));
	if (HasAuthority())
	{
		if (!FParse::Value(FCommandLine::Get(), TEXT("CalmyEmergencySeed="), BaseDirectorSeed))
			BaseDirectorSeed = static_cast<int32>(FPlatformTime::Cycles64() & 0x7fffffffULL);
		BaseDirectorSeed = FCalmyEmergencyDirectorPolicy::NormalizeSeed(BaseDirectorSeed);
		const auto* Shift = ACalmyShift::Find(GetWorld());
		ResetForRound(Shift ? Shift->State.Round : 1);
	}
	if (const auto* Shift = ACalmyShift::Find(GetWorld())) TaskLight->SetWorldLocation(Shift->GetActorLocation() + FVector(0, 0, 240));
	OnRep_State();
}

void ACalmyLeak::ResetForRound(int32 Round)
{
	if (!HasAuthority()) return;
	State = FCalmyLeakSnapshot();
	State.Round = Round;
	State.DirectorSeed = FCalmyEmergencyDirectorPolicy::NormalizeSeed(BaseDirectorSeed);
	ScheduleRemaining = static_cast<float>(FCalmyEmergencyDirectorPolicy::InitialDelaySeconds(State.DirectorSeed, Round));
	State.NextEventSeconds = FMath::CeilToInt(ScheduleRemaining);
	PhaseAge = WaterAmount = RepairAmount = 0;
	CooldownRemaining = 0;
	RepairLeaseEnd = 0;
	OnRep_State(); ForceNetUpdate();
}

bool ACalmyLeak::CanRepair(const ACalmyCharacter* Pawn) const
{
	const auto* Shift = ACalmyShift::Find(GetWorld());
	if (!IsValid(Pawn) || Pawn->GetWorld() != GetWorld() || !Pawn->GetController() || !Pawn->IsControlled() || Pawn->HeldCargo ||
		!Shift || Shift->State.Phase != ECalmyShiftPhase::Loading || Shift->State.Round != State.Round ||
		State.Phase != ECalmyLeakPhase::Leaking || (IsValid(State.Repairer) && State.Repairer != Pawn) ||
		Pawn->GetVelocity().SizeSquared() > FMath::Square(25.f) || Pawn->GetCharacterMovement()->IsFalling() ||
		FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) > FMath::Square(180.f)) return false;
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CalmyRepair), false, Pawn);
	Query.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetActorLocation(), GetActorLocation(), ECC_Visibility, Query);
}

bool ACalmyLeak::TryRepairPulse(ACalmyCharacter* Pawn, int32 ExpectedRound)
{
	if (!HasAuthority() || ExpectedRound != State.Round || !CanRepair(Pawn)) return false;
	State.Repairer = Pawn;
	RepairLeaseEnd = GetWorld()->GetTimeSeconds() + .65;
	ForceNetUpdate();
	return true;
}

void ACalmyLeak::StopRepair(ACalmyCharacter* Pawn)
{
	if (!HasAuthority() || !Pawn || State.Repairer != Pawn) return;
	State.Repairer = nullptr;
	RepairLeaseEnd = 0;
	ForceNetUpdate();
}

void ACalmyLeak::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
	const auto* Shift = ACalmyShift::Find(GetWorld());
	if (Shift && Shift->State.Round != State.Round) ResetForRound(Shift->State.Round);
	if (!Shift || Shift->State.Phase != ECalmyShiftPhase::Loading)
	{
		if (State.Phase != ECalmyLeakPhase::Suspended)
		{
			State.Phase = ECalmyLeakPhase::Suspended; State.Repairer = nullptr;
			State.Water = 0; State.bPowerDegraded = false; State.NextEventSeconds = 0; State.CooldownSeconds = 0;
			WaterAmount = ScheduleRemaining = CooldownRemaining = 0; RepairLeaseEnd = 0;
			OnRep_State(); ForceNetUpdate();
		}
		return;
	}
	const auto Before = State;
	PhaseAge += DeltaSeconds;
	if (State.Phase == ECalmyLeakPhase::Idle)
	{
		ScheduleRemaining = FMath::Max(0.f, ScheduleRemaining - DeltaSeconds);
		State.NextEventSeconds = FMath::CeilToInt(ScheduleRemaining);
		if (FCalmyEmergencyDirectorPolicy::CanStartEvent(State.EventSerial, false, ScheduleRemaining))
		{
			++State.EventSerial;
			WaterAmount = RepairAmount = 0;
			State.Water = State.Repair = 0;
			State.Repairer = nullptr;
			RepairLeaseEnd = 0;
			State.Phase = ECalmyLeakPhase::Warning;
			State.WarningSeconds = 5;
			State.NextEventSeconds = 0;
			PhaseAge = 0;
		}
	}
	if (State.Phase == ECalmyLeakPhase::Warning)
	{
		State.WarningSeconds = FMath::Max(0, FMath::CeilToInt(5.f - PhaseAge));
		if (PhaseAge >= 5) { State.Phase = ECalmyLeakPhase::Leaking; State.WarningSeconds = 0; PhaseAge = 0; }
	}
	else if (State.Phase == ECalmyLeakPhase::Leaking)
	{
		WaterAmount = FMath::Min(100.f, WaterAmount + DeltaSeconds * 2.f);
		const double Now = GetWorld()->GetTimeSeconds();
		if (!CanRepair(State.Repairer) || Now > RepairLeaseEnd) State.Repairer = nullptr;
		if (State.Repairer)
		{
			RepairAmount = FMath::Min(100.f, RepairAmount + DeltaSeconds * 20.f);
			if (RepairAmount >= 100.f)
			{
				State.Phase = ECalmyLeakPhase::Draining; State.Repairer = nullptr; PhaseAge = 0;
			}
		}
	}
	else if (State.Phase == ECalmyLeakPhase::Draining)
	{
		WaterAmount = FMath::Max(0.f, WaterAmount - DeltaSeconds * 10.f);
		if (WaterAmount <= 0)
		{
			State.Phase = ECalmyLeakPhase::Resolved;
			CooldownRemaining = State.EventSerial < FCalmyEmergencyDirectorPolicy::MaxEventsPerRound ?
				static_cast<float>(FCalmyEmergencyDirectorPolicy::ResolveCooldownSeconds(State.DirectorSeed, State.Round, State.EventSerial)) : 0.f;
			State.CooldownSeconds = FMath::CeilToInt(CooldownRemaining);
			PhaseAge = 0;
		}
	}
	else if (State.Phase == ECalmyLeakPhase::Resolved && State.EventSerial < FCalmyEmergencyDirectorPolicy::MaxEventsPerRound)
	{
		CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaSeconds);
		State.CooldownSeconds = FMath::CeilToInt(CooldownRemaining);
		if (FCalmyEmergencyDirectorPolicy::CanStartEvent(State.EventSerial, false, CooldownRemaining))
		{
			++State.EventSerial;
			WaterAmount = RepairAmount = 0;
			State.Water = State.Repair = 0;
			State.Repairer = nullptr;
			RepairLeaseEnd = 0;
			State.Phase = ECalmyLeakPhase::Warning;
			State.WarningSeconds = 5;
			State.CooldownSeconds = 0;
			PhaseAge = 0;
		}
	}
	State.Water = FMath::RoundToInt(WaterAmount);
	State.Repair = FMath::FloorToInt(RepairAmount);
	// A recoverable, readable consequence derived only from authoritative water.
	State.bPowerDegraded = (State.Phase == ECalmyLeakPhase::Leaking || State.Phase == ECalmyLeakPhase::Draining) && State.Water >= 30;
	if (Before.Phase != State.Phase || Before.Water != State.Water || Before.Repair != State.Repair ||
		Before.WarningSeconds != State.WarningSeconds || Before.DirectorSeed != State.DirectorSeed ||
		Before.EventSerial != State.EventSerial || Before.NextEventSeconds != State.NextEventSeconds ||
		Before.CooldownSeconds != State.CooldownSeconds || Before.bPowerDegraded != State.bPowerDegraded || Before.Repairer != State.Repairer)
	{
		OnRep_State(); ForceNetUpdate();
	}
}

void ACalmyLeak::OnRep_State()
{
	const bool Active = State.Phase == ECalmyLeakPhase::Leaking;
	if (ValveMaterial) ValveMaterial->SetVectorParameterValue(TEXT("Tint"), Active ? FLinearColor(1, .04f, .02f) :
		(State.Phase == ECalmyLeakPhase::Warning ? FLinearColor(1, .65f, .02f) : FLinearColor(.04f, .6f, .2f)));
	WaterPatch->SetVisibility(State.Water > 0);
	// Readable rising blue indicator beside the valve; never cover the green delivery zone.
	WaterPatch->SetWorldLocation(GetActorLocation() + FVector(0, -130, -GetActorLocation().Z + 2));
	WaterPatch->SetWorldScale3D(FVector(1.8f + State.Water * .02f, 1.8f + State.Water * .02f, .01f));
	EmergencyLight->SetVisibility(State.bPowerDegraded);
	TaskLight->SetVisibility(State.bPowerDegraded);
	EmergencyGrade->BlendWeight = State.bPowerDegraded ? 1.f : 0.f;
}

FString ACalmyLeak::GetStatusText() const
{
	switch (State.Phase)
	{
	case ECalmyLeakPhase::Idle: return FString::Printf(TEXT("Valve ready | Next drill ~%ds | Event %d/%d"),
		State.NextEventSeconds, State.EventSerial + 1, FCalmyEmergencyDirectorPolicy::MaxEventsPerRound);
	case ECalmyLeakPhase::Warning: return FString::Printf(TEXT("LEAK WARNING: %ds | Event %d/%d | Find the yellow valve"),
		State.WarningSeconds, State.EventSerial, FCalmyEmergencyDirectorPolicy::MaxEventsPerRound);
	case ECalmyLeakPhase::Leaking: return FString::Printf(TEXT("LEAK | Water %d%% | Repair %d%% | %s"), State.Water, State.Repair,
		State.Repairer ? TEXT("REPAIRING") : TEXT("Hold F at red valve, empty hands, stand still"));
	case ECalmyLeakPhase::Draining: return FString::Printf(TEXT("SEALED | Pump draining: water %d%%"), State.Water);
	case ECalmyLeakPhase::Resolved: return State.EventSerial < FCalmyEmergencyDirectorPolicy::MaxEventsPerRound ?
		FString::Printf(TEXT("LEAK REPAIRED | Deck dry | Director cooldown %ds"), State.CooldownSeconds) :
		TEXT("LEAK REPAIRED | Deck dry | Event cap reached");
	default: return TEXT("Leak drill stopped with the contract.");
	}
}

void ACalmyLeak::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyLeak, State);
}
