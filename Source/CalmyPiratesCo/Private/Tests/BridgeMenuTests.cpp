#include "Gameplay/CalmyBridgeTerminal.h"
#include "Core/CalmySession.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMenuRows,"CalmyPiratesCo.UI.BridgeTerminalHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBridgeMenuRows::RunTest(const FString&)
{
    TestEqual(TEXT("First action"),ACalmyBridgeTerminal::RowAt(FVector2D(500,133),5),0);
    TestEqual(TEXT("Settings"),ACalmyBridgeTerminal::RowAt(FVector2D(500,183),5),1);
    TestEqual(TEXT("Gap not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,158),5),INDEX_NONE);
    TestEqual(TEXT("Outside screen"),ACalmyBridgeTerminal::RowAt(FVector2D(-1,133),5),INDEX_NONE);
    TestEqual(TEXT("Replay training"),ACalmyBridgeTerminal::RowAt(FVector2D(500,233),5),2);
    TestEqual(TEXT("Crew session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,283),5),3);
    TestEqual(TEXT("Leave session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,333),5),4);
    TestEqual(TEXT("Below leave not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,383),5),INDEX_NONE);
    TestEqual(TEXT("Settings last row on bodycam page"),ACalmyBridgeTerminal::RowAt(FVector2D(500,333),5),4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmySessionHelpers,"CalmyPiratesCo.Core.SessionLanHelpers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmySessionHelpers::RunTest(const FString&)
{
    TestTrue(TEXT("admit empty"), FCalmySession::CanAdmit(0));
    TestTrue(TEXT("admit three"), FCalmySession::CanAdmit(3));
    TestFalse(TEXT("reject fourth already connected"), FCalmySession::CanAdmit(4));
    TestEqual(TEXT("host + two loaded + joiner"), FCalmySession::SeatsTakenAtPreLogin(1, 3, 3), 3);
    TestFalse(TEXT("second simultaneous joiner sees the first still loading"), FCalmySession::CanAdmit(FCalmySession::SeatsTakenAtPreLogin(1, 4, 3)));
    TestTrue(TEXT("fourth crew member while one is loading"), FCalmySession::CanAdmit(FCalmySession::SeatsTakenAtPreLogin(1, 3, 2)));
    TestEqual(TEXT("never below logged-in players"), FCalmySession::SeatsTakenAtPreLogin(0, 0, 4), 4);
    TestTrue(TEXT("solo may replay training"), FCalmySession::CanRequestSessionTravel(NM_Standalone, true));
    TestTrue(TEXT("listen host may replay training"), FCalmySession::CanRequestSessionTravel(NM_ListenServer, true));
    TestFalse(TEXT("remote controller on host cannot travel session"), FCalmySession::CanRequestSessionTravel(NM_ListenServer, false));
    TestFalse(TEXT("client cannot travel session"), FCalmySession::CanRequestSessionTravel(NM_Client, true));
    TestFalse(TEXT("dedicated server has no local host"), FCalmySession::CanRequestSessionTravel(NM_DedicatedServer, false));
    TestTrue(TEXT("port to ship allowlisted"), FCalmySession::IsAllowlistedVoyageTravel(TEXT("PortHub"), TEXT("/Game/Maps/OceanFreighterR7")));
    TestTrue(TEXT("ship to port allowlisted"), FCalmySession::IsAllowlistedVoyageTravel(TEXT("OceanFreighterR7"), TEXT("/Game/Maps/PortHub")));
    TestFalse(TEXT("port to waterlab blocked"), FCalmySession::IsAllowlistedVoyageTravel(TEXT("PortHub"), TEXT("/Game/Maps/WaterLab")));
    TestFalse(TEXT("ship to waterlab blocked"), FCalmySession::IsAllowlistedVoyageTravel(TEXT("OceanFreighterR7"), TEXT("/Game/Maps/WaterLab")));
    FString Clean, Error;
    TestTrue(TEXT("sanitize ip port"), FCalmySession::SanitizeJoinAddress(TEXT(" 192.168.0.12:17777 "), Clean, Error));
    TestEqual(TEXT("clean address"), Clean, FString(TEXT("192.168.0.12:17777")));
    TestFalse(TEXT("reject path"), FCalmySession::SanitizeJoinAddress(TEXT("/Game/Maps/WaterLab"), Clean, Error));
    TestFalse(TEXT("reject query"), FCalmySession::SanitizeJoinAddress(TEXT("127.0.0.1:17777?listen"), Clean, Error));
    TestEqual(TEXT("max crew"), FCalmySession::MaxCrew, 4);
    return true;
}
#endif
