#include "Core/CalmySession.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/ConfigCacheIni.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

namespace CalmySessionPrivate
{
	static const TCHAR* Section = TEXT("Calmy.Session");
	static const TCHAR* JoinKey = TEXT("LastJoin");
}

bool FCalmySession::CanAdmit(int32 ConnectedPlayersIncludingPending)
{
	return ConnectedPlayersIncludingPending >= 0 && ConnectedPlayersIncludingPending < MaxCrew;
}

int32 FCalmySession::ClampPort(int32 Port)
{
	return FMath::Clamp(Port, 1024, 65535);
}

bool FCalmySession::SanitizeJoinAddress(const FString& Raw, FString& OutAddress, FString& OutError)
{
	OutAddress.Reset();
	OutError.Reset();
	FString Trimmed = Raw.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		OutError = TEXT("Enter host IP and optional port, e.g. 192.168.1.10:17777");
		return false;
	}
	if (Trimmed.Contains(TEXT(" ")) || Trimmed.Contains(TEXT("\t")) || Trimmed.Contains(TEXT("\"")) ||
		Trimmed.Contains(TEXT("'")) || Trimmed.Contains(TEXT("\\")) || Trimmed.Contains(TEXT("/")) ||
		Trimmed.Contains(TEXT("?")) || Trimmed.Contains(TEXT("&")) || Trimmed.Contains(TEXT("=")))
	{
		OutError = TEXT("Address may only contain host and optional :port");
		return false;
	}
	FString Host = Trimmed;
	FString PortText;
	int32 Port = DefaultPort;
	if (Trimmed.Split(TEXT(":"), &Host, &PortText, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		if (!PortText.IsNumeric())
		{
			OutError = TEXT("Port must be a number from 1024 to 65535");
			return false;
		}
		const int32 Parsed = FCString::Atoi(*PortText);
		Port = ClampPort(Parsed);
		if (Parsed != Port)
		{
			OutError = TEXT("Port must be a number from 1024 to 65535");
			return false;
		}
	}
	Host = Host.TrimStartAndEnd();
	if (Host.IsEmpty() || Host.Len() > 64)
	{
		OutError = TEXT("Host name or IP is missing or too long");
		return false;
	}
	for (TCHAR C : Host)
	{
		if (!(FChar::IsAlnum(C) || C == TEXT('.') || C == TEXT('-')))
		{
			OutError = TEXT("Host may only use letters, digits, dots and hyphens");
			return false;
		}
	}
	OutAddress = FString::Printf(TEXT("%s:%d"), *Host, Port);
	return true;
}

FString FCalmySession::LoadSavedJoinAddress()
{
	FString Saved;
	GConfig->GetString(CalmySessionPrivate::Section, CalmySessionPrivate::JoinKey, Saved, GGameUserSettingsIni);
	FString Clean, Error;
	if (SanitizeJoinAddress(Saved, Clean, Error)) return Clean;
	return FString::Printf(TEXT("127.0.0.1:%d"), DefaultPort);
}

void FCalmySession::SaveJoinAddress(const FString& Address)
{
	FString Clean, Error;
	if (!SanitizeJoinAddress(Address, Clean, Error)) return;
	GConfig->SetString(CalmySessionPrivate::Section, CalmySessionPrivate::JoinKey, *Clean, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

bool FCalmySession::HostLanListen(UWorld* World, int32 Port, FString* OutError)
{
	if (!World)
	{
		if (OutError) *OutError = TEXT("No world");
		return false;
	}
	if (World->GetNetMode() == NM_ListenServer) return true;
	if (World->GetNetMode() != NM_Standalone)
	{
		if (OutError) *OutError = TEXT("Only a solo session can open LAN listen");
		return false;
	}
	FURL Url = World->URL;
	Url.Port = ClampPort(Port > 0 ? Port : DefaultPort);
	if (!World->Listen(Url))
	{
		if (OutError) *OutError = TEXT("Listen failed - port may be occupied");
		UE_LOG(LogTemp, Error, TEXT("CALMY_SESSION host_listen_failed port=%d"), Url.Port);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION HOST_LAN port=%d endpoint=%s"), Url.Port, *DescribeListenEndpoint(World));
	return true;
}

bool FCalmySession::JoinLan(APlayerController* Controller, const FString& Address, FString* OutError)
{
	if (!Controller || !Controller->IsLocalController())
	{
		if (OutError) *OutError = TEXT("Local controller required");
		return false;
	}
	FString Clean, Error;
	if (!SanitizeJoinAddress(Address, Clean, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	SaveJoinAddress(Clean);
	UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION JOIN_LAN address=%s"), *Clean);
	Controller->ClientTravel(Clean, TRAVEL_Absolute);
	return true;
}

FString FCalmySession::DescribeListenEndpoint(const UWorld* World)
{
	const int32 Port = ClampPort(World && World->URL.Port > 0 ? World->URL.Port : DefaultPort);
	FString Chosen = TEXT("127.0.0.1");
	if (ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		bool bCanBindAll = false;
		TSharedRef<FInternetAddr> Local = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
		if (Local->IsValid())
		{
			const FString Candidate = Local->ToString(false);
			if (!Candidate.IsEmpty() && Candidate != TEXT("0.0.0.0")) Chosen = Candidate;
		}
	}
	return FString::Printf(TEXT("%s:%d"), *Chosen, Port);
}

int32 FCalmySession::CountCrew(const UWorld* World)
{
	if (!World) return 0;
	int32 Controllers = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->IsValid()) ++Controllers;
	}
	int32 FromState = 0;
	if (const AGameStateBase* GS = World->GetGameState())
	{
		FromState = GS->PlayerArray.Num();
	}
	return FMath::Max(Controllers, FromState);
}

FString FCalmySession::RoleLabel(const UWorld* World)
{
	if (!World) return TEXT("UNKNOWN");
	switch (World->GetNetMode())
	{
	case NM_Client: return TEXT("CLIENT");
	case NM_ListenServer: return TEXT("HOST");
	case NM_DedicatedServer: return TEXT("SERVER");
	default: return TEXT("SOLO");
	}
}

FString FCalmySession::StatusLine(const UWorld* World)
{
	const int32 Crew = CountCrew(World);
	const FString Role = RoleLabel(World);
	if (!World) return TEXT("SESSION OFFLINE");
	if (World->GetNetMode() == NM_ListenServer)
	{
		return FString::Printf(TEXT("%s | CREW %d/%d | LAN %s"), *Role, Crew, MaxCrew, *DescribeListenEndpoint(World));
	}
	if (World->GetNetMode() == NM_Client)
	{
		return FString::Printf(TEXT("%s | CREW %d/%d | JOINED LAN"), *Role, Crew, MaxCrew);
	}
	return FString::Printf(TEXT("%s | CREW %d/%d | LOCAL ONLY"), *Role, Crew, MaxCrew);
}
