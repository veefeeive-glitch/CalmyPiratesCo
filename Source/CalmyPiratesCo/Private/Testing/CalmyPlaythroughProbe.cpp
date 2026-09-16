#include "Testing/CalmyPlaythroughProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyFuelStation.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace CalmyPlaythrough
{
	constexpr float ReachCm = 40.f;
	constexpr float StallSeconds = 8.f;
	FString Vec(const FVector& V) { return FString::Printf(TEXT("[%.0f,%.0f,%.0f]"), V.X, V.Y, V.Z); }
}

bool UCalmyPlaythroughProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value; const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyPlaythroughProbe="), Value);
#endif
}

FVector UCalmyPlaythroughProbe::ToWorld(const FVector& Local) const { return ACalmyVoyageDirector::ShipLocalToWorld(GetWorld(), Local); }

FVector UCalmyPlaythroughProbe::ToLocal(const FVector& World) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetRestTransform().TransformPosition(Ship->GetActorTransform().InverseTransformPosition(World)) : World;
}

void UCalmyPlaythroughProbe::Finish(bool bOK, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYTHROUGH %s %s timeline=%s"), bOK ? TEXT("PASS") : TEXT("FAIL"), *Detail, *FString::Join(Timeline, TEXT(" | ")));
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bOK ? 0 : 1);
}

void UCalmyPlaythroughProbe::Photo(const FString& Name)
{
	if (FApp::CanEverRender())
	{
		const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Verification") / FPaths::GetCleanFilename(Dir));
		IFileManager::Get().MakeDirectory(*Folder, true);
		FScreenshotRequest::RequestScreenshot(Folder / FString::Printf(TEXT("%02d-%s.png"), ++Shots, *Name), true, false);
	}
}

void UCalmyPlaythroughProbe::Step(const FString& Name)
{
	Timeline.Add(FString::Printf(TEXT("%s@%.0fs"), *Name, FPlatformTime::Seconds() - Start));
	UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYTHROUGH STEP %s t=%.1f"), *Name, FPlatformTime::Seconds() - Start);
	GLog->Flush();
	StageAge = 0; SinceProgress = 0; BestDistance = BIG_NUMBER; Waypoint = 1; PulseAge = 0;
}

bool UCalmyPlaythroughProbe::LoadLegs()
{
	FString Path, Text;
	FParse::Value(FCommandLine::Get(), TEXT("CalmyPlaythroughProbe="), Path);
	if (!FFileHelper::LoadFileToString(Text, *Path)) return false;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Root->TryGetArrayField(TEXT("legs"), List)) return false;
	for (const TSharedPtr<FJsonValue>& V : *List)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		if (!O.IsValid() || !O->TryGetArrayField(TEXT("points"), Points)) continue;
		FLeg Leg; Leg.Name = O->GetStringField(TEXT("name"));
		for (const TSharedPtr<FJsonValue>& P : *Points)
		{
			const TArray<TSharedPtr<FJsonValue>>& XYZ = P->AsArray();
			if (XYZ.Num() == 3) Leg.Points.Add(FVector(XYZ[0]->AsNumber(), XYZ[1]->AsNumber(), XYZ[2]->AsNumber()));
		}
		Legs.Add(Leg);
	}
	return Legs.Num() == 4;
}

void UCalmyPlaythroughProbe::Face(ACalmyCharacter* Pawn, const FVector& WorldTarget) const
{
	if (auto* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		FRotator R = (WorldTarget - Pawn->GetInteractionOrigin()).Rotation();
		PC->SetControlRotation(R);
	}
}

bool UCalmyPlaythroughProbe::WalkLeg(ACalmyCharacter* Pawn, const FLeg& Leg, float DT)
{
	using namespace CalmyPlaythrough;
	if (Waypoint >= Leg.Points.Num()) return true;
	const FVector Feet = ToLocal(Pawn->GetActorLocation()) - FVector(0, 0, 90);
	const FVector Target = Leg.Points[Waypoint];
	const FVector Delta = Target - Feet;
	const bool bLast = Waypoint == Leg.Points.Num() - 1;
	if (Delta.Size2D() < (bLast ? ReachCm * .6f : ReachCm) && FMath::Abs(Delta.Z) < 140)
	{
		++Waypoint; BestDistance = BIG_NUMBER; SinceProgress = 0;
		return Waypoint >= Leg.Points.Num();
	}
	const FVector WorldDir = (ToWorld(Target) - ToWorld(Feet)).GetSafeNormal2D();
	// Look where we walk, like a player: the held crate rides in front of the camera yaw.
	if (auto* PC = Cast<APlayerController>(Pawn->GetController())) PC->SetControlRotation(FRotator(-8, WorldDir.Rotation().Yaw, 0));
	const float Scale = bLast ? FMath::Clamp(float(Delta.Size2D()) / 60.f, .35f, 1.f) : 1.f;
	Pawn->MoveIntent(FVector2D(WorldDir) * Scale);
	if (Delta.Size2D() < BestDistance - 8) { BestDistance = float(Delta.Size2D()); SinceProgress = 0; }
	else SinceProgress += DT;
	// A closed watertight door on the way is opened the way a player does it: face it and tap E once, then keep walking.
	if (SinceProgress > 1.2f)
	{
		DoorTapAge += DT;
		ACalmyDoor* Ahead = nullptr;
		const FVector Eye = Pawn->GetInteractionOrigin();
		for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It)
		{
			const FVector ToDoor = It->GetActorLocation() - Pawn->GetActorLocation();
			if (It->IsOpen() || ToDoor.Size() > 260.f || FVector::DotProduct(ToDoor.GetSafeNormal2D(), WorldDir) < .2f) continue;
			if (!Ahead || ToDoor.Size() < (Ahead->GetActorLocation() - Pawn->GetActorLocation()).Size()) Ahead = *It;
		}
		if (Ahead && DoorTapAge > 1.5f)
		{
			DoorTapAge = 0;
			Face(Pawn, Ahead->GetGripLocation());
			Pawn->ServerDoor(Ahead, Ahead->Revision, ++Pawn->LastIssuedSequence);
			++DoorsOpened;
			SinceProgress = 0;
			UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYTHROUGH DOOR id=%d tapped on leg=%s"), Ahead->DoorId, *Leg.Name);
		}
	}
	if (SinceProgress > StallSeconds)
		Finish(false, FString::Printf(TEXT("stalled leg=%s waypoint=%d/%d feet=%s target=%s held=%d"), *Leg.Name, Waypoint, Leg.Points.Num(),
			*Vec(Feet), *Vec(Target), Pawn->HeldCargo != nullptr));
	return false;
}

void UCalmyPlaythroughProbe::Tick(float DT)
{
	using namespace CalmyPlaythrough;
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	const double Now = FPlatformTime::Seconds();
	if (!Start)
	{
		Start = Now;
		FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
		if (Dir.IsEmpty()) Dir = TEXT("ShipPlaythrough-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
	}
	if (Now - Start > 900) { Finish(false, FString::Printf(TEXT("timeout stage=%d"), Stage)); return; }
	auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Pawn = PC ? Cast<ACalmyCharacter>(PC->GetPawn()) : nullptr;
	if (!Pawn) return;
	if (PC->IsPauseMenuOpen()) PC->ResumeFromMenu();
	StageAge += DT;
	auto* Voyage = ACalmyVoyageDirector::Find(GetWorld());
	auto* Shift = ACalmyShift::Find(GetWorld());
	auto* Leak = ACalmyLeak::Find(GetWorld());
	auto* Fuel = ACalmyFuelStation::Find(GetWorld());

	if (Stage == 0)
	{
		if (Now - Start < 6) return;
		if (!LoadLegs()) { Finish(false, TEXT("legs_missing")); return; }
		if (!Voyage || !Shift || !Leak || !Fuel) { Finish(false, FString::Printf(TEXT("fixtures voyage=%d shift=%d leak=%d fuel=%d"), Voyage != nullptr, Shift != nullptr, Leak != nullptr, Fuel != nullptr)); return; }
		// Put the pawn exactly on the first surveyed point (the weather-deck player start the legs were planned from).
		Pawn->SetActorLocation(ToWorld(Legs[0].Points[0] + FVector(0, 0, 92)), false, nullptr, ETeleportType::TeleportPhysics);
		if (!Voyage->TryBeginWatch(Pawn)) { Finish(false, TEXT("begin_watch_denied")); return; }
		Photo(TEXT("watch-start")); Step(TEXT("watch_started")); Stage = 1;
		return;
	}
	if (Stage == 1)
	{
		if (StageAge < 1.5f) return;
		if (WalkLeg(Pawn, Legs[0], DT)) { Photo(TEXT("at-leak-valve")); Step(TEXT("reached_leak_valve")); Stage = 2; }
		return;
	}
	if (Stage == 2)
	{
		Face(Pawn, Leak->GetActorLocation());
		if (bSawLeak && (Leak->State.Phase == ECalmyLeakPhase::Draining || Leak->State.Phase == ECalmyLeakPhase::Resolved))
		{
			Photo(TEXT("leak-sealed")); Step(TEXT("leak_sealed")); Stage = 3;
			return;
		}
		if (Leak->State.Phase != ECalmyLeakPhase::Leaking)
		{
			if (StageAge > 45) Finish(false, FString::Printf(TEXT("leak_never_started phase=%d next=%d"), int32(Leak->State.Phase), Leak->State.NextEventSeconds));
			return;
		}
		if (!bSawLeak) { bSawLeak = true; Photo(TEXT("leak-flooding-hold")); }
		if (!Leak->CanRepair(Pawn))
		{
			if (StageAge > 50) Finish(false, FString::Printf(TEXT("cannot_repair_from_here feet=%s valve=%s dist=%.0f vel=%.0f"),
				*Vec(ToLocal(Pawn->GetActorLocation())), *Vec(ToLocal(Leak->GetActorLocation())), FVector::Dist(Pawn->GetActorLocation(), Leak->GetActorLocation()), Pawn->GetVelocity().Size()));
			return;
		}
		PulseAge += DT;
		if (PulseAge > .35f) { PulseAge = 0; Pawn->ServerRepair(Leak, Leak->State.Round, true, ++Pawn->LastRepairIssued); }
		if (Leak->State.Repair > 40 && Shots < 3) Photo(TEXT("repairing-valve"));
		return;
	}
	if (Stage == 3)
	{
		if (WalkLeg(Pawn, Legs[1], DT))
		{
			// The standard crate from the curated manifest (solo player: bulky needs two hands).
			for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It) if (It->Kind == ECalmyCargoKind::Standard && !It->Carry.bDelivered) { Crate = *It; break; }
			if (!Crate.IsValid()) { Finish(false, TEXT("no_standard_crate")); return; }
			Step(TEXT("reached_cargo")); Stage = 4;
		}
		return;
	}
	if (Stage == 4)
	{
		ACalmyCargo* C = Crate.Get();
		if (!C) { Finish(false, TEXT("crate_lost")); return; }
		if (Pawn->HeldCargo == C) { Photo(TEXT("crate-picked-up")); Step(TEXT("picked_up_crate")); DeliveredBefore = Shift->State.Delivered; Stage = 5; return; }
		if (FCalmyPickupFixture::Approach(Pawn, C) && PulseAge > .6f) { PulseAge = 0; Pawn->ServerGrab(C, ++Pawn->LastIssuedSequence); }
		PulseAge += DT;
		if (StageAge > 25) Finish(false, FString::Printf(TEXT("cannot_pick_up crate=%d feedback=%s"), C->CargoId, *ACalmyCharacter::GrabFeedbackText(Pawn->GetGrabFeedback(C))));
		return;
	}
	if (Stage == 5)
	{
		if (!Pawn->HeldCargo) { Finish(false, FString::Printf(TEXT("dropped_crate_on_the_way feet=%s"), *Vec(ToLocal(Pawn->GetActorLocation())))); return; }
		if (StageAge > 3 && StageAge < 3 + DT) Photo(TEXT("carrying-up-hold-stair"));
		if (WalkLeg(Pawn, Legs[2], DT))
		{
			// Face forward along the side lane so the crate is set down inside the zone ahead.
			if (auto* APC = Cast<APlayerController>(Pawn->GetController()))
				APC->SetControlRotation(FRotator(-20, (ToWorld(FVector(100, 0, 0)) - ToWorld(FVector::ZeroVector)).Rotation().Yaw, 0));
			Step(TEXT("reached_delivery")); Stage = 6;
		}
		return;
	}
	if (Stage == 6)
	{
		if (StageAge > .6f && Pawn->HeldCargo) Pawn->RequestDrop();
		if (Shift->State.Delivered > DeliveredBefore) { Photo(TEXT("crate-delivered")); Step(TEXT("crate_delivered")); Voyage->ApplyFuelPulse(-45.f); Stage = 7; return; }
		if (StageAge > 12)
		{
			const ACalmyCargo* C = Crate.Get();
			Finish(false, FString::Printf(TEXT("delivery_not_accepted crate_at=%s zone=%s eligible=%d"), C ? *Vec(ToLocal(C->GetActorLocation())) : TEXT("?"),
				*Vec(ToLocal(Shift->GetActorLocation())), C ? Shift->IsEligible(C) : 0));
		}
		return;
	}
	if (Stage == 7)
	{
		if (WalkLeg(Pawn, Legs[3], DT)) { Step(TEXT("reached_fuel_station")); Stage = 8; }
		return;
	}
	if (Stage == 8)
	{
		Face(Pawn, Fuel->GetActorLocation());
		const int32 Percent = Voyage->State.FuelPercent;
		if (Percent >= 95) { Photo(TEXT("refueled")); Step(TEXT("refueled")); Finish(true, FString::Printf(TEXT("fuel=%d delivered=%d leak_phase=%d doors_opened=%d shots=%d"), Percent, Shift->State.Delivered, int32(Leak->State.Phase), DoorsOpened, Shots)); return; }
		PulseAge += DT;
		if (PulseAge > .35f) { PulseAge = 0; Pawn->ServerFuel(true, ++Pawn->LastRepairIssued); }
		if (StageAge > 3 && StageAge < 3 + DT) Photo(TEXT("refueling-bunker-station"));
		if (StageAge > 40) Finish(false, FString::Printf(TEXT("refuel_stuck fuel=%d can=%d dist=%.0f"), Percent, Fuel->CanRefuel(Pawn), FVector::Dist(Pawn->GetActorLocation(), Fuel->GetActorLocation())));
		return;
	}
}
