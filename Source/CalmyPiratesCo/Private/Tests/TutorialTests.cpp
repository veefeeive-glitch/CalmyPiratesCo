#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyCargo.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
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
    TestFalse(TEXT("Not active on unrelated test worlds"),ACalmyTutorial::Enabled(W));
    T->Step=ECalmyTutorialStep::Deliver;T->Tick(1);
    TestEqual(TEXT("No physical delivery means no departure"),T->Step,ECalmyTutorialStep::Deliver);
    W->DestroyWorld(false);return true;
}
#endif
