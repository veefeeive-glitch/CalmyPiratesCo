#include "Core/GameplayPolicies.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "AIController.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyCommandGateTest, "CalmyPiratesCo.Core.CommandGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyCommandGateTest::RunTest(const FString&)
{
	FCalmyCommandGate Gate;
	TestFalse(TEXT("Zero sequence rejected"), Gate.Accept(0, 0));
	TestTrue(TEXT("First command accepted"), Gate.Accept(1, 0));
	TestFalse(TEXT("Replay rejected"), Gate.Accept(1, 1));
	TestFalse(TEXT("Flood rejected"), Gate.Accept(2, .01));
	TestFalse(TEXT("Rate-rejected sequence is still consumed"), Gate.Accept(2, 1));
	TestTrue(TEXT("Fresh command after cooldown accepted"), Gate.Accept(3, 1));
	TestFalse(TEXT("Backwards time rejected"), Gate.Accept(4, .5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyActorCarryTest, "CalmyPiratesCo.Gameplay.AuthorityInvariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyActorCarryTest::RunTest(const FString&)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* A = World->SpawnActor<ACalmyCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* B = World->SpawnActor<ACalmyCharacter>(FVector(0, 150, 100), FRotator::ZeroRotator, Spawn);
	auto* Cargo = World->SpawnActor<ACalmyCargo>(FVector(150, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* Far = World->SpawnActor<ACalmyCargo>(FVector(1100, 0, 100), FRotator::ZeroRotator, Spawn);
	TestFalse(TEXT("Out-of-range grab denied"), Far->TryGrab(A));
	TestTrue(TEXT("First holder admitted"), Cargo->TryGrab(A));
	TestTrue(TEXT("Held cargo does not simulate"), !Cargo->Mesh->IsSimulatingPhysics());
	TestFalse(TEXT("Contender cannot steal"), Cargo->TryGrab(B));
	TestFalse(TEXT("Contender cannot throw"), Cargo->Throw(B));
	TestFalse(TEXT("Contender cannot drop"), Cargo->Drop(B));
	TestTrue(TEXT("Holder can throw"), Cargo->Throw(A));
	TestTrue(TEXT("Thrown cargo simulates"), Cargo->Mesh->IsSimulatingPhysics());
	TestEqual(TEXT("Thrown state"), Cargo->Carry.State, ECalmyCarryState::Thrown);
	TestTrue(TEXT("Weak impact does not knock down"), !B->ReceiveCargoImpact(499));
	TestTrue(TEXT("Qualifying server impact knocks down"), B->ReceiveCargoImpact(900));
	TestFalse(TEXT("Repeated collision does not extend knockdown"), B->ReceiveCargoImpact(900));
	TestEqual(TEXT("Movement locked during knockdown"), B->GetCharacterMovement()->GetMaxSpeed(), 0.f);
	TestFalse(TEXT("Knocked down player cannot grab"), B->CanGrab(Far));
	auto* Second = World->SpawnActor<ACalmyCargo>(FVector(-150, 0, 100), FRotator::ZeroRotator, Spawn);
	TestTrue(TEXT("Second crate can be grabbed"), Second->TryGrab(A));
	A->ReleaseCargo();
	TestEqual(TEXT("Logout release returns free"), Second->Carry.State, ECalmyCarryState::Free);
	TestNull(TEXT("Logout release clears holder"), Second->Carry.Holder.Get());
	TestNull(TEXT("Logout release clears pawn pointer"), A->HeldCargo.Get());
    auto* NPC=World->SpawnActor<ACalmyCrewHelper>(FVector(0,-150,100),FRotator::ZeroRotator,Spawn);
    auto* AI=World->SpawnActor<AAIController>();AI->Possess(NPC);NPC->AssignLeader(A);
    TestFalse(TEXT("NPC cannot assist somebody else's player"),NPC->CanAssist(B));
    NPC->AssignLeader(B);
    TestEqual(TEXT("A second player cannot steal a recruited crew member"),NPC->Leader.Get(),A);
    NPC->ReleaseLeader();NPC->UpdateAssistance(.1f);
    TestFalse(TEXT("Waiting crew persists without a leader"),NPC->IsActorBeingDestroyed());
    TestNull(TEXT("Release frees leader"),NPC->Leader.Get());
    NPC->AssignLeader(A);
    Second->Kind=ECalmyCargoKind::Unstable;
    TestTrue(TEXT("Unstable cargo can be picked up solo"),Second->TryGrab(A));
    TestFalse(TEXT("NPC cannot trap an unsupported handoff in its hands"),Second->TryHandoff(A,NPC));
    TestEqual(TEXT("Rejected NPC handoff preserves the holder"),Second->Carry.Holder.Get(),A);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyFirstPersonCharacterTest, "CalmyPiratesCo.Presentation.FirstPersonCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyFirstPersonCharacterTest::RunTest(const FString&)
{
	const ACalmyCharacter* Character = GetDefault<ACalmyCharacter>();
	const UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	TestNotNull(TEXT("First-person camera exists"), Camera);
	TestNull(TEXT("No third-person spring arm remains"), Character->FindComponentByClass<USpringArmComponent>());
	TestTrue(TEXT("Mouse view drives camera"), Camera && Camera->bUsePawnControlRotation);
	TestTrue(TEXT("Character yaw follows view"), Character->bUseControllerRotationYaw);
	TestFalse(TEXT("Movement no longer turns independently of view"), Character->GetCharacterMovement()->bOrientRotationToMovement);
	TestTrue(TEXT("Wide first-person field of view"), Camera && FMath::IsNearlyEqual(Camera->FieldOfView, 92.f));
	return true;
}
#endif
