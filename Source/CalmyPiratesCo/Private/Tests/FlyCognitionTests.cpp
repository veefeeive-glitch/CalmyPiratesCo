#include "Gameplay/FlyCognition.h"
#include "Gameplay/FlySwarmMob.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyLearningTest, "CalmyPiratesCo.FlyMind.ExperienceChangesNeuralInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyLearningTest::RunTest(const FString&)
{
    FFlyCognition Calm, Startled;
    FFlyObservation O; O.Key = TEXT("visitor"); O.Proximity = .8f;
    for (int32 I = 0; I < 300; ++I)
    {
        O.Startle = 0; Calm.Step(.2f, {O}, 0);
        O.Startle = 1; Startled.Step(.2f, {O}, 0);
    }
    TestTrue(TEXT("Experience changes future gain for same stimulus"), Startled.SensoryMultiplier(O.Key) > Calm.SensoryMultiplier(O.Key) + .5f);
    TestTrue(TEXT("Person-specific memory"), Calm.SensoryMultiplier(TEXT("stranger")) > Calm.SensoryMultiplier(O.Key));
    FFlyLIFState A, B; A.Initialize(88); B.Initialize(88);
    for (int32 I = 0; I < 1000; ++I)
    {
        A.Step(FFlyConnectome::Get(), .5f * Calm.SensoryMultiplier(O.Key), .2f, .3f, false);
        B.Step(FFlyConnectome::Get(), .5f * Startled.SensoryMultiplier(O.Key), .2f, .3f, false);
    }
    TestTrue(TEXT("Learned input changes actual sensory population firing"), FMath::Abs(A.Rates[1] - B.Rates[1]) > 1.f);
    TestTrue(TEXT("Recall after negative event"), Calm.Converse(O.Key, true, EFlyUtterance::Threaten).Contains(TEXT("remember")));
    TestEqual(TEXT("Immediate danger overrides activity"), Calm.Activity, EFlyActivity::Withdraw);
    TestTrue(TEXT("Memory answer grounded in actual event"), Calm.Converse(O.Key, true, EFlyUtterance::AskMemory).Contains(TEXT("Threats remembered: 1")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyAgencyTest, "CalmyPiratesCo.FlyMind.AutonomousNeedsAndAttention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyAgencyTest::RunTest(const FString&)
{
    FFlyCognition M; bool Rest = false, Explore = false;
    for (int32 I = 0; I < 3000; ++I)
    {
        M.Step(.2f, {}, 0.f); Rest |= M.Activity == EFlyActivity::Rest; Explore |= M.Activity == EFlyActivity::Explore;
        if (!FMath::IsWithinInclusive(M.Energy, 0.f, 1.f)) { AddError(TEXT("Invalid energy")); break; }
    }
    TestTrue(TEXT("Without commands, needs select rest and exploration"), Rest && Explore);
    M.Energy = 1; M.Curiosity = 0; M.SocialNeed = 1; M.Alarm = 0; M.ActivitySeconds = 4;
    FFlyObservation O; O.Key = TEXT("nearby"); O.Proximity = .3f; O.Position = FVector(123,0,0);
    M.Step(.2f, {O}, 0);
    TestEqual(TEXT("Social need selects contact"), M.Activity, EFlyActivity::Socialize);
    TestEqual(TEXT("Attention points at observed visitor"), M.AttentionPosition.X, 123.);
    M.Energy = 1; M.Curiosity = 0; M.SocialNeed = 0; M.Alarm = .2f; M.ActivitySeconds = 4;
    M.Step(.2f, {O}, 0);
    TestEqual(TEXT("Calm visitor selects observe over explore"), M.Activity, EFlyActivity::Observe);
    M.Step(.2f, {}, 0);
    TestTrue(TEXT("Disconnected/absent player no longer in attention"), M.AttentionKey.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyInscriptionTest, "CalmyPiratesCo.FlyMind.SharedInscriptionsGather", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyInscriptionTest::RunTest(const FString&)
{
    FFlyCognition M;
    const FVector Spot(100, 0, 40);
    TestTrue(TEXT("First mark kept"), M.Inscribe(TEXT("host"), true, EFlyMessageKind::BeWary, Spot).Contains(TEXT("BE WARY")));
    TestEqual(TEXT("One inscription"), M.InscriptionCount(), 1);
    TestTrue(TEXT("Nearby same kind echoes"), M.Inscribe(TEXT("guest"), true, EFlyMessageKind::BeWary, Spot + FVector(50,0,0)).Contains(TEXT("grows stronger")));
    TestEqual(TEXT("Echo merges"), M.InscriptionCount(), 1);
    TestEqual(TEXT("Echo count"), M.Inscriptions[0].Echoes, 1);
    M.Inscribe(TEXT("host"), true, EFlyMessageKind::SeekContact, Spot + FVector(500,0,0));
    TestEqual(TEXT("Different kind separate"), M.InscriptionCount(), 2);
    TestTrue(TEXT("Caution from wary marks"), M.InscriptionCaution > .1f);
    TestTrue(TEXT("Welcome from contact marks"), M.InscriptionWelcome > .1f);
    TestTrue(TEXT("AskMemory reports marks"), M.Converse(TEXT("host"), true, EFlyUtterance::AskMemory).Contains(TEXT("Marks left by visitors: 2")));
    TestTrue(TEXT("Travel thought while planning"), M.DescribeTravelThought(true, false, false).Contains(TEXT("thinking")));
    TestTrue(TEXT("Travel thought after path"), M.DescribeTravelThought(false, true, false).Contains(TEXT("path")));
    auto* Save = Cast<UFlyCognitionSave>(UGameplayStatics::CreateSaveGameObject(UFlyCognitionSave::StaticClass()));
    Save->Version = 2; Save->Mind = M; Save->Mind.Sanitize(true);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Save inscriptions"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UFlyCognitionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Reload save"), Loaded)) return false;
    Loaded->Mind.Sanitize(true);
    TestEqual(TEXT("Inscriptions survive reload"), Loaded->Mind.InscriptionCount(), 2);
    TestEqual(TEXT("Echoes survive reload"), Loaded->Mind.InscriptionEchoTotal(), 1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyInscriptionNeuralTest, "CalmyPiratesCo.FlyMind.InscriptionsDriveFlyWire", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyInscriptionNeuralTest::RunTest(const FString&)
{
    FFlyCognition Calm, Wary, Warm;
    for (int32 I = 0; I < 5; ++I)
        Wary.Inscribe(TEXT("a"), true, EFlyMessageKind::BeWary, FVector(I * 500.f, 0, 0));
    for (int32 I = 0; I < 4; ++I)
        Warm.Inscribe(TEXT("a"), true, I % 2 ? EFlyMessageKind::PraiseColony : EFlyMessageKind::SeekContact, FVector(I * 500.f, 0, 0));
    TestTrue(TEXT("Wary marks raise loom gain"), Wary.InscriptionSensoryGain() > Calm.InscriptionSensoryGain() + .2f);
    TestTrue(TEXT("Welcome marks lower loom gain"), Warm.InscriptionSensoryGain() < Calm.InscriptionSensoryGain());
    TestTrue(TEXT("Wary marks drive air pathway"), Wary.InscriptionAirPuff() > .25f);
    TestTrue(TEXT("Welcome marks drive ascending"), Warm.InscriptionAscendDrive() > .25f);
    FFlyLIFState Neutral, Vigilant;
    Neutral.Initialize(11); Vigilant.Initialize(11);
    const FFlyConnectome& Graph = FFlyConnectome::Get();
    for (int32 I = 0; I < 800; ++I)
    {
        Neutral.Step(Graph, .35f, .35f, .3f * Calm.InscriptionSensoryGain(), false, Calm.InscriptionAirPuff(), Calm.InscriptionAscendDrive());
        Vigilant.Step(Graph, .35f, .35f, .3f * Wary.InscriptionSensoryGain(), false, Wary.InscriptionAirPuff(), Wary.InscriptionAscendDrive());
    }
    TestTrue(TEXT("Shared wary marks change looming population rate"), FMath::Abs(Neutral.Rates[1] - Vigilant.Rates[1]) > .5f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyMemoryTest, "CalmyPiratesCo.FlyMind.PersistentMemoryRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyMemoryTest::RunTest(const FString&)
{
    auto* Save = Cast<UFlyCognitionSave>(UGameplayStatics::CreateSaveGameObject(UFlyCognitionSave::StaticClass()));
    Save->Mind.Converse(TEXT("persistent-host"), true, EFlyUtterance::Threaten);
    Save->Mind.Converse(TEXT("guest-session"), false, EFlyUtterance::Greet);
    Save->Mind.Sanitize(true);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Real UE SaveGame serialization"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UFlyCognitionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Reloaded save type"), Loaded)) return false;
    Loaded->Mind.Sanitize(true);
    const auto* Person = Loaded->Mind.Find(TEXT("persistent-host"));
    if (TestNotNull(TEXT("Known player survives reload"), Person)) TestEqual(TEXT("Threat experience survives"), Person->Threats, 1);
    TestNull(TEXT("Session guest cannot become false persistent identity"), Loaded->Mind.Find(TEXT("guest-session")));
    TestEqual(TEXT("Episode survives reload"), Loaded->Mind.Episodes.Num(), 1);
    for (int32 I = 0; I < 100; ++I) Loaded->Mind.Converse(FString::FromInt(I), false, EFlyUtterance::Greet);
    TestEqual(TEXT("Bounded people"), Loaded->Mind.People.Num(), 64);
    TestEqual(TEXT("Bounded episodes"), Loaded->Mind.Episodes.Num(), 32);
    return true;
}
#endif
