#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyPreferences.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"

bool ACalmyTutorial::bForceReplaySession = false;

ACalmyTutorial::ACalmyTutorial()
{
	PrimaryActorTick.bCanEverTick=true;
	bReplicates=true; bAlwaysRelevant=true; SetNetUpdateFrequency(5);
}
void ACalmyTutorial::BeginForcedReplay()
{
	bForceReplaySession = true;
}
bool ACalmyTutorial::IsForcedReplay()
{
	return bForceReplaySession;
}
void ACalmyTutorial::MarkCompletedLocally()
{
	bForceReplaySession = false;
	auto* Pref = UCalmyPreferences::Get();
	if (Pref->bTutorialCompleted) return;
	Pref->bTutorialCompleted = true;
	Pref->Persist();
	UE_LOG(LogTemp, Display, TEXT("CALMY_TUTORIAL completed_saved=1"));
}
bool ACalmyTutorial::Enabled(const UWorld* World)
{
	if(!World || !World->GetMapName().Contains(TEXT("WaterLab"))) return false;
	const FString Command=FCommandLine::Get();
	// Existing isolated physics fixtures keep their laboratory contract semantics.
	// Explicit -CalmyLab also permits free practice without automatic departure.
	if (FParse::Param(*Command,TEXT("CalmyLab"))) return false;
	if (Command.Contains(TEXT("Probe=")) && !Command.Contains(TEXT("CalmyTutorialProbe="))) return false;
	if (bForceReplaySession) return true;
	if (UCalmyPreferences::Get()->bTutorialCompleted) return false;
	return true;
}
bool ACalmyTutorial::ShouldAutoBoard(const UWorld* World)
{
	if (!World || !World->GetMapName().Contains(TEXT("WaterLab"))) return false;
	if (bForceReplaySession) return false;
	const FString Command = FCommandLine::Get();
	if (FParse::Param(*Command, TEXT("CalmyLab"))) return false;
	// Probes and unattended fixtures keep their map; never silent-travel away.
	if (Command.Contains(TEXT("Probe="))) return false;
	return UCalmyPreferences::Get()->bTutorialCompleted;
}
ACalmyTutorial* ACalmyTutorial::Find(const UWorld* World)
{
	if(World) for(TActorIterator<ACalmyTutorial> It(World);It;++It) return *It;
	return nullptr;
}
void ACalmyTutorial::OnRep_Step()
{
	if (Step == ECalmyTutorialStep::Depart) MarkCompletedLocally();
}
void ACalmyTutorial::Advance(ECalmyTutorialStep Next)
{
	Step=Next; ForceNetUpdate();
	if (Next == ECalmyTutorialStep::Depart) MarkCompletedLocally();
	UE_LOG(LogTemp,Display,TEXT("CALMY_TUTORIAL step=%d"),int32(Step));
}
void ACalmyTutorial::Tick(float DT)
{
	Super::Tick(DT);
	if(!HasAuthority() || DT<=0 || !FMath::IsFinite(DT) || bTravelRequested) return;
	if(!IsValid(TrainingCargo))
		for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)
			if(It->Kind==ECalmyCargoKind::Standard && !It->Carry.bDelivered) {TrainingCargo=*It;ForceNetUpdate();break;}
	if(Step==ECalmyTutorialStep::Walk)
	{
		// Only controlled human pawns count; NPC motion, swimming and rescues do not.
		for(auto It=PreviousPositions.CreateIterator();It;++It) if(!It.Key().IsValid()) It.RemoveCurrent();
		for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)
		{
			if(!Cast<APlayerController>(It->GetController())) continue;
			const FVector P=It->GetActorLocation();
			const auto* Previous=PreviousPositions.Find(*It);
			if(Previous && It->IsControlled() && It->GetCharacterMovement()->IsMovingOnGround())
			{
				const float Distance=FVector::Dist2D(P,*Previous);
				if(Distance<100 && Distance<DT*900) WalkDistance+=Distance;
			}
			PreviousPositions.Add(*It,P);
		}
		WalkedCm=FMath::Min(400,FMath::FloorToInt(WalkDistance));
		if(WalkedCm>=400) Advance(ECalmyTutorialStep::Grab);
	}
	if(Step==ECalmyTutorialStep::Grab && IsValid(TrainingCargo))
	{
		const auto* Holder=TrainingCargo->Carry.Holder.Get();
		if(Holder && Cast<APlayerController>(Holder->GetController())) Advance(ECalmyTutorialStep::Deliver);
		// A player may complete delivery before the introductory walking distance.
		else if(TrainingCargo->Carry.bDelivered) Advance(ECalmyTutorialStep::Deliver);
	}
	if(Step==ECalmyTutorialStep::Deliver && IsValid(TrainingCargo) && TrainingCargo->Carry.bDelivered)
		Advance(ECalmyTutorialStep::Depart);
	if(Step==ECalmyTutorialStep::Depart)
	{
		DepartureRemaining=FMath::Max(0.f,DepartureRemaining-DT);
		DepartureSeconds=FMath::CeilToInt(DepartureRemaining);
		if(DepartureRemaining<=0)
		{
			// Fixed allowlisted map; engine ServerTravel moves all connected players.
			// One request only. Late join sees the replicated current step/countdown;
			// disconnect does not undo team progress. No input can supply a URL.
			bTravelRequested=true;
			// Relative engine travel retains listen and the current session port.
			// Absolute travel without those options would silently turn the host solo.
			if(!FPackageName::DoesPackageExist(Destination()) || !GetWorld()->ServerTravel(Destination(),false))
				Advance(ECalmyTutorialStep::TravelFailed);
		}
	}
}
FString ACalmyTutorial::Instruction() const
{
	switch(Step)
	{
	case ECalmyTutorialStep::Walk: return FString::Printf(TEXT("1/3  WALK ON THE PLATFORM  [WASD + mouse]  %.1f / 4 m"),WalkedCm/100.f);
	case ECalmyTutorialStep::Grab: return TEXT("2/3  PICK UP THE WOODEN CRATE  [aim + E]");
	case ECalmyTutorialStep::Deliver: return TEXT("3/3  GREEN ZONE: PUT THE CRATE DOWN  [G]  and let it settle");
	case ECalmyTutorialStep::Depart: return FString::Printf(TEXT("TRAINING COMPLETE | Entire crew boards the ship in %d s"),DepartureSeconds);
	default: return TEXT("SHIP LOAD FAILED | Restart or launch OceanFreighterR7 directly");
	}
}
FVector ACalmyTutorial::ObjectiveLocation() const
{
	if(Step==ECalmyTutorialStep::Deliver) if(auto* Shift=ACalmyShift::Find(GetWorld())) return Shift->DeliveryZone->GetComponentLocation();
	return IsValid(TrainingCargo)?TrainingCargo->GetActorLocation():FVector::ZeroVector;
}
void ACalmyTutorial::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyTutorial,Step); DOREPLIFETIME(ACalmyTutorial,TrainingCargo);
	DOREPLIFETIME(ACalmyTutorial,DepartureSeconds); DOREPLIFETIME(ACalmyTutorial,WalkedCm);
}
