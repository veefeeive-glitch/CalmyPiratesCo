#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyPreferences.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyTutorialTest,"CalmyPiratesCo.Gameplay.TutorialProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyTutorialTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    auto* T=W->SpawnActor<ACalmyTutorial>();
    TestTrue(TEXT("Snapshot replicates"),T->GetIsReplicated());
    TestTrue(TEXT("Available to late join regardless of distance"),T->bAlwaysRelevant);
    TestEqual(TEXT("Starts at walking lesson"),T->Step,ECalmyTutorialStep::Walk);
    T->Tick(1);TestEqual(TEXT("Time alone cannot pass lesson"),T->Step,ECalmyTutorialStep::Walk);
    T->Tick(-1);TestEqual(TEXT("Invalid delta cannot progress"),T->WalkedCm,0);
    TestTrue(TEXT("Allowlisted ship package exists"),FPackageName::DoesPackageExist(ACalmyTutorial::Destination()));
    TestTrue(TEXT("Allowlisted training package exists"),FPackageName::DoesPackageExist(ACalmyTutorial::TrainingMap()));
    TestFalse(TEXT("Not active on unrelated test worlds"),ACalmyTutorial::Enabled(W));
    T->Step=ECalmyTutorialStep::Deliver;T->Tick(1);
    TestEqual(TEXT("No physical delivery means no departure"),T->Step,ECalmyTutorialStep::Deliver);
    TestTrue(TEXT("Lesson freezes the contract clock"),ACalmyShift::FreezesContractTimer(true,false));
    TestTrue(TEXT("Lesson still accepts the training crate"),ACalmyShift::AcceptsDeliveries(true,false));
    TestFalse(TEXT("Voyage briefing blocks deliveries"),ACalmyShift::AcceptsDeliveries(false,true));
    TestFalse(TEXT("Live contract clock runs"),ACalmyShift::FreezesContractTimer(false,false));
    W->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyTutorialProgressTest,"CalmyPiratesCo.Gameplay.TutorialProgressPersist",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyTutorialProgressTest::RunTest(const FString&)
{
	auto* Pref = UCalmyPreferences::Get();
	const bool Previous = Pref->bTutorialCompleted;
	Pref->bTutorialCompleted = false;
	Pref->Persist();
	ACalmyTutorial::MarkCompletedLocally();
	TestTrue(TEXT("Completion sticks in memory"), Pref->bTutorialCompleted);
	bool Disk = false;
	TestTrue(TEXT("Completion written to progress ini"),
		GConfig->GetBool(TEXT("Calmy.Progress"), TEXT("TutorialCompleted"), Disk, GGameUserSettingsIni) && Disk);
	ACalmyTutorial::BeginForcedReplay();
	TestTrue(TEXT("Menu replay forces another lesson"), ACalmyTutorial::IsForcedReplay());
	ACalmyTutorial::MarkCompletedLocally();
	TestFalse(TEXT("Finishing clears forced replay"), ACalmyTutorial::IsForcedReplay());
	TestTrue(TEXT("Replay finish keeps completion saved"), Pref->bTutorialCompleted);
	Pref->bTutorialCompleted = Previous;
	Pref->Persist();
	return true;
}
#endif
