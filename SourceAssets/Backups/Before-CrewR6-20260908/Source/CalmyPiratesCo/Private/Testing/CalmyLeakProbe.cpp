#include "Testing/CalmyLeakProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyLeakProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value; const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyLeakProbe="), Value);
#endif
}

void UCalmyLeakProbe::Finish(bool Success, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_LEAK %s %s %s"), *Role, Success ? TEXT("PASS") : TEXT("FAIL"), *Detail);
	GLog->Flush(); Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}

void UCalmyLeakProbe::Tick(float DeltaSeconds)
{
	if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyLeakProbe="), Role);
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if ((Role == TEXT("Host") && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Role != TEXT("Host") && GetWorld()->GetNetMode() != NM_Client)) return;
	Age += DeltaSeconds;
	auto* Leak = ACalmyLeak::Find(GetWorld()); auto* Shift = ACalmyShift::Find(GetWorld());
	if (Age > 180) { Finish(false, FString::Printf(TEXT("timeout stage=%d water=%d repair=%d"), Stage, Leak ? Leak->State.Water : -1, Leak ? Leak->State.Repair : -1)); return; }
	ACalmyCharacter* Local = nullptr; int32 Players = 0;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It) { ++Players; if (It->IsLocallyControlled()) Local = *It; }
	TMap<int32, ACalmyCargo*> Crates;
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It) Crates.Add(It->CargoId, *It);
	if (!Local || !Leak || !Shift || Crates.Num() != 3 || !Crates.Contains(1) || !Crates.Contains(2) || !Crates.Contains(3)) return;
	if (!bStarted) { bStarted = true; UE_LOG(LogTemp, Display, TEXT("CALMY_LEAK %s START"), *Role); GLog->Flush(); }
	bSawWarning |= Leak->State.Phase == ECalmyLeakPhase::Warning;
	bSawPowerDegraded |= Leak->State.bPowerDegraded;
	bSawDirectorSnapshot |= Leak->State.DirectorSeed == 203 && Leak->State.EventSerial == 1;
	auto Advance = [this, Local, Leak](int32 Next)
	{
		Stage = Next; StageTime = Age;
		UE_LOG(LogTemp, Display, TEXT("CALMY_LEAK %s STAGE %d water=%d repair=%d pos=%s"), *Role, Stage, Leak->State.Water, Leak->State.Repair, *Local->GetActorLocation().ToString()); GLog->Flush();
	};
	auto MoveTo = [Local](FVector2D Target)
	{
		const FVector2D Delta = Target - FVector2D(Local->GetActorLocation());
		if (Delta.Size() < 18) return true;
		Local->MoveIntent(Delta.GetSafeNormal()); return false;
	};
	if (Role == TEXT("Client"))
	{
        if(Stage==2) FCalmyPickupFixture::Aim(Local,Crates[3]);else FCalmyPickupFixture::Forward(Local);
		if (Stage == 0 && Leak->State.Phase == ECalmyLeakPhase::Leaking)
		{
			const uint32 Seq = ++Local->LastRepairIssued;
			Local->ServerRepair(Leak, 1, true, Seq); Local->ServerRepair(Leak, 1, true, Seq); // Range + replay rejection.
			Advance(1);
		}
		else if (Stage == 1 && MoveTo(FVector2D(-530, 340))) Advance(2);
		else if (Stage == 2 && Age - StageTime > .5f && FCalmyPickupFixture::Approach(Local,Crates[3]))
		{
			Local->ServerGrab(Crates[3], ++Local->LastIssuedSequence); Advance(3);
		}
		else if (Stage == 3 && Local->HeldCargo == Crates[3])
		{
			Local->ServerRepair(Leak, 1, true, ++Local->LastRepairIssued); Advance(4);
		}
		else if (Stage == 4 && MoveTo(FVector2D(-530, 180))) Advance(40);
		else if (Stage == 40 && Age - StageTime > .4f) { Local->RequestDrop(); Advance(41); }
		else if (Stage == 41 && !Local->HeldCargo && MoveTo(FVector2D(-530, 340))) Advance(5);
		else if (Stage == 5 && Leak->State.Water >= 55 && !Local->HeldCargo)
		{
			if (Local->GetCharacterMovement()->GetMaxSpeed() != 315.f || !Leak->State.bPowerDegraded) { Finish(false, TEXT("slowdown_or_power_not_replicated")); return; }
			Local->BeginRepairInput(); Advance(6);
		}
		else if (Stage == 6 && Leak->State.Repair >= 20) { Local->EndRepairInput(); Advance(7); }
		else if (Stage == 7 && Age - StageTime > 1) { SavedProgress = Leak->State.Repair; Advance(8); }
		else if (Stage == 8 && Age - StageTime > 1)
		{
			if (Leak->State.Repairer || Leak->State.Repair != SavedProgress) { Finish(false, TEXT("release_did_not_pause_progress")); return; }
			UE_LOG(LogTemp, Display, TEXT("CALMY_LEAK Client READY partial=%d water=%d"), SavedProgress, Leak->State.Water); GLog->Flush(); Advance(9);
		}
		else if (Stage == 9 && Players == 3) { Local->BeginRepairInput(); Advance(10); }
		else if (Stage == 10 && Leak->State.Repair >= 45 && Leak->State.Repairer == Local)
			Finish(bSawWarning && bSawPowerDegraded && bSawDirectorSnapshot && Local->RejectedCommands >= 3,
				TEXT("warning=1 director_seed_and_serial=1 replicated_power_degraded=1 range_rejected=1 replay_rejected=1 cargo_blocks_repair=1 slow_deck=1 release_pauses=1 disconnect_while_repairing=1"));
		return;
	}
	if (Role == TEXT("LateJoin"))
	{
		if (Stage == 0 && Leak->State.Phase == ECalmyLeakPhase::Leaking && Leak->State.Water >= 50 && Leak->State.Repair >= 20 && Leak->State.Repair < 100 &&
			Leak->State.bPowerDegraded && Leak->State.DirectorSeed == 203 && Leak->State.EventSerial == 1)
		{
			if (Local->GetCharacterMovement()->GetMaxSpeed() != 315.f) { Finish(false, TEXT("late_flood_speed_missing")); return; }
			Advance(1);
		}
		else if (Stage == 1 && Leak->State.Repairer && Leak->State.Repairer != Local) { DepartingWorker = Leak->State.Repairer; Advance(2); }
		else if (Stage == 2 && !DepartingWorker.IsValid() && !Leak->State.Repairer && Players == 2) Advance(3);
		else if (Stage == 3 && MoveTo(FVector2D(-560, 340))) Advance(4);
		else if (Stage == 4 && Age - StageTime > .5f)
		{
			Local->ServerRepair(Leak, 0, true, ++Local->LastRepairIssued); Local->BeginRepairInput(); Advance(5);
		}
		else if (Stage == 5 && Leak->State.Phase == ECalmyLeakPhase::Resolved)
		{
			Local->EndRepairInput();
			if (Local->GetCharacterMovement()->GetMaxSpeed() != 450.f || Leak->State.bPowerDegraded || Local->RejectedCommands < 1) { Finish(false, TEXT("power_recovery_or_stale_rejection_failed")); return; }
			Advance(6);
		}
		else if (Stage == 6 && Shift->State.Round == 2 && Leak->State.Round == 2) Advance(7);
		else if (Stage == 7 && Age - StageTime > 1)
			Finish(Leak->State.Phase == ECalmyLeakPhase::Idle && Leak->State.DirectorSeed == 203 && Leak->State.EventSerial == 0 &&
				Leak->State.NextEventSeconds > 0 && Leak->State.NextEventSeconds <= 24 && Leak->State.Water == 0 && Leak->State.Repair == 0 &&
				!Leak->State.Repairer && Shift->State.Delivered == 0,
				TEXT("late_active_director_snapshot=1 late_active_power_snapshot=1 worker_disconnect_cleared=1 takeover=1 stale_round_rejected=1 power_recovered=1 repair_and_drain=1 new_round_schedule=1"));
		return;
	}
	// The host keeps the route clear, then completes the physical contract after recovery.
    if(Stage==11 && !Local->HeldCargo) FCalmyPickupFixture::Aim(Local,Crates[CargoIndex]);else FCalmyPickupFixture::Forward(Local);
	if (Stage == 0 && MoveTo(FVector2D(-650, -150))) Advance(1);
	else if (Stage == 1 && Leak->State.Phase == ECalmyLeakPhase::Resolved) Advance(10);
	else if (Stage == 10 && MoveTo(FVector2D(-600, -250))) Advance(11);
	else if (Stage == 11 && Age - StageTime > .4f)
	{
		if (Local->HeldCargo == Crates[CargoIndex]) Advance(12);
		else if(FCalmyPickupFixture::Approach(Local,Crates[CargoIndex])) { Local->ServerGrab(Crates[CargoIndex], ++Local->LastIssuedSequence); StageTime = Age; }
	}
	else if (Stage == 12 && MoveTo(FVector2D(-250, -250))) Advance(13);
	else if (Stage == 13 && MoveTo(FVector2D(300, -250))) Advance(14);
	else if (Stage == 14 && MoveTo(FVector2D(350, -350 + (CargoIndex - 1) * 150))) Advance(15);
	else if (Stage == 15 && MoveTo(FVector2D(545, -350 + (CargoIndex - 1) * 150))) Advance(16);
	else if (Stage == 16 && Age - StageTime > .4f) { Local->RequestDrop(); Advance(17); }
	else if (Stage == 17 && Shift->State.Delivered == CargoIndex)
	{
		if (CargoIndex < 3) { ++CargoIndex; Advance(18); }
		else Advance(20);
	}
	else if (Stage == 18 && MoveTo(FVector2D(300, -250))) Advance(19);
	else if (Stage == 19 && MoveTo(FVector2D(-250, -250))) Advance(10);
	else if (Stage == 20 && Age - StageTime > 2)
	{
		if (Leak->State.Phase != ECalmyLeakPhase::Suspended) { Finish(false, TEXT("completed_shift_left_emergency_running")); return; }
		Local->RequestRestart();
		NewRoundInitialSeconds = Leak->State.NextEventSeconds;
		Local->ServerRepair(Leak, 1, true, ++Local->LastRepairIssued); // Old generation after synchronous host restart.
		Advance(21);
	}
	else if (Stage == 21 && Age - StageTime > 5)
		Finish(bSawWarning && bSawPowerDegraded && bSawDirectorSnapshot && Shift->State.Round == 2 && Shift->State.Delivered == 0 && Leak->State.Phase == ECalmyLeakPhase::Idle &&
			Leak->State.Round == 2 && Leak->State.DirectorSeed == 203 && Leak->State.EventSerial == 0 &&
			NewRoundInitialSeconds >= 18 && NewRoundInitialSeconds <= 24 && Leak->State.NextEventSeconds > 0 && Leak->State.NextEventSeconds <= NewRoundInitialSeconds &&
			Leak->State.Water == 0 && Leak->State.Repair == 0 && !Leak->State.bPowerDegraded && !Leak->State.Repairer && Local->RejectedCommands >= 1,
			TEXT("seeded_director=1 warning=1 power_degraded_and_recovered=1 repair_takeover=1 recovered_deck=1 physical_deliveries=3 contract_complete=1 host_restart=1 new_round_schedule=1 stale_repair_denied=1"));
}
