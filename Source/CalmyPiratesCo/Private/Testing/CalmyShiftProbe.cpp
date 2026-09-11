#include "Testing/CalmyShiftProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyShift.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyShiftProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyShiftProbe="), Value);
#endif
}

void UCalmyShiftProbe::Finish(bool bSuccess, const FString& Details)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT %s %s %s"), *Role, bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Details);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void UCalmyShiftProbe::Tick(float DeltaSeconds)
{
	if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyShiftProbe="), Role);
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if ((Role == TEXT("Host") && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Role != TEXT("Host") && GetWorld()->GetNetMode() != NM_Client)) return;
	Age += DeltaSeconds;
	auto* Shift = ACalmyShift::Find(GetWorld());
	if (Age > 180) { Finish(false, FString::Printf(TEXT("timeout stage=%d cargo=%d delivered=%d round=%d"), Stage, CargoIndex, Shift ? Shift->State.Delivered : -1, Shift ? Shift->State.Round : -1)); return; }
	ACalmyCharacter* Local = nullptr;
	ACalmyCharacter* Remote = nullptr;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsLocallyControlled()) Local = *It;
		else if (!Remote || It->ResetCount > Remote->ResetCount) Remote = *It;
	}
	TMap<int32, ACalmyCargo*> Crates;
	int32 ActualCrates = 0;
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It) { Crates.Add(It->CargoId, *It); ++ActualCrates; }
	if (!Local || !Shift || !Crates.Contains(1) || !Crates.Contains(2) || !Crates.Contains(3)) return;
	if (!bStarted)
	{
		bStarted = true;
		UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT %s START"), *Role);
		GLog->Flush();
	}
	if (ActualCrates != 3) { Finish(false, TEXT("duplicate_or_missing_physical_cargo")); return; }
	if (Role == TEXT("LateJoin"))
	{
		if (Shift->State.Phase == ECalmyShiftPhase::Complete && Shift->State.Delivered == 3 && Shift->State.Required == 3 &&
			Shift->State.Rescues == 3 && Shift->State.SecondsRemaining > 0 && Shift->State.FinalPayout > 0 &&
			Shift->State.PerformanceStars >= 1 && Crates[1]->Carry.bDelivered && Crates[2]->Carry.bDelivered && Crates[3]->Carry.bDelivered)
			Finish(true, TEXT("completed_contract_snapshot=1 economy_snapshot=1 delivered_flags=1 rescue_count=3 exact_cargo_count=3"));
		return;
	}
	if (Role == TEXT("Host"))
	{
		if (Stage == 0 && Remote && Remote->HeldCargo == Crates[1])
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT FIXTURE server moves held-cargo pawn overboard"));
			Remote->SetActorLocation(FVector(0, 1000, -280), false, nullptr, ETeleportType::TeleportPhysics);
			Stage = 1;
		}
		else if (Stage == 1 && Remote && Remote->ResetCount == 1 && Crates[1]->ResetCount == 1 && Crates[2]->Carry.State == ECalmyCarryState::Thrown)
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT FIXTURE server moves already-thrown crate overboard"));
			Crates[2]->SetActorLocation(FVector(0, -1100, -350), false, nullptr, ETeleportType::TeleportPhysics);
			Stage = 2;
		}
		else if (Stage == 2 && Shift->State.Phase == ECalmyShiftPhase::Complete && Shift->State.FinalPayout > 0 && Shift->State.PerformanceStars >= 1)
		{
			if (!Remote || Remote->ResetCount != 1 || Crates[1]->ResetCount != 1 || Crates[2]->ResetCount != 1 || Shift->State.Rescues != 3)
			{ Finish(false, TEXT("reset_count_mismatch")); return; }
			Stage = 3; StageTime = Age;
		}
		else if (Stage == 3 && Age - StageTime > 30 && Remote && Remote->RejectedCommands >= 3)
		{
			const uint32 Sequence = ++Local->LastIssuedSequence;
			Local->ServerRestart(1, Sequence);
			Local->ServerRestart(1, Sequence);
			Stage = 4; StageTime = Age;
		}
		else if (Stage == 4 && Age - StageTime > .3f)
		{
			Local->ServerRestart(1, ++Local->LastIssuedSequence); // New sequence, stale round.
			Stage = 5; StageTime = Age;
		}
		else if (Stage == 5 && Age - StageTime > 6)
		{
			Finish(Shift->State.Round == 2 && Shift->State.Delivered == 0 && Shift->State.Rescues == 0 &&
				Local->RejectedCommands >= 2 && !Crates[1]->Carry.bDelivered && !Crates[2]->Carry.bDelivered && !Crates[3]->Carry.bDelivered,
				TEXT("held_fall_reset=1 thrown_fall_reset=1 physical_deliveries=3 server_economy=1 remote_restart_denied=1 host_restart=1 replay_denied=1 exact_cargo_count=3"));
		}
		return;
	}

	auto Advance = [this, Local](int32 Next)
	{
		Stage = Next; StageTime = Age;
		UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT Client STAGE %d cargo=%d pos=%s"), Stage, CargoIndex, *Local->GetActorLocation().ToString());
		GLog->Flush();
	};
	auto MoveTo = [Local](FVector2D Target)
	{
		const FVector2D Delta = Target - FVector2D(Local->GetActorLocation());
		if (Delta.Size() < 18.f) return true;
		Local->MoveIntent(Delta.GetSafeNormal());
		return false;
	};
    if(Stage==0) FCalmyPickupFixture::Aim(Local,Crates[1]);
    else if(Stage==2) FCalmyPickupFixture::Aim(Local,Crates[2]);
    else if(Stage==11 && !Local->HeldCargo) FCalmyPickupFixture::Aim(Local,Crates[CargoIndex]);
    else FCalmyPickupFixture::Forward(Local);
	if (Stage == 0 && Age > 1 && Remote && FCalmyPickupFixture::Approach(Local,Crates[1]))
	{
		const uint32 Sequence = ++Local->LastIssuedSequence;
		Local->ServerGrab(Crates[1], Sequence);
		Local->ServerGrab(Crates[1], Sequence);
		Advance(1);
	}
	else if (Stage == 1 && Local->ResetCount == 1 && Crates[1]->ResetCount == 1 && !Local->HeldCargo) Advance(2);
	else if (Stage == 2 && Age - StageTime > .4f && FCalmyPickupFixture::Approach(Local,Crates[2])) { Local->ServerGrab(Crates[2], ++Local->LastIssuedSequence); Advance(3); }
	else if (Stage == 3 && Local->HeldCargo == Crates[2] && Age - StageTime > .4f) { Local->RequestThrow(); Advance(4); }
	else if (Stage == 4 && Crates[2]->ResetCount == 1 && Crates[2]->Carry.State == ECalmyCarryState::Free) Advance(10);
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
	else if (Stage == 17 && Shift->State.Delivered == CargoIndex && Crates[CargoIndex]->Carry.bDelivered)
	{
		UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT Client DELIVERED %d"), CargoIndex);
		GLog->Flush();
		if (CargoIndex < 3) { ++CargoIndex; Advance(10); }
		else
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SHIFT Client COMPLETE"));
			GLog->Flush();
			const uint32 Sequence = ++Local->LastIssuedSequence;
			Local->ServerRestart(1, Sequence);
			Local->ServerRestart(1, Sequence);
			Advance(20);
		}
	}
	else if (Stage == 20 && Shift->State.Round == 2) Advance(21);
	else if (Stage == 21 && Age - StageTime > 2)
	{
		Finish(Shift->State.Delivered == 0 && Shift->State.Rescues == 0 && Shift->State.FinalPayout == 0 &&
			Shift->State.SecondsRemaining > 0 && Local->RejectedCommands >= 3 && Local->ResetCount == 1 &&
			!Local->HeldCargo && !Crates[1]->Carry.bDelivered && !Crates[2]->Carry.bDelivered && !Crates[3]->Carry.bDelivered,
			TEXT("owning_client_movement=1 held_and_thrown_reset=1 three_deliveries=1 economy_reset=1 unauthorized_restart_rejected=1 new_round_snapshot=1"));
	}
}
