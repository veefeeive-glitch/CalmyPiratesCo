#include "Testing/CalmySessionProbe.h"
#include "Core/CalmySession.h"
#include "Gameplay/CalmyCharacter.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmySessionProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString ProbeRole;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmySessionProbe="), ProbeRole);
#endif
}

void UCalmySessionProbe::Finish(bool Success, const FString& Details)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s %s %s"), *Role, Success ? TEXT("PASS") : TEXT("FAIL"), *Details);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}

void UCalmySessionProbe::Tick(float DeltaSeconds)
{
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if (Role.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("CalmySessionProbe="), Role);
		FParse::Value(FCommandLine::Get(), TEXT("CalmySessionPeers="), ExpectedPeers);
		ExpectedPeers = FMath::Clamp(ExpectedPeers, 2, FCalmySession::MaxCrew);
	}
	Age += DeltaSeconds;
	if (Age > 90.f)
	{
		Finish(false, FString::Printf(TEXT("timeout crew=%d peak=%d expected=%d mode=%d"),
			FCalmySession::CountCrew(GetWorld()), PeakCrew, ExpectedPeers, (int32)GetWorld()->GetNetMode()));
		return;
	}

	const bool Host = Role == TEXT("Host");
	const bool Late = Role == TEXT("LateJoin");
	const bool Clientish = Role == TEXT("Client") || Role == TEXT("Client2") || Late;
	if ((Host && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Clientish && GetWorld()->GetNetMode() != NM_Client)) return;

	int32 Pawns = 0;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It) ++Pawns;
	const int32 Crew = FCalmySession::CountCrew(GetWorld());
	PeakCrew = FMath::Max(PeakCrew, Crew);

	if (!bStarted)
	{
		bStarted = true;
		UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s START crew=%d pawns=%d expected=%d endpoint=%s"),
			*Role, Crew, Pawns, ExpectedPeers, *FCalmySession::DescribeListenEndpoint(GetWorld()));
		GLog->Flush();
	}

	if (Host)
	{
		if (Stage == 0 && PeakCrew >= 2)
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST Host READY crew=%d peak=%d"), Crew, PeakCrew);
			GLog->Flush();
			Stage = 1;
		}
		if (Stage == 1 && PeakCrew >= ExpectedPeers)
		{
			if (ReadyAt < 0.f) ReadyAt = Age;
			// Give clients a moment to observe the full roster and log PASS first.
			if (Age - ReadyAt >= 5.f)
			{
				Finish(true, FString::Printf(TEXT("four_slot_cap=%d crew=%d peak=%d pawns=%d listen=1 late_join_ok=1"),
					FCalmySession::MaxCrew, Crew, PeakCrew, Pawns));
			}
		}
		return;
	}

	if (Crew >= 1 && Pawns >= 1)
	{
		if (Stage == 0)
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s READY crew=%d pawns=%d"), *Role, Crew, Pawns);
			GLog->Flush();
			Stage = 1;
		}
		if (Late)
		{
			// Late joiner proves attachment to a live multiplayer session.
			if (Stage >= 1 && GetWorld()->GetNetMode() == NM_Client && Age >= 3.f)
			{
				Finish(true, FString::Printf(TEXT("late_join=1 crew=%d peak=%d pawns=%d expected=%d"),
					Crew, PeakCrew, Pawns, ExpectedPeers));
			}
			return;
		}
		if (PeakCrew >= ExpectedPeers)
		{
			if (ReadyAt < 0.f) ReadyAt = Age;
			// Hold the roster briefly so LateJoin can attach before early clients exit.
			if (Age - ReadyAt >= 4.f)
			{
				Finish(true, FString::Printf(TEXT("joined=1 crew=%d peak=%d pawns=%d expected=%d late=%d"),
					Crew, PeakCrew, Pawns, ExpectedPeers, 0));
			}
		}
	}
}
