#include "Gameplay/CalmyBridgeTerminal.h"
#include "Core/CalmySession.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMenuRows,"CalmyPiratesCo.UI.BridgeTerminalHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBridgeMenuRows::RunTest(const FString&)
{
    TestEqual(TEXT("First action"),ACalmyBridgeTerminal::RowAt(FVector2D(500,198),5),0);
    TestEqual(TEXT("Settings"),ACalmyBridgeTerminal::RowAt(FVector2D(500,250),5),1);
    TestEqual(TEXT("Gap not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,224),5),INDEX_NONE);
    TestEqual(TEXT("Outside screen"),ACalmyBridgeTerminal::RowAt(FVector2D(-1,198),5),INDEX_NONE);
    TestEqual(TEXT("Replay training"),ACalmyBridgeTerminal::RowAt(FVector2D(500,302),5),2);
    TestEqual(TEXT("Crew session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,354),5),3);
    TestEqual(TEXT("Leave session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    TestEqual(TEXT("Below leave not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,458),5),INDEX_NONE);
    TestEqual(TEXT("Settings last row on bodycam page"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmySessionHelpers,"CalmyPiratesCo.Core.SessionLanHelpers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmySessionHelpers::RunTest(const FString&)
{
    TestTrue(TEXT("admit empty"), FCalmySession::CanAdmit(0));
    TestTrue(TEXT("admit three"), FCalmySession::CanAdmit(3));
    TestFalse(TEXT("reject fourth already connected"), FCalmySession::CanAdmit(4));
    FString Clean, Error;
    TestTrue(TEXT("sanitize ip port"), FCalmySession::SanitizeJoinAddress(TEXT(" 192.168.0.12:17777 "), Clean, Error));
    TestEqual(TEXT("clean address"), Clean, FString(TEXT("192.168.0.12:17777")));
    TestFalse(TEXT("reject path"), FCalmySession::SanitizeJoinAddress(TEXT("/Game/Maps/WaterLab"), Clean, Error));
    TestFalse(TEXT("reject query"), FCalmySession::SanitizeJoinAddress(TEXT("127.0.0.1:17777?listen"), Clean, Error));
    TestEqual(TEXT("max crew"), FCalmySession::MaxCrew, 4);
    return true;
}
#endif
