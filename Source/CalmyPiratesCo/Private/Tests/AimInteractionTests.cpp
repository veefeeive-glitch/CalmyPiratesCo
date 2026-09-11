#include "Core/AimInteraction.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyAimMathTest,"CalmyPiratesCo.Interaction.AimMath",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyAimMathTest::RunTest(const FString&)
{
    float D=0;const FBox Box(FVector(-50),FVector(50));
    TestTrue(TEXT("Scaled rotated box hit"),FCalmyAimInteraction::HitBox(FVector::ZeroVector,FVector::ForwardVector,
        FTransform(FRotator(0,90,0),FVector(200,0,0),FVector(.7,1.2,.8)),Box,220,D));
    TestEqual(TEXT("World-space surface distance not centre distance"),D,140.f,.001f);
    TestFalse(TEXT("Behind is not aimed"),FCalmyAimInteraction::HitBox(FVector::ZeroVector,-FVector::ForwardVector,FTransform(FVector(200,0,0)),Box,500,D));
    TestFalse(TEXT("Short segment rejects far surface"),FCalmyAimInteraction::HitBox(FVector::ZeroVector,FVector::ForwardVector,FTransform(FVector(300,0,0)),Box,220,D));
    TestFalse(TEXT("Side object is not selected"),FCalmyAimInteraction::HitBox(FVector::ZeroVector,FVector::ForwardVector,FTransform(FVector(200,100,0)),Box,500,D));
    TestEqual(TEXT("Sensitivity lower bound"),FCalmyAimInteraction::Sensitivity(-100),.25f);
    TestEqual(TEXT("Sensitivity upper bound"),FCalmyAimInteraction::Sensitivity(100),2.5f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyAimActorTest,"CalmyPiratesCo.Interaction.AimedCargoAndRefusals",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyAimActorTest::RunTest(const FString&)
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* P=W->SpawnActor<ACalmyCharacter>(FVector(0,0,100),FRotator::ZeroRotator,S);
    auto* PC=W->SpawnActor<APlayerController>();PC->Possess(P);
    auto* Cargo=W->SpawnActor<ACalmyCargo>(FVector(180,0,100),FRotator::ZeroRotator,S);
    auto* Side=W->SpawnActor<ACalmyCargo>(FVector(0,110,100),FRotator::ZeroRotator,S);
    PC->SetControlRotation((Cargo->GetActorLocation()-P->GetInteractionOrigin()).Rotation());
    TestEqual(TEXT("Aim selects forward crate, not closer side crate"),P->FindGrabTarget(),Cargo);
    TestEqual(TEXT("Near aimed crate ready"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Ready);
    TestEqual(TEXT("Forged side target denied"),P->GetGrabFeedback(Side),ECalmyGrabFeedback::NoTarget);
    Cargo->SetActorLocation(FVector(400,0,100));PC->SetControlRotation((Cargo->GetActorLocation()-P->GetInteractionOrigin()).Rotation());
    TestEqual(TEXT("Far object yields explicit approach hint"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::TooFar);
    TestFalse(TEXT("Server-side direct pickup also respects reach"),Cargo->TryGrab(P));
    Cargo->SetActorLocation(FVector(180,0,100));PC->SetControlRotation((Cargo->GetActorLocation()-P->GetInteractionOrigin()).Rotation());
    Cargo->Kind=ECalmyCargoKind::Bulky;
    TestEqual(TEXT("Heavy needs helper"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::NeedHelper);
    Cargo->Kind=ECalmyCargoKind::Unstable;Cargo->Condition.Stability=10;
    TestEqual(TEXT("Exhausted explains resting"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Unstable);
    Cargo->Kind=ECalmyCargoKind::Standard;Cargo->Carry.bDelivered=true;
    TestEqual(TEXT("Delivered refuses re-grab"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Delivered);
    Cargo->Carry.bDelivered=false;Cargo->Carry.State=ECalmyCarryState::Held;
    TestEqual(TEXT("Occupied still targetable for refusal"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Occupied);
    Cargo->Carry.State=ECalmyCarryState::Free;
    auto* Wall=W->SpawnActor<AStaticMeshActor>(FVector(95,0,100),FRotator::ZeroRotator,S);
    Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Wall->SetActorScale3D(FVector(.1,1,3));Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    TestEqual(TEXT("Thin wall blocks aimed cargo"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Obstructed);
    Wall->Destroy();
    P->Lifecycle=ECalmyPlayerLifecycleState::Recovering;
    TestEqual(TEXT("Recovery feedback"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Recovering);
    P->Lifecycle=ECalmyPlayerLifecycleState::Controlled;
    P->GetCharacterMovement()->SetMovementMode(MOVE_Custom,1);
    TestEqual(TEXT("Swimming feedback"),P->GetGrabFeedback(Cargo),ECalmyGrabFeedback::Swimming);
    W->DestroyWorld(false);return true;
}
#endif
