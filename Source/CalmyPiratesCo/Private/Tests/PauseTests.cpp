#include "Gameplay/CalmyGameMode.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyPauseControllerTest, "CalmyPiratesCo.UI.PauseControllerDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyPauseControllerTest::RunTest(const FString&)
{
    const auto* Mode=GetDefault<ACalmyGameMode>();
    TestEqual(TEXT("All native game modes use the pause-enabled controller"),Mode->PlayerControllerClass.Get(),ACalmyPlayerController::StaticClass());
    const auto* PC=GetDefault<ACalmyPlayerController>();
    TestFalse(TEXT("Menu closed by default"),PC->IsPauseMenuOpen());
    TestTrue(TEXT("Controller can process resume while paused"),PC->PrimaryActorTick.bTickEvenWhenPaused);
    TestFalse(TEXT("No visible cursor during gameplay"),PC->bShowMouseCursor);
    return true;
}
#endif
