#include "Testing/CalmyInteriorProbe.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyInteriorCutaway.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyInteriorProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyInteriorProbe="), Value);
#endif
}

void UCalmyInteriorProbe::Finish(bool bSuccess, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_INTERIOR %s %s %s"), *Role, bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Detail);
	GLog->Flush(); Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void UCalmyInteriorProbe::Tick(float DeltaSeconds)
{
	if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyInteriorProbe="), Role);
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if ((Role == TEXT("Host") && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Role != TEXT("Host") && GetWorld()->GetNetMode() != NM_Client)) return;
	Age += DeltaSeconds;
	ACalmyCharacter* Local = nullptr;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It) if (It->IsLocallyControlled()) Local = *It;
	ACalmyInteriorCutaway* Interior = nullptr;
	for (TActorIterator<ACalmyInteriorCutaway> It(GetWorld()); It; ++It) { Interior = *It; break; }
	if (!Local || !Interior || Interior->RoofPanels.IsEmpty()) return;
	if (Age > 120) { Finish(false, FString::Printf(TEXT("timeout stage=%d pos=%s"), Stage, *Local->GetActorLocation().ToString())); return; }
	if (!bStarted)
	{
		bStarted = true;
		UE_LOG(LogTemp, Display, TEXT("CALMY_INTERIOR %s START"), *Role); GLog->Flush();
	}
	auto Advance = [this, Local](int32 Next)
	{
		Stage = Next;
		StageTime = Age;
		UE_LOG(LogTemp, Display, TEXT("CALMY_INTERIOR %s STAGE %d pos=%s"), *Role, Stage, *Local->GetActorLocation().ToString());
		GLog->Flush();
	};
	auto MoveTo = [Local](const FVector2D Target)
	{
		const FVector2D Delta = Target - FVector2D(Local->GetActorLocation());
		if (Delta.Size() < 22.f) return true;
		Local->MoveIntent(Delta.GetSafeNormal()); return false;
	};
	if (Stage == 0 && MoveTo(FVector2D(-650, 0))) Advance(1);
	else if (Stage == 1 && MoveTo(FVector2D(-1450, 0))) Advance(2);
	else if (Stage == 2)
	{
		if (Interior->RoofPanels[0]->IsVisible()) { Finish(false, TEXT("roof_did_not_hide_for_local_crew")); return; }
		UE_LOG(LogTemp, Display, TEXT("CALMY_INTERIOR %s INSIDE"), *Role); GLog->Flush();
		Advance(3);
	}
	// Walk between the helm console and port navigation table before probing
	// the aft bulkhead. This is a deliberate navigable aisle in the bridge.
	else if (Stage == 3 && MoveTo(FVector2D(-1450, 220))) Advance(4);
	else if (Stage == 4)
	{
		Local->MoveIntent(FVector2D(-1, 0));
		if (Age - StageTime > 4.f)
		{
			const float X = Local->GetActorLocation().X;
			const bool bValid = X >= -1720.f && X <= -1640.f && !Interior->RoofPanels[0]->IsVisible();
			if (!bValid) { Finish(false, FString::Printf(TEXT("aft_collision_failed stopped_x=%.1f"), X)); return; }
			Advance(5);
		}
	}
	else if (Stage == 5 && MoveTo(FVector2D(-1450, 220))) Advance(6);
	else if (Stage == 6 && MoveTo(FVector2D(-1450, 0))) Advance(7);
	else if (Stage == 7 && MoveTo(FVector2D(-1070, 0))) Advance(8);
	else if (Stage == 8 && MoveTo(FVector2D(-1070, -350))) Advance(9);
	else if (Stage == 9 && MoveTo(FVector2D(-970, -350))) Advance(10);
	else if (Stage == 10 && MoveTo(FVector2D(-1180, -350))) Advance(11);
	else if (Stage == 11)
	{
		if (Age - StageTime < 1.f) return;
		const float Z = Local->GetActorLocation().Z;
		bool bFoundDeck = false;
		bool bDeckHidden = true;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("CalmyLowerDeckOccluder"))) continue;
			bFoundDeck = true;
			if (const UStaticMeshComponent* Mesh = It->FindComponentByClass<UStaticMeshComponent>()) bDeckHidden &= !Mesh->IsVisible();
		}
		if (Z > -90.f || !bFoundDeck || !bDeckHidden)
		{
			Finish(false, FString::Printf(TEXT("lower_deck_entry_failed pos=%s found_deck=%d hidden=%d"),
				*Local->GetActorLocation().ToString(), bFoundDeck, bDeckHidden)); return;
		}
		UE_LOG(LogTemp, Display, TEXT("CALMY_INTERIOR %s BELOW_DECK"), *Role); GLog->Flush();
		Advance(12);
	}
	// Step sideways off the flight at the lower landing before moving toward
	// the bow; moving directly forward would correctly climb the stairs again.
	else if (Stage == 12 && MoveTo(FVector2D(-1170, -150))) Advance(13);
	else if (Stage == 13 && MoveTo(FVector2D(-700, 0))) Advance(14);
	else if (Stage == 14 && MoveTo(FVector2D(300, 0))) Advance(15);
	else if (Stage == 15)
	{
		const float LaneY = Role == TEXT("Client") ? -150.f : (Role == TEXT("LateJoin") ? 150.f : 0.f);
		if (MoveTo(FVector2D(850, LaneY))) Advance(16);
	}
	else if (Stage == 16 && (Role != TEXT("Host") || Age > 85.f))
	{
		Finish(true, FString::Printf(TEXT("bridge=1 aft_collision=1 stairs=1 lower_cutaway=1 engine_room=1 crew_deck=1 cargo_hold=1 final_pos=%s"),
			*Local->GetActorLocation().ToString()));
	}
}
