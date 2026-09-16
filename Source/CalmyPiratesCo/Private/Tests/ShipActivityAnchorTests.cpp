#include "Gameplay/CalmyVoyageDirector.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS

// The R7 watch tasks used to sit wherever the code happened to put them: the bilge valve ended up on the crest of the
// hold coaming with no floor to work from, the fuel pump inside an engine-room casing. The playability survey
// (Scripts/run_ship_playability_ue.ps1) proved the reviewed spots; this test pins them so a later edit cannot move a
// task off the deck unnoticed. Volumes below are the compartments themselves, in ship-local rest-pose cm.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyShipActivityAnchorTest, "CalmyPiratesCo.Ship.ActivityAnchors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyShipActivityAnchorTest::RunTest(const FString&)
{
	const FVector Valve = ACalmyVoyageDirector::LeakValveLocal();
	const FVector Pool = ACalmyVoyageDirector::LeakPoolLocal();
	const FVector Fuel = ACalmyVoyageDirector::FuelStationLocal();
	const FVector Delivery = ACalmyVoyageDirector::DeliveryLocal();

	// Hold 1 runs X -20..10 m between the transverse bulkheads, tank top at Z 2.08 m, hold width |Y| <= 7.1 m.
	TestTrue(TEXT("Valve on the hold 1 aft bulkhead"), Valve.X > -2000.f && Valve.X < -1900.f);
	TestTrue(TEXT("Valve inside the hold width"), FMath::Abs(Valve.Y) < 710.f);
	TestTrue(TEXT("Valve at working height above the tank top"), Valve.Z > 208.f + 60.f && Valve.Z < 208.f + 180.f);
	TestTrue(TEXT("Flood water pools on the hold 1 tank top"), Pool.Z > 190.f && Pool.Z < 240.f);
	TestTrue(TEXT("Flood water pools inside hold 1"), Pool.X > -1990.f && Pool.X < 1000.f && FMath::Abs(Pool.Y) < 710.f);
	TestTrue(TEXT("Water spreads away from the valve"), (Pool - Valve).Size2D() > 500.f);

	// Weather deck is Z 10.58 m; the side lane runs at |Y| 7.9..8.7 m and the S3 bunker station at X -19.3..-16.1 m.
	TestTrue(TEXT("Fuel station on the weather deck"), Fuel.Z > 1042.f && Fuel.Z < 1260.f);
	TestTrue(TEXT("Fuel station at the hold 1 coaming"), Fuel.X > -1950.f && Fuel.X < -1600.f);
	TestTrue(TEXT("Fuel station reachable from the side lane"), FMath::Abs(Fuel.Y) > 700.f && FMath::Abs(Fuel.Y) < 900.f);
	TestTrue(TEXT("Delivery zone on the weather deck"), Delivery.Z > 1042.f && Delivery.Z < 1260.f);
	TestTrue(TEXT("Delivery zone on a side lane"), FMath::Abs(Delivery.Y) > 700.f && FMath::Abs(Delivery.Y) < 950.f);
	// Tasks must stay apart: the watch is a walk between them, not one spot doing everything.
	TestTrue(TEXT("Valve is far from the delivery zone"), (Delivery - Valve).Size() > 1500.f);
	TestTrue(TEXT("Fuel station is far from the delivery zone"), (Delivery - Fuel).Size2D() > 1000.f);
	return true;
}
#endif
