#include "Core/CalmyDoorMotion.h"
#include "Core/CalmyBreakdownCompass.h"
#include "Core/CalmyAudio.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyDoorMotionTest, "CalmyPiratesCo.Gameplay.HeavyDoorMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyDoorMotionTest::RunTest(const FString&)
{
	const float Dt = 1.f / 60.f;
	float Impact = 0.f;

	FCalmyDoorMotion Pull;
	Pull.Step(Dt, 180.f, true, 1.f, false, 0.f, Impact);
	TestTrue(TEXT("A full pull cannot reach top speed in one frame"), Pull.Velocity < FCalmyDoorMotion::MaxDragSpeed * .1f);
	for (int32 I = 0; I < 60; ++I) Pull.Step(Dt, 180.f, true, 1.f, false, 0.f, Impact);
	TestTrue(TEXT("Held pull reaches drag speed within a second"), FMath::IsNearlyEqual(Pull.Velocity, FCalmyDoorMotion::MaxDragSpeed, 1.f));
	TestTrue(TEXT("Pull opens the leaf"), Pull.Angle > 40.f);

	FCalmyDoorMotion Coast = Pull;
	float Before = Coast.Angle;
	for (int32 I = 0; I < 30; ++I) Coast.Step(Dt, 180.f, false, 0.f, false, 0.f, Impact);
	TestTrue(TEXT("Released leaf keeps swinging by inertia"), Coast.Angle > Before + 20.f);
	for (int32 I = 0; I < 600 && !Coast.IsSettled(); ++I) Coast.Step(Dt, 180.f, false, 0.f, false, 0.f, Impact);
	TestTrue(TEXT("Hinge friction eventually stops a free leaf"), Coast.IsSettled());

	FCalmyDoorMotion Slam; Slam.Angle = 30.f;
	FCalmyDoorMotion::EEvent Event = FCalmyDoorMotion::EEvent::None;
	for (int32 I = 0; I < 120 && Event == FCalmyDoorMotion::EEvent::None; ++I) Event = Slam.Step(Dt, 180.f, true, -1.f, false, 0.f, Impact);
	TestTrue(TEXT("Hard pull shut slams"), Event == FCalmyDoorMotion::EEvent::Slammed && Impact >= FCalmyDoorMotion::SlamSpeed);
	TestEqual(TEXT("Slammed leaf rests closed"), Slam.Angle, 0.f);

	FCalmyDoorMotion Gentle; Gentle.Angle = 90.f;
	Event = FCalmyDoorMotion::EEvent::None;
	for (int32 I = 0; I < 900 && Event == FCalmyDoorMotion::EEvent::None; ++I) Event = Gentle.Step(Dt, 180.f, false, 0.f, true, 0.f, Impact);
	TestTrue(TEXT("Tapped close eases in and clicks instead of slamming"), Event == FCalmyDoorMotion::EEvent::Latched);

	FCalmyDoorMotion Open;
	for (int32 I = 0; I < 900 && !(Open.Angle >= 180.f && Open.IsSettled()); ++I) Open.Step(Dt, 180.f, false, 0.f, true, 180.f, Impact);
	TestTrue(TEXT("Tapped open reaches the stop and settles"), Open.Angle >= 180.f && Open.IsSettled());

	FCalmyDoorMotion Knock; Knock.Angle = 175.f; Knock.Velocity = FCalmyDoorMotion::MaxDragSpeed;
	Event = Knock.Step(.1f, 180.f, false, 0.f, false, 0.f, Impact);
	TestTrue(TEXT("Fast swing knocks the open stop and rebounds a little"), Event == FCalmyDoorMotion::EEvent::HitOpenStop && Knock.Velocity < 0.f && Knock.Velocity > -20.f);

	FCalmyDoorMotion Clamp;
	for (int32 I = 0; I < 120; ++I) Clamp.Step(Dt, 90.f, true, 50.f, false, 0.f, Impact);
	TestTrue(TEXT("Client push is clamped to drag speed"), Clamp.Velocity <= FCalmyDoorMotion::MaxDragSpeed + .01f && Clamp.Angle <= 90.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyWorldAudioTest, "CalmyPiratesCo.Presentation.WorldSoundAttenuation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyWorldAudioTest::RunTest(const FString&)
{
	const FSoundAttenuationSettings Door = FCalmyAudio::MakeAttenuation(ECalmySoundCue::DoorCreak);
	TestTrue(TEXT("World cues fade with distance"), Door.bAttenuate && Door.FalloffDistance > 0.f);
	TestTrue(TEXT("World cues are positional"), Door.bSpatialize);
	TestTrue(TEXT("Bulkheads muffle world cues"), Door.bEnableOcclusion && Door.OcclusionVolumeAttenuation < 1.f);
	TestTrue(TEXT("A door is silent from the far end of the ship"), FCalmyAudio::AudibleRadiusCm(ECalmySoundCue::DoorCreak) < 6000.f);
	TestTrue(TEXT("The leak alarm carries further than a door"), FCalmyAudio::AudibleRadiusCm(ECalmySoundCue::LeakAlarm) > FCalmyAudio::AudibleRadiusCm(ECalmySoundCue::DoorCreak));
	TestTrue(TEXT("A slam carries further than a latch click"), FCalmyAudio::AudibleRadiusCm(ECalmySoundCue::DoorSlam) > FCalmyAudio::AudibleRadiusCm(ECalmySoundCue::DoorLatch));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyBreakdownCompassTest, "CalmyPiratesCo.UI.BreakdownCompass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyBreakdownCompassTest::RunTest(const FString&)
{
	const FVector Valve(100, 0, 0), Pump(0, 500, 0);
	TestFalse(TEXT("Nothing broken, no arrow"), FCalmyBreakdownCompass::Select(false, false, Valve, true, 80, Pump).IsValid());
	TestTrue(TEXT("Leak warning points at the valve"), FCalmyBreakdownCompass::Select(true, false, Valve, false, 5, Pump).Kind == ECalmyBreakdownKind::LeakWarning);
	TestTrue(TEXT("Fuel only counts during an active watch"), !FCalmyBreakdownCompass::Select(false, false, Valve, false, 5, Pump).IsValid());
	TestTrue(TEXT("Critical fuel outranks a leak warning"), FCalmyBreakdownCompass::Select(true, false, Valve, true, 8, Pump).Kind == ECalmyBreakdownKind::FuelCritical);
	const FCalmyBreakdownTarget Leak = FCalmyBreakdownCompass::Select(false, true, Valve, true, 8, Pump);
	TestTrue(TEXT("Active leak outranks everything"), Leak.Kind == ECalmyBreakdownKind::Leak && Leak.Location.Equals(Valve));
	TestTrue(TEXT("Low fuel points at the pump"), FCalmyBreakdownCompass::Select(false, false, Valve, true, 20, Pump).Location.Equals(Pump));

	const FRotator Forward(0, 0, 0);
	TestTrue(TEXT("Ahead"), FMath::IsNearlyEqual(FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(1000, 0, 0)).YawDelta, 0.f, .01f));
	TestTrue(TEXT("Right is positive"), FMath::IsNearlyEqual(FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(0, 1000, 0)).YawDelta, 90.f, .01f));
	TestTrue(TEXT("Left is negative"), FMath::IsNearlyEqual(FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(0, -1000, 0)).YawDelta, -90.f, .01f));
	TestTrue(TEXT("Behind is 180"), FMath::IsNearlyEqual(FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(-1000, 0, 0)).YawDelta, 180.f, .01f));
	TestTrue(TEXT("View yaw is subtracted"), FMath::IsNearlyEqual(FCalmyBreakdownCompass::Arrow(FRotator(0, 90, 0), FVector::ZeroVector, FVector(0, 1000, 0)).YawDelta, 0.f, .01f));
	const FCalmyCompassArrow Below = FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(800, 0, -560));
	TestTrue(TEXT("Two decks below"), Below.Elevation < 0.f && Below.DeckOffset == -2 && !Below.bArrived);
	const FCalmyCompassArrow Here = FCalmyBreakdownCompass::Arrow(Forward, FVector::ZeroVector, FVector(120, 80, 60));
	TestTrue(TEXT("Arrived on the same deck"), Here.bArrived && Here.DeckOffset == 0 && FMath::IsNearlyEqual(Here.DistanceMeters, FVector(120, 80, 60).Size() / 100.f, .01f));
	return true;
}
#endif
