#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
class APlayerController;
class UWorld;
/** LAN listen/join helpers (G4/A04/A11). No Steam/OnlineSubsystem integration. */
class CALMYPIRATESCO_API FCalmySession
{
public:
	static constexpr int32 MaxCrew = 4;
	static constexpr int32 DefaultPort = 17777;
	static bool CanAdmit(int32 ConnectedPlayersIncludingPending);
	/** Seats held while a new connection is in PreLogin: local host players plus every other open client connection
	 *  (loading clients have no PlayerController yet), never less than GetNumPlayers(). The joiner is already listed. */
	static int32 SeatsTakenAtPreLogin(int32 LocalPlayers, int32 OpenClientConnectionsIncludingJoiner, int32 NumPlayers);
	/** Whole-session map travel (replay training / voyage loop) is decided by the local host/solo controller only. */
	static bool CanRequestSessionTravel(ENetMode NetMode, bool bLocalController);
	/** PortHub ↔ OceanFreighterR7 only (plus existing training maps via their own callers). */
	static bool IsAllowlistedVoyageTravel(const FString& FromMap, const FString& ToPackagePath);
	static const TCHAR* PortHubMap() { return TEXT("/Game/Maps/PortHub"); }
	static const TCHAR* ShipMap() { return TEXT("/Game/Maps/OceanFreighterR7"); }
	static bool SanitizeJoinAddress(const FString& Raw, FString& OutAddress, FString& OutError);
	static int32 ClampPort(int32 Port);
	static FString LoadSavedJoinAddress();
	static void SaveJoinAddress(const FString& Address);
	static bool HostLanListen(UWorld* World, int32 Port = DefaultPort, FString* OutError = nullptr);
	static bool JoinLan(APlayerController* Controller, const FString& Address, FString* OutError = nullptr);
	static FString DescribeListenEndpoint(const UWorld* World);
	static int32 CountCrew(const UWorld* World);
	static FString RoleLabel(const UWorld* World);
	static FString StatusLine(const UWorld* World);
};
