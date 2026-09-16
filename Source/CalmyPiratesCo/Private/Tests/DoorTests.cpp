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

    // Held drag: exactly one holder, and the heavy leaf follows the push gradually.
    auto* Other=W->SpawnActor<ACalmyCharacter>(FVector(-56,-150,90),FRotator(0,90,0),S);
    auto* OtherPC=W->SpawnActor<APlayerController>();OtherPC->Possess(Other);OtherPC->SetControlRotation(FRotator(0,90,0));
    TestTrue(TEXT("Aimed player grabs the leaf"),D->TryGrab(P));
    TestFalse(TEXT("A second player cannot take a held leaf"),D->TryGrab(Other));
    TestFalse(TEXT("Only the holder pushes"),D->ApplyDrag(Other,1.f));
    for(int32 I=0;I<5;++I){TestTrue(TEXT("Holder pushes"),D->ApplyDrag(P,1.f));D->Tick(.1f);}
    TestTrue(TEXT("Push opens the heavy leaf"),D->Angle>5.f);
    TestTrue(TEXT("Half a second of pushing cannot fling it open"),D->Angle<FCalmyDoorMotion::MaxDragSpeed*.5f+1.f);
    const float AfterPush=D->Angle;
    for(int32 I=0;I<4;++I)D->Tick(.1f);
    TestTrue(TEXT("Expired push lease lets the leaf coast on"),D->Angle>AfterPush);
    D->ReleaseGrab(Other);
    TestTrue(TEXT("Another player cannot release the holder"),D->Holder==P);
    D->ReleaseGrab(P);
    TestTrue(TEXT("Holder releases"),D->Holder==nullptr);
    for(int32 I=0;I<40;++I)D->Tick(.1f);
    D->SnapTo(0);
    Other->Destroy();

    TestFalse(TEXT("Stale revision denied"),D->RequestToggle(P,1));
    TestTrue(TEXT("Near player opens door"),D->RequestToggle(P,D->Revision));
    TestFalse(TEXT("Duplicate toggle denied"),D->RequestToggle(P,D->Revision-1));
    for(int32 I=0;I<60;++I)D->Tick(.1f);
    TestTrue(TEXT("Tapped heavy door swings fully open on its own"),D->IsOpen());
    FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(P);
    TestFalse(TEXT("Open doorway clears a chest-height ray"),W->LineTraceSingleByChannel(Hit,FVector(-56,-100,125),FVector(-56,100,125),ECC_Visibility,Q));
    P->SetActorLocation(FVector(56,-150,90));
    TestTrue(TEXT("Open leaf remains interactable"),D->RequestToggle(P,D->Revision));
    // Real collision body in the closing arc, not a mocked overlap result.
    auto* Blocker=W->SpawnActor<AActor>(FVector(-10,-55,105),FRotator::ZeroRotator,S);
    auto* Box=NewObject<UBoxComponent>(Blocker);Blocker->SetRootComponent(Box);Box->SetBoxExtent(FVector(20,20,100));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Blocker->SetActorLocation(FVector(-10,-55,105));
    for(int32 I=0;I<60;++I)D->Tick(.1f);
    TestEqual(TEXT("Obstacle stops swing"),D->State,ECalmyDoorState::Blocked);
    TestTrue(TEXT("Stopped before closing on obstacle"),D->Angle>1.f);
    Blocker->Destroy();
    P->SetActorLocation(FVector(1000,1000,90));
    TestFalse(TEXT("Out-of-range toggle rejected"),D->RequestToggle(P,D->Revision));
    TestFalse(TEXT("Out-of-range grab rejected"),D->TryGrab(P));
    W->DestroyWorld(false);return true;
}
#endif
