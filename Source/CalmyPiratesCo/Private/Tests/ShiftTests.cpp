#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyShiftTest, "CalmyPiratesCo.Gameplay.ResetAndContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyShiftTest::RunTest(const FString&)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, Spawn);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(18, 10, .5f));
	auto* A = World->SpawnActor<ACalmyCharacter>(FVector(-350, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* B = World->SpawnActor<ACalmyCharacter>(FVector(-350, 200, 100), FRotator::ZeroRotator, Spawn);
	auto* One = World->SpawnActor<ACalmyCargo>(FVector(-150, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* Two = World->SpawnActor<ACalmyCargo>(FVector(-150, 220, 100), FRotator::ZeroRotator, Spawn);
	Two->CargoId = 2;
	A->DispatchBeginPlay(); B->DispatchBeginPlay(); One->DispatchBeginPlay(); Two->DispatchBeginPlay();
	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(250, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DeliveryZone->SetBoxExtent(FVector(200, 150, 150));
	Shift->DispatchBeginPlay();
	TestEqual(TEXT("Manifest contains exact two physical crates"), Shift->State.Required, 2);
	TestTrue(TEXT("Grab before overboard"), One->TryGrab(A));
	A->SetActorLocation(FVector(0, 1000, -280), false, nullptr, ETeleportType::TeleportPhysics);
	// Original spawn is occupied: deterministic fallback must not overlap this second player.
	B->SetActorLocation(FVector(-350, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Overboard pawn can return"), A->ReturnToDeck());
	TestTrue(TEXT("Return avoids occupied home"), FVector::Distance(A->GetActorLocation(), B->GetActorLocation()) > 70);
	TestNull(TEXT("Reset clears player's holder link"), A->HeldCargo.Get());
	TestNull(TEXT("Reset clears crate's holder link"), One->Carry.Holder.Get());
	TestEqual(TEXT("Held crate reset exactly once"), One->ResetCount, 1);
	TestEqual(TEXT("Pawn reset exactly once"), A->ResetCount, 1);
	TestEqual(TEXT("Separate rescued objects recorded"), Shift->State.Rescues, 2);
	A->Tick(.1f); One->Tick(.1f);
	TestEqual(TEXT("Safe pawn does not repeatedly reset"), A->ResetCount, 1);
	TestEqual(TEXT("Safe crate does not repeatedly reset"), One->ResetCount, 1);
	TestTrue(TEXT("Impact before reset"), A->ReceiveCargoImpact(900));
	TestTrue(TEXT("Knocked pawn can safely return"), A->ReturnToDeck());
	TestTrue(TEXT("Reset advances recovery to controlled"), A->IsControlled());
	TestEqual(TEXT("Recovery counted once"), A->RecoveryCount, 1);

	auto Place = [](ACalmyCargo* Cargo, FVector Position)
	{
		Cargo->SetActorLocationAndRotation(Position, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		Cargo->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Cargo->Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	};
	Place(One, FVector(250, 0, 50));
	TestFalse(TEXT("Immediate delivery rejected before settling"), Shift->TryAccept(One));
	Shift->Tick(.5f);
	TestEqual(TEXT("Insufficient settle time"), Shift->State.Delivered, 0);
	Shift->Tick(.3f);
	TestEqual(TEXT("First stable cargo accepted"), Shift->State.Delivered, 1);
	TestTrue(TEXT("Accepted cargo is locked"), One->Carry.bDelivered);
	TestFalse(TEXT("Accepted cargo cannot be re-grabbed"), One->TryGrab(A));
	TestFalse(TEXT("Duplicate acceptance rejected"), Shift->TryAccept(One));
	One->ResetHome(false);
	// A rescue/admin reset of an accepted object must not create a second contract credit.
	Place(One, FVector(250, 0, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("Reset cannot double count accepted cargo"), Shift->State.Delivered, 1);
	Place(Two, FVector(435, 100, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("Pivot inside but bounds outside is rejected"), Shift->State.Delivered, 1);
	Place(Two, FVector(250, 100, 50));
	Two->Mesh->SetPhysicsLinearVelocity(FVector(200, 0, 0));
	Shift->Tick(1.f);
	TestEqual(TEXT("Moving cargo cannot be accepted"), Shift->State.Delivered, 1);
	Place(Two, FVector(250, 100, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("All unique crates complete contract"), Shift->State.Phase, ECalmyShiftPhase::Complete);
	TestTrue(TEXT("Successful contract awards server payout"), Shift->State.FinalPayout > 0);
	TestTrue(TEXT("Successful contract awards a rating"), Shift->State.PerformanceStars >= 1 && Shift->State.PerformanceStars <= 3);
	TestFalse(TEXT("No unauthenticated restart"), Shift->TryRestart(nullptr, 1));
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetAsLocalPlayerController(); // Transient test world has no GameInstance to create a local player.
	PC->Possess(A);
	TestFalse(TEXT("Stale generation cannot restart"), Shift->TryRestart(A, 0));
    auto* Helper=World->SpawnActor<ACalmyCrewHelper>(FVector(600,0,100),FRotator::ZeroRotator,Spawn);
    Helper->AssignLeader(A);A->CrewHelper=Helper;
	TestTrue(TEXT("Local host restarts completed round"), Shift->TryRestart(A, 1));
    TestNull(TEXT("New shift clears the old helper link"),A->CrewHelper.Get());
    TestFalse(TEXT("New shift preserves the standing crew actor"),Helper->IsActorBeingDestroyed());
    TestNull(TEXT("New shift releases the crew assignment"),Helper->Leader.Get());
	TestEqual(TEXT("Exactly one new round"), Shift->State.Round, 2);
	TestEqual(TEXT("Fresh contract is empty"), Shift->State.Delivered, 0);
	TestFalse(TEXT("Replay cannot restart again"), Shift->TryRestart(A, 1));
	TestFalse(TEXT("New round clears delivered flag"), Two->Carry.bDelivered);
	TestEqual(TEXT("New round clears final payout"), Shift->State.FinalPayout, 0);
	TestEqual(TEXT("New round restores contract clock"), Shift->State.SecondsRemaining, Shift->ContractDurationSeconds);
	Shift->ContractDurationSeconds = 30;
	Shift->InitializeManifest();
	Shift->Tick(31.f);
	TestEqual(TEXT("Expired server clock fails contract"), Shift->State.Phase, ECalmyShiftPhase::Failed);
	TestEqual(TEXT("Failed contract pays nothing"), Shift->State.FinalPayout, 0);
	TestTrue(TEXT("Local host can retry failed contract"), Shift->TryRestart(A, 2));
	TestEqual(TEXT("Retry advances round exactly once"), Shift->State.Round, 3);
	Two->CargoId = 1;
	Shift->InitializeManifest();
	TestEqual(TEXT("Duplicate manifest ids fail closed"), Shift->State.Phase, ECalmyShiftPhase::InvalidManifest);
	World->DestroyWorld(false);
	return true;
}
#endif
