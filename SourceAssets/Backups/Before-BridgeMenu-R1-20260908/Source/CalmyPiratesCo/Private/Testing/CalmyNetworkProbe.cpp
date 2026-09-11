#include "Testing/CalmyNetworkProbe.h"
#include "Testing/CalmyPickupFixture.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyNetworkProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString ProbeRole;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyProbe="), ProbeRole);
#endif
}

void UCalmyNetworkProbe::Finish(bool Success, const FString& Details)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_PROBE %s %s %s"), *Role, Success ? TEXT("PASS") : TEXT("FAIL"), *Details);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}

void UCalmyNetworkProbe::Tick(float DeltaSeconds)
{
	if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyProbe="), Role);
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if ((Role == TEXT("Host") && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Role != TEXT("Host") && GetWorld()->GetNetMode() != NM_Client)) return;
	Age += DeltaSeconds;
	if (Age > 120.f)
	{
		Finish(false, FString::Printf(TEXT("timeout stage=%d moved=%d held=%d follow=%d thrown=%d knocked=%d recovering=%d recovered=%d"),
			Stage, bMoved, bHeld, bFollowed, bThrown, bKnocked, bRecovering, bRecovered));
		return;
	}
	ACalmyCharacter* Local = nullptr;
	ACalmyCharacter* Other = nullptr;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsLocallyControlled()) Local = *It;
		else if (!Other || It->GetActorLocation().Y < Other->GetActorLocation().Y) Other = *It;
	}
	ACalmyCargo* Cargo = nullptr;
	ACalmyCargo* FarCargo = nullptr;
	for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
	{
		if (It->CargoId == 1) Cargo = *It;
		if (It->CargoId == 3) FarCargo = *It;
	}
	if (!Local || !Cargo) return;
	if (!bStarted)
	{
		bStarted = true;
		StartPosition = Local->GetActorLocation();
		UE_LOG(LogTemp, Display, TEXT("CALMY_PROBE %s START pos=%s"), *Role, *StartPosition.ToString());
		GLog->Flush();
	}
	if (Role == TEXT("LateJoin"))
	{
		if (Cargo->Carry.State == ECalmyCarryState::Held && IsValid(Cargo->Carry.Holder) &&
			Cargo->Carry.Holder->HeldCargo == Cargo && FVector::Distance(Cargo->GetActorLocation(), Cargo->Carry.Holder->GetCarryLocation()) < 55)
			Finish(true, TEXT("current_held_snapshot=1 holder_resolved=1 follow=1"));
		return;
	}
	ACalmyCharacter* Victim = Role == TEXT("Host") ? Local : Other;
	if (Victim)
	{
		bKnocked |= Victim->Lifecycle == ECalmyPlayerLifecycleState::KnockedDown;
		bRecovering |= Victim->Lifecycle == ECalmyPlayerLifecycleState::Recovering;
		bRecovered |= bKnocked && bRecovering && Victim->IsControlled() && Victim->KnockdownCount == 1 && Victim->RecoveryCount == 1;
	}
	bThrown |= Cargo->Carry.State == ECalmyCarryState::Thrown;
	if (Role == TEXT("Client"))
	{
        if(Stage==0 || Stage==3) FCalmyPickupFixture::Aim(Local,Cargo);
        // DeckTest P1 faces -X: retain the original throw line towards the host.
        else if(Stage==2) if(auto* PC=Cast<APlayerController>(Local->GetController())) PC->SetControlRotation(FRotator(0,180,0));
		if (Stage == 0 && Age > 1.f && Other)
		{
			Local->MoveIntent(FVector2D(-1, 0));
			if (StartPosition.X - Local->GetActorLocation().X > 85 && FCalmyPickupFixture::Approach(Local,Cargo))
			{
				bMoved = true;
				const uint32 Sequence = ++Local->LastIssuedSequence;
				Local->ServerGrab(Cargo, Sequence);
				Local->ServerGrab(Cargo, Sequence); // Actual duplicate over the network, not a local assertion.
				Stage = 1;
			}
		}
		else if (Stage == 1 && Local->HeldCargo == Cargo && Cargo->Carry.Holder == Local && Cargo->Carry.State == ECalmyCarryState::Held)
		{
			bHeld = true;
			StageTime = Age;
			Stage = 2;
			UE_LOG(LogTemp, Display, TEXT("CALMY_PROBE Client HELD"));
			GLog->Flush();
		}
		else if (Stage == 2)
		{
			if (StartPosition.X - Local->GetActorLocation().X < 130) Local->MoveIntent(FVector2D(-1, 0));
			else bFollowed |= FVector::Distance(Cargo->GetActorLocation(), Local->GetCarryLocation()) < 55;
			// A third real client starts during this interval to verify late-join state.
			if (Age - StageTime > 25)
			{
				Local->RequestThrow();
				Stage = 3;
			}
		}
		else if (Stage == 3 && bThrown && bRecovered && Cargo->Carry.State == ECalmyCarryState::Free && FCalmyPickupFixture::Approach(Local,Cargo))
		{
			Local->ServerGrab(Cargo, ++Local->LastIssuedSequence);
			Stage = 4;
		}
		else if (Stage == 4 && Cargo->Carry.State == ECalmyCarryState::Held && Local->HeldCargo == Cargo)
		{
			StageTime = Age;
			Stage = 5;
		}
		else if (Stage == 5 && Age - StageTime > 1.f)
		{
			Finish(bMoved && bHeld && bFollowed && bThrown && bRecovered && Local->RejectedCommands >= 1,
				TEXT("movement=1 held=1 follow=1 thrown=1 knockdown=1 recovery=1 duplicate_rejected=1 disconnect_while_held=1"));
		}
	}
	else if (Role == TEXT("Host"))
	{
		if (!Thrower.IsValid() && Stage == 0 && Other)
		{
			Thrower = Other;
			RemoteStart = Other->GetActorLocation();
		}
		if (Thrower.IsValid())
		{
			bMoved |= RemoteStart.X - Thrower->GetActorLocation().X > 60;
			if (Cargo->Carry.State == ECalmyCarryState::Held && Cargo->Carry.Holder == Thrower.Get())
			{
				bHeld = true;
				bFollowed |= bMoved && FVector::Distance(Cargo->GetActorLocation(), Thrower->GetCarryLocation()) < 55;
				if (Stage == 0)
				{
					Local->ServerGrab(Cargo, ++Local->LastIssuedSequence); // Contention must not steal.
					Stage = 1;
					StageTime = Age;
				}
			}
		}
		if (Stage == 1 && Age - StageTime > .3f && FarCargo)
		{
			Local->ServerGrab(FarCargo, ++Local->LastIssuedSequence); // More than eight metres from host.
			Stage = 2;
		}
		if (Stage == 2 && bRecovered && !Thrower.IsValid() && Cargo->Carry.State == ECalmyCarryState::Free && !Cargo->Carry.Holder)
		{
			Finish(bMoved && bHeld && bFollowed && bThrown && Local->RejectedCommands >= 2 && !Local->HeldCargo,
				TEXT("authority_movement=1 exclusive_holder=1 range_rejected=1 follow=1 impact=1 recovery=1 disconnect_release=1"));
		}
	}
}
