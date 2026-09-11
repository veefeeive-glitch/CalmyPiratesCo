#include "Testing/CalmyCargoProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyShift.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyCargoProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyCargoProbe="), Value);
#endif
}

void UCalmyCargoProbe::Finish(bool Success, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_CARGO %s %s %s"), *Role, Success ? TEXT("PASS") : TEXT("FAIL"), *Detail);
	GLog->Flush(); Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}

void UCalmyCargoProbe::Tick(float DeltaSeconds)
{
	if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyCargoProbe="), Role);
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if ((Role == TEXT("Host") && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Role != TEXT("Host") && GetWorld()->GetNetMode() != NM_Client)) return;
	Age += DeltaSeconds;
	if (Age > 160) { Finish(false, FString::Printf(TEXT("timeout stage=%d"), Stage)); return; }
	ACalmyCharacter* Local = nullptr;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsLocallyControlled()) Local = *It;
		else if (!Peer.IsValid() && !bStarted) Peer = *It;
	}
	// Host starts before the first peer; retain that peer when it joins, not a later observer.
	if (Role == TEXT("Host") && !Peer.IsValid() && Stage <= 1)
		for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It) if (!It->IsLocallyControlled()) { Peer = *It; break; }
	TMap<int32, ACalmyCargo*> Crates;
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It) Crates.Add(It->CargoId, *It);
	auto* Shift = ACalmyShift::Find(GetWorld());
	if (!Local || !Shift || !Crates.Contains(1) || !Crates.Contains(2) || !Crates.Contains(3)) return;
	auto* Heavy = Crates[1]; auto* Unstable = Crates[2]; auto* Fragile = Crates[3];
	if (!bStarted)
	{
		bStarted = true;
		UE_LOG(LogTemp, Display, TEXT("CALMY_CARGO %s START"), *Role); GLog->Flush();
	}
	if (Role == TEXT("LateJoin"))
	{
		if (Heavy->Kind == ECalmyCargoKind::Bulky && Unstable->Kind == ECalmyCargoKind::Unstable && Fragile->Kind == ECalmyCargoKind::Fragile &&
			Heavy->Condition.ForcedDrops == 1 && Unstable->Condition.ForcedDrops == 1 && Unstable->Condition.Handoffs == 1 && Fragile->Condition.Integrity < 100)
			Finish(true, TEXT("cargo_kinds=1 conditions=1 handoff_history=1 damage_snapshot=1"));
		return;
	}
	auto MoveTo = [Local](FVector2D Target)
	{
		const FVector2D Delta = Target - FVector2D(Local->GetActorLocation());
		if (Delta.Size() < 18) return true;
		Local->MoveIntent(Delta.GetSafeNormal()); return false;
	};
	auto Advance = [this, Local](int32 Next)
	{
		Stage = Next; StageTime = Age;
		UE_LOG(LogTemp, Display, TEXT("CALMY_CARGO %s STAGE %d pos=%s"), *Role, Stage, *Local->GetActorLocation().ToString()); GLog->Flush();
	};
	if (Role == TEXT("Host"))
	{
		if (Stage == 0)
		{
			Local->ServerGrab(Heavy, ++Local->LastIssuedSequence); // Before the second player connects.
			if (Local->HeldCargo || Local->RejectedCommands < 1) { Finish(false, TEXT("solo_heavy_lift_was_not_rejected")); return; }
			Advance(1);
		}
		// Client's -520 goal accepts an 18 cm arrival radius. Trigger throughout
        // that band, including the slower coordinated-carry stopping position.
		else if (Stage == 1 && Peer.IsValid() && Peer->GetActorLocation().X > -540 && Heavy->Carry.Holder == Peer.Get())
		{
			MoveTo(FVector2D(-650, -280)); // Normal host input moves the helper out of range.
		}
		if (Stage == 1 && Heavy->Condition.ForcedDrops == 1) Advance(2);
		else if (Stage == 2 && MoveTo(FVector2D(-650, 50))) Advance(3);
		else if (Stage == 3 && Unstable->Carry.Holder == Local && Unstable->Condition.Handoffs == 1)
		{
			Local->ServerHandoff(Peer.Get(), ++Local->LastIssuedSequence); // Immediate reverse transfer must fail.
			Advance(4);
		}
		else if (Stage == 4 && Shift->State.Delivered == 1 && Shift->State.DamagedDelivered == 1 && !Peer.IsValid())
		{
			Finish(Local->RejectedCommands >= 2 && Heavy->Condition.ForcedDrops == 1 && Unstable->Condition.ForcedDrops == 1 &&
				Unstable->Condition.Handoffs == 1 && Fragile->Carry.bDelivered && Fragile->Condition.Integrity < 100,
				TEXT("solo_lift_denied=1 helper_separation_drop=1 transfer_cooldown=1 unstable_drop=1 real_collision_damage=1 damaged_delivery=1"));
		}
		return;
	}
	if (!Peer.IsValid()) return;
    if(Stage==0) FCalmyPickupFixture::Aim(Local,Heavy);
    else if(Stage==4 || Stage==6) FCalmyPickupFixture::Aim(Local,Unstable);
    else if(Stage==9 || Stage==13) FCalmyPickupFixture::Aim(Local,Fragile);
    else FCalmyPickupFixture::Forward(Local);
	if (Stage == 0 && Age > 1 && FCalmyPickupFixture::Approach(Local,Heavy)) { Local->ServerGrab(Heavy, ++Local->LastIssuedSequence); Advance(1); }
	else if (Stage == 1 && Local->HeldCargo == Heavy && Age - StageTime > .3f)
	{
		const float ExpectedSpeed=260.f*Heavy->Condition.TeamPace/100.f;
		if (Heavy->Condition.TeamPace<55 || Heavy->Condition.TeamPace>100 ||
            !FMath::IsNearlyEqual(Local->GetCharacterMovement()->GetMaxSpeed(),ExpectedSpeed))
        { Finish(false, TEXT("heavy_team_speed_not_replicated")); return; }
		Local->RequestThrow(); Advance(2);
	}
	else if (Stage == 2 && MoveTo(FVector2D(-520, 220))) Advance(3);
	else if (Stage == 3 && Heavy->Condition.ForcedDrops == 1 && !Local->HeldCargo) Advance(4);
	else if (Stage == 4 && Age - StageTime > .5f && FCalmyPickupFixture::Approach(Local,Unstable))
	{
		Local->ServerGrab(Unstable, ++Local->LastIssuedSequence); Advance(5);
	}
	else if (Stage == 5 && Local->HeldCargo == Unstable && Unstable->Condition.Stability <= 60 &&
		FVector::Distance(Local->GetActorLocation(), Peer->GetActorLocation()) < 245)
	{
		const uint32 Sequence = ++Local->LastIssuedSequence;
		Local->ServerHandoff(Peer.Get(), Sequence); Local->ServerHandoff(Peer.Get(), Sequence);
		Advance(6);
	}
	else if (Stage == 6 && Unstable->Condition.Handoffs == 1 && Unstable->Condition.ForcedDrops == 1 && Unstable->Carry.State == ECalmyCarryState::Free)
	{
		Local->ServerGrab(Unstable, ++Local->LastIssuedSequence); // Exhausted cargo is not instantly liftable.
		Advance(7);
	}
	else if (Stage == 7 && Unstable->Condition.Stability >= 80 && !Local->HeldCargo) Advance(8);
	// The exhausted crate lands in the old straight-line approach at (-540,45).
	// Walk around it and the helper; do not teleport or disable their collision.
	else if (Stage == 8 && MoveTo(FVector2D(-820, 220))) Advance(81);
	else if (Stage == 81 && MoveTo(FVector2D(-820, -180))) Advance(82);
	else if (Stage == 82 && MoveTo(FVector2D(-550, -140))) Advance(9);
	else if (Stage == 9 && Age - StageTime > .4f && FCalmyPickupFixture::Approach(Local,Fragile))
	{
		Local->ServerGrab(Fragile, ++Local->LastIssuedSequence); Advance(10);
	}
	else if (Stage == 10 && Local->HeldCargo == Fragile && MoveTo(FVector2D(-340, 0))) Advance(11);
	else if (Stage == 11 && Age - StageTime > .4f) { Local->RequestThrow(); Advance(12); }
	else if (Stage == 12 && Fragile->Condition.DamageEvents > 0 && Fragile->Condition.Integrity < 100)
	{
		UE_LOG(LogTemp, Display, TEXT("CALMY_CARGO Client READY")); GLog->Flush(); Advance(13);
	}
	else if (Stage == 13 && Age - StageTime > 20 && Fragile->Carry.State == ECalmyCarryState::Free && FCalmyPickupFixture::Approach(Local,Fragile))
	{
		Local->ServerGrab(Fragile, ++Local->LastIssuedSequence); Advance(14);
	}
	else if (Stage == 14 && Local->HeldCargo == Fragile && MoveTo(FVector2D(-250, -250))) Advance(15);
	else if (Stage == 15 && MoveTo(FVector2D(350, -250))) Advance(16);
	else if (Stage == 16 && MoveTo(FVector2D(545, -250))) Advance(17);
	else if (Stage == 17 && Age - StageTime > .4f) { Local->RequestDrop(); Advance(18); }
	else if (Stage == 18 && Shift->State.Delivered == 1 && Shift->State.DamagedDelivered == 1) Advance(19);
	else if (Stage == 19 && Age - StageTime > 2)
	{
		Finish(Local->RejectedCommands >= 3 && Unstable->Condition.Handoffs == 1 && Fragile->Carry.bDelivered,
			TEXT("heavy_slow_carry=1 heavy_throw_denied=1 helper_loss=1 handoff=1 replay_denied=1 unstable_rest=1 real_fragile_impact=1 damaged_delivery=1"));
	}
}
