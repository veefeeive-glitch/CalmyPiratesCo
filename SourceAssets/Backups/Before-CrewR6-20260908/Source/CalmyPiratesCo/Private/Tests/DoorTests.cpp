#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyDoorTest,"CalmyPiratesCo.Gameplay.Doors",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyDoorTest::RunTest(const FString&)
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* D=W->SpawnActor<ACalmyDoor>(FVector::ZeroVector,FRotator::ZeroRotator,S);
    D->bStartOpen=false;D->RebuildGeometry();D->State=ECalmyDoorState::Closed;
    auto* P=W->SpawnActor<ACalmyCharacter>(FVector(-56,-150,90),FRotator(0,90,0),S);
    auto* PC=W->SpawnActor<APlayerController>();PC->Possess(P);PC->SetControlRotation(FRotator(0,90,0));
    TestEqual(TEXT("Aimed closed leaf is interactable"),ACalmyDoor::FindTarget(P),D);
    TestFalse(TEXT("Stale revision denied"),D->RequestToggle(P,1));
    TestTrue(TEXT("Near player opens door"),D->RequestToggle(P,0));
    TestFalse(TEXT("Duplicate/moving toggle denied"),D->RequestToggle(P,0));
    for(int32 I=0;I<30;++I)D->Tick(.1f);
    TestTrue(TEXT("Door reaches fully open"),D->IsOpen());
    FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(P);
    TestFalse(TEXT("Open doorway clears a chest-height ray"),W->LineTraceSingleByChannel(Hit,FVector(-56,-100,125),FVector(-56,100,125),ECC_Visibility,Q));
    P->SetActorLocation(FVector(56,-150,90));
    TestTrue(TEXT("Open leaf remains interactable"),D->RequestToggle(P,D->Revision));
    // Real collision body in the closing arc, not a mocked overlap result.
    auto* Blocker=W->SpawnActor<AActor>(FVector(-10,-55,105),FRotator::ZeroRotator,S);
    auto* Box=NewObject<UBoxComponent>(Blocker);Blocker->SetRootComponent(Box);Box->SetBoxExtent(FVector(20,20,100));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Blocker->SetActorLocation(FVector(-10,-55,105));
    for(int32 I=0;I<30;++I)D->Tick(.1f);
    TestEqual(TEXT("Obstacle stops swing"),D->State,ECalmyDoorState::Blocked);
    TestTrue(TEXT("Stopped before closing on obstacle"),D->Angle>1.f);
    Blocker->Destroy();
    P->SetActorLocation(FVector(1000,1000,90));
    TestFalse(TEXT("Out-of-range toggle rejected"),D->RequestToggle(P,D->Revision));
    W->DestroyWorld(false);return true;
}
#endif
