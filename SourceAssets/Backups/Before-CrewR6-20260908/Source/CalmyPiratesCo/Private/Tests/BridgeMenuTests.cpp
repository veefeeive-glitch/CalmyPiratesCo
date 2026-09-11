#include "Gameplay/CalmyBridgeTerminal.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMenuRows,"CalmyPiratesCo.UI.BridgeTerminalHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBridgeMenuRows::RunTest(const FString&)
{
    TestEqual(TEXT("First action"),ACalmyBridgeTerminal::RowAt(FVector2D(500,198),3),0);
    TestEqual(TEXT("Settings"),ACalmyBridgeTerminal::RowAt(FVector2D(500,250),3),1);
    TestEqual(TEXT("Gap not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,224),3),INDEX_NONE);
    TestEqual(TEXT("Outside screen"),ACalmyBridgeTerminal::RowAt(FVector2D(-1,198),3),INDEX_NONE);
    TestEqual(TEXT("Hidden row not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,354),3),INDEX_NONE);
    TestEqual(TEXT("Settings last row"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    return true;
}
#endif
