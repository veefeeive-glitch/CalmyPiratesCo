#include "Core/CrewRecovery.h"
#include "Components/BoxComponent.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyRecoveryTest,"CalmyPiratesCo.Character.PhysicalRecoveryClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyRecoveryTest::RunTest(const FString&)
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    auto* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    auto MakeBox=[&](FVector P,FVector Extent)
    {
        auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);A->AddInstanceComponent(B);
        B->SetBoxExtent(Extent);B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();B->SetWorldLocation(P);return B;
    };
    FVector Stand;
    TestFalse(TEXT("No stand-up in mid-air"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),34,88,Stand));
    auto* Floor=MakeBox(FVector(0,0,-10),FVector(500,500,10));
    TestTrue(TEXT("Clear supported position"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),34,88,Stand));
    TestTrue(TEXT("No lateral relocation; full capsule above floor"),Stand.Equals(FVector(0,0,91),.01));
    auto* Ceiling=MakeBox(FVector(0,0,130),FVector(150,150,10));
    TestFalse(TEXT("Low ceiling defers recovery"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),34,88,Stand));
    Ceiling->SetWorldLocation(FVector(0,0,300));
    auto* Obstacle=MakeBox(FVector(30,0,90),FVector(10,80,85));
    TestFalse(TEXT("Side obstacle blocks the standing capsule"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),34,88,Stand));
    Obstacle->SetWorldLocation(FVector(250,0,90));
    TestTrue(TEXT("Recovery allowed once obstacle leaves"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),34,88,Stand));
    TestFalse(TEXT("Floor too far below body is not support"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,240),34,88,Stand));
    TestFalse(TEXT("Invalid dimensions rejected"),FCalmyCrewRecovery::FindStandingCapsule(W,nullptr,FVector(0,0,20),88,34,Stand));
    W->DestroyWorld(false);return true;
}
#endif
