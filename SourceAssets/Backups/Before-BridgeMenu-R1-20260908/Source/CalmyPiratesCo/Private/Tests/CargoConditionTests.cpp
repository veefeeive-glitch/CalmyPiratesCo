#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyShift.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyCargoConditionTest, "CalmyPiratesCo.Gameplay.CargoConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyCargoConditionTest::RunTest(const FString&)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* A = World->SpawnActor<ACalmyCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* B = World->SpawnActor<ACalmyCharacter>(FVector(0, 200, 100), FRotator::ZeroRotator, Spawn);
	auto* PC = World->SpawnActor<APlayerController>(); PC->SetAsLocalPlayerController(); PC->Possess(A);
	auto* Heavy = World->SpawnActor<ACalmyCargo>(FVector(150, 0, 100), FRotator::ZeroRotator, Spawn);
	Heavy->Kind = ECalmyCargoKind::Bulky; Heavy->DispatchBeginPlay();
	TestFalse(TEXT("Bulky cannot be lifted without active helper"), Heavy->TryGrab(A));
	auto* HelperPC = World->SpawnActor<APlayerController>(); HelperPC->Possess(B);
	TestTrue(TEXT("Nearby empty-handed player enables lift"), Heavy->TryGrab(A));
	TestEqual(TEXT("Bulky slows character movement"), A->GetCharacterMovement()->GetMaxSpeed(), 260.f);
	TestFalse(TEXT("Bulky cannot be thrown"), Heavy->Throw(A));
	B->SetActorLocation(FVector(0, 700, 100));
	Heavy->UpdateCondition(.6f);
	TestEqual(TEXT("Brief separation has grace period"), Heavy->Carry.State, ECalmyCarryState::Held);
	Heavy->UpdateCondition(.5f);
	TestEqual(TEXT("Lost helper forces safe drop"), Heavy->Carry.State, ECalmyCarryState::Free);
	TestNull(TEXT("Heavy drop clears holder"), A->HeldCargo.Get());
	TestEqual(TEXT("One forced heavy drop"), Heavy->Condition.ForcedDrops, 1);
	Heavy->UpdateCondition(2.f);
	TestEqual(TEXT("No duplicate forced drop"), Heavy->Condition.ForcedDrops, 1);
	Heavy->SetActorLocation(FVector(900, 700, 100));
	B->SetActorLocation(FVector(0, 200, 100));
	auto* Unstable = World->SpawnActor<ACalmyCargo>(FVector(150, 0, 100), FRotator::ZeroRotator, Spawn);
	Unstable->CargoId = 2; Unstable->Kind = ECalmyCargoKind::Unstable; Unstable->DispatchBeginPlay();
	TestTrue(TEXT("Stable volatile cargo can be grabbed"), Unstable->TryGrab(A));
	Unstable->UpdateCondition(2.f);
	TestEqual(TEXT("Held cargo drains stability"), Unstable->Condition.Stability, 60);
	TestFalse(TEXT("Cannot hand to self"), Unstable->TryHandoff(A, A));
	TestTrue(TEXT("Handoff to nearby empty-handed peer"), Unstable->TryHandoff(A, B));
	TestNull(TEXT("Old holder cleared atomically"), A->HeldCargo.Get());
	TestEqual(TEXT("New holder installed"), B->HeldCargo.Get(), Unstable);
	TestEqual(TEXT("Handoff stabilizes by 35"), Unstable->Condition.Stability, 95);
	TestFalse(TEXT("Former holder cannot replay transfer"), Unstable->TryHandoff(A, B));
	TestFalse(TEXT("Immediate ping-pong blocked by cargo cooldown"), Unstable->TryHandoff(B, A));
	TestEqual(TEXT("One handoff only"), Unstable->Condition.Handoffs, 1);
	Unstable->UpdateCondition(5.f);
	TestEqual(TEXT("Zero stability forces drop"), Unstable->Carry.State, ECalmyCarryState::Free);
	TestFalse(TEXT("Exhausted cargo cannot be instantly grabbed"), Unstable->TryGrab(A));
	Unstable->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Unstable->UpdateCondition(1.f);
	TestEqual(TEXT("Rest recovers stability"), Unstable->Condition.Stability, 25);
	TestTrue(TEXT("Recovered cargo can be lifted again"), Unstable->TryGrab(A));
	A->ReleaseCargo();
	Unstable->SetActorLocation(FVector(900, -700, 100));
	auto* Fragile = World->SpawnActor<ACalmyCargo>(FVector(150, 0, 100), FRotator::ZeroRotator, Spawn);
	Fragile->CargoId = 3; Fragile->Kind = ECalmyCargoKind::Fragile; Fragile->DispatchBeginPlay();
	TestFalse(TEXT("Gentle contact is harmless"), Fragile->ApplyImpactSpeed(649));
	TestTrue(TEXT("Hard impact damages fragile cargo"), Fragile->ApplyImpactSpeed(900));
	TestEqual(TEXT("Impact has measurable consequence"), Fragile->Condition.Integrity, 50);
	TestFalse(TEXT("Collision manifold cannot multiply same impact"), Fragile->ApplyImpactSpeed(900));
	Fragile->ResetHome(true);
	TestEqual(TEXT("Overboard reset does not heal damage"), Fragile->Condition.Integrity, 50);
	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(150, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DeliveryZone->SetBoxExtent(FVector(100)); Shift->DispatchBeginPlay();
	Shift->Tick(1.f);
	TestEqual(TEXT("Damaged cargo can still be delivered"), Shift->State.Delivered, 1);
	TestEqual(TEXT("Contract records damaged delivery"), Shift->State.DamagedDelivered, 1);
	Shift->Tick(1.f);
	TestEqual(TEXT("Damaged delivery counted once"), Shift->State.DamagedDelivered, 1);
	Fragile->ResetHome(false);
	TestEqual(TEXT("New shift restores integrity"), Fragile->Condition.Integrity, 100);
	TestEqual(TEXT("New shift clears damage events"), Fragile->Condition.DamageEvents, 0);
	World->DestroyWorld(false);
	return true;
}
#endif
