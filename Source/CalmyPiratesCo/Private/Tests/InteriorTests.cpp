#include "Gameplay/CalmyInteriorCutaway.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyInteriorCutawayTest, "CalmyPiratesCo.Presentation.InteriorCutaway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalmyInteriorCutawayTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Crew inside hides roof locally"), ACalmyInteriorCutaway::ShouldHideRoof(FVector(0, 0, 90)));
	TestTrue(TEXT("Entrance threshold hides roof before camera crosses wall"), ACalmyInteriorCutaway::ShouldHideRoof(FVector(490, 0, 90)));
	TestFalse(TEXT("Crew on open deck keeps exterior roof"), ACalmyInteriorCutaway::ShouldHideRoof(FVector(700, 0, 90)));
	TestFalse(TEXT("Crew beyond side bulkhead keeps exterior roof"), ACalmyInteriorCutaway::ShouldHideRoof(FVector(0, 800, 90)));
	TestTrue(TEXT("Lower engine room hides roof"), ACalmyInteriorCutaway::ShouldHideRoof(FVector(0, 0, -180)));
	TestTrue(TEXT("Lower deck hides weather deck"), ACalmyInteriorCutaway::ShouldHideMainDeck(FVector(1200, 0, -180)));
	TestFalse(TEXT("Weather deck stays visible topside"), ACalmyInteriorCutaway::ShouldHideMainDeck(FVector(1200, 0, 90)));
	return true;
}
#endif
