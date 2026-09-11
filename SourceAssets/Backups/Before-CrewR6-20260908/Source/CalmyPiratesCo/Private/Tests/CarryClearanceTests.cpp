#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyCarryClearanceTest,"CalmyPiratesCo.Gameplay.CarryCornerClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyCarryClearanceTest::RunTest(const FString&)
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* P=World->SpawnActor<ACalmyCharacter>(FVector(0,0,100),FRotator::ZeroRotator,S);
    auto* Cargo=World->SpawnActor<ACalmyCargo>(FVector(0,-150,100),FRotator::ZeroRotator,S);
    TestTrue(TEXT("Real cargo grabbed"),Cargo->TryGrab(P));
    TestTrue(TEXT("Open-space carry stays below chest camera"),P->GetCarryLocation().Equals(FVector(110,0,92),.1));
    auto* Wall=World->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);
    Box->SetBoxExtent(FVector(5,30,100));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
    Box->SetWorldLocation(FVector(100,60,100));
    const FVector Carry=P->GetCarryLocation();
    TestTrue(TEXT("Crate corner stops before offset wall; old sphere penetrated"),Carry.X<61);
    FCollisionQueryParams Q;Q.AddIgnoredActor(P);Q.AddIgnoredActor(Cargo);
    TestFalse(TEXT("Held box has no blocking overlap"),World->OverlapBlockingTestByChannel(Carry,P->GetActorQuat(),ECC_WorldStatic,FCollisionShape::MakeBox(FVector(34)),Q));
    // Rotation follows the actual rectangular cargo, not world-aligned extents.
    Box->SetWorldLocation(FVector(-60,100,100));
    Box->SetWorldRotation(FRotator(0,90,0));
    P->SetActorRotation(FRotator(0,90,0));
    const FVector Turned=P->GetCarryLocation();
    TestTrue(TEXT("Rotated corner stops before offset wall"),Turned.Y<61);
    TestFalse(TEXT("Rotated held box has no blocking overlap"),World->OverlapBlockingTestByChannel(Turned,P->GetActorQuat(),ECC_WorldStatic,FCollisionShape::MakeBox(FVector(34)),Q));
    P->SetActorRotation(FRotator::ZeroRotator);
    Cargo->Mesh->SetRelativeScale3D(FVector(1.15,.9,.75));
    Box->SetWorldRotation(FRotator::ZeroRotator);Box->SetBoxExtent(FVector(200,5,100));
    Box->SetWorldLocation(FVector(0,40,100));
    const FVector Wide=P->GetCarryLocation();
    TestTrue(TEXT("Wide crate moved away from initial wall overlap"),Wide.Y < -9);
    TestFalse(TEXT("Wide crate does not penetrate wall after correction"),World->OverlapBlockingTestByChannel(Wide,P->GetActorQuat(),ECC_WorldStatic,FCollisionShape::MakeBox(FVector(56.5,44,36.5)),Q));
    World->DestroyWorld(false);return true;
}
#endif
