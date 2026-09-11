#include "Gameplay/CalmyBridgeTerminal.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMenuRows,"CalmyPiratesCo.UI.BridgeTerminalHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBridgeMenuRows::RunTest(const FString&)
{
    TestEqual(TEXT("First action"),ACalmyBridgeTerminal::RowAt(FVector2D(500,198),4),0);
    TestEqual(TEXT("Settings"),ACalmyBridgeTerminal::RowAt(FVector2D(500,250),4),1);
    TestEqual(TEXT("Gap not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,224),4),INDEX_NONE);
    TestEqual(TEXT("Outside screen"),ACalmyBridgeTerminal::RowAt(FVector2D(-1,198),4),INDEX_NONE);
    TestEqual(TEXT("Replay training"),ACalmyBridgeTerminal::RowAt(FVector2D(500,302),4),2);
    TestEqual(TEXT("Leave session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,354),4),3);
    TestEqual(TEXT("Below leave not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),4),INDEX_NONE);
    TestEqual(TEXT("Settings last row on bodycam page"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    return true;
}
#endif
