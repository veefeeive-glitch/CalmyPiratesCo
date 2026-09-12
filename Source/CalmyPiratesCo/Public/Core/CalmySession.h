#pragma once
#include "CoreMinimal.h"
class APlayerController;
class UWorld;
/** LAN listen/join helpers (G4/A04/A11). No Steam/OnlineSubsystem integration. */
class CALMYPIRATESCO_API FCalmySession
{
public:
	static constexpr int32 MaxCrew = 4;
	static constexpr int32 DefaultPort = 17777;
	static bool CanAdmit(int32 ConnectedPlayersIncludingPending);
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
