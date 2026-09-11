#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Core/GameplayPolicies.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyLeakTest, "CalmyPiratesCo.Gameplay.LeakAndRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyLeakTest::RunTest(const FString&)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, Spawn);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(18, 10, .5f));
	auto* A = World->SpawnActor<ACalmyCharacter>(FVector(-100, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* B = World->SpawnActor<ACalmyCharacter>(FVector(0, 100, 100), FRotator::ZeroRotator, Spawn);
	auto* PC = World->SpawnActor<APlayerController>(); PC->SetAsLocalPlayerController(); PC->Possess(A);
	auto* PC2 = World->SpawnActor<APlayerController>(); PC2->Possess(B);
	auto* Cargo = World->SpawnActor<ACalmyCargo>(FVector(-150, -150, 100), FRotator::ZeroRotator, Spawn);
	Cargo->DispatchBeginPlay();
	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(600, 0, 100), FRotator::ZeroRotator, Spawn); Shift->DispatchBeginPlay();
	auto* Leak = World->SpawnActor<ACalmyLeak>(FVector(0, 0, 50), FRotator::ZeroRotator, Spawn); Leak->DispatchBeginPlay();
	TestFalse(TEXT("No repair before warning/active event"), Leak->TryRepairPulse(A, 1));
	const int32 FirstDelay = Leak->State.NextEventSeconds;
	TestTrue(TEXT("Director chooses a bounded initial delay"), FirstDelay >= FCalmyEmergencyDirectorPolicy::InitialDelayMinSeconds &&
		FirstDelay <= FCalmyEmergencyDirectorPolicy::InitialDelayMaxSeconds);
	Leak->Tick(FirstDelay - 1);
	TestEqual(TEXT("No immediate emergency"), Leak->State.Phase, ECalmyLeakPhase::Idle);
	Leak->Tick(1);
	TestEqual(TEXT("Warning precedes leak"), Leak->State.Phase, ECalmyLeakPhase::Warning);
	TestEqual(TEXT("Five-second warning"), Leak->State.WarningSeconds, 5);
	TestEqual(TEXT("First event has one serial"), Leak->State.EventSerial, 1);
	TestFalse(TEXT("Normal power remains during warning"), Leak->State.bPowerDegraded);
	Leak->Tick(5);
	TestEqual(TEXT("Leak becomes active"), Leak->State.Phase, ECalmyLeakPhase::Leaking);
	Leak->Tick(14);
	TestFalse(TEXT("Small leak does not immediately degrade power"), Leak->State.bPowerDegraded);
	Leak->Tick(16);
	TestEqual(TEXT("Server integrates water"), Leak->State.Water, 60);
	TestTrue(TEXT("Server derives degraded power from water"), Leak->State.bPowerDegraded);
	TestEqual(TEXT("Flooded deck slows movement"), A->GetCharacterMovement()->GetMaxSpeed(), 315.f);
	TestFalse(TEXT("Stale generation rejected"), Leak->TryRepairPulse(A, 0));
	TestFalse(TEXT("Null player rejected"), Leak->TryRepairPulse(nullptr, 1));
	A->SetActorLocation(FVector(-500, 0, 100));
	TestFalse(TEXT("Distant repair rejected"), Leak->TryRepairPulse(A, 1));
	A->SetActorLocation(FVector(-100, 0, 100));
	auto* Wall = World->SpawnActor<AStaticMeshActor>(FVector(-50, 0, 70), FRotator::ZeroRotator, Spawn);
	Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Wall->SetActorScale3D(FVector(.1f, 1, 2));
	TestFalse(TEXT("Repair cannot pass through geometry"), Leak->TryRepairPulse(A, 1));
	Wall->Destroy();
	TestTrue(TEXT("Controlled empty-handed player starts repair"), Leak->TryRepairPulse(A, 1));
	TestFalse(TEXT("Only one repairer owns station"), Leak->TryRepairPulse(B, 1));
	Leak->Tick(1);
	TestEqual(TEXT("Progress is server time, not pulse count"), Leak->State.Repair, 20);
	for (int32 I = 0; I < 30; ++I) Leak->TryRepairPulse(A, 1);
	TestEqual(TEXT("Repeated intent does not add progress"), Leak->State.Repair, 20);
	Leak->StopRepair(B);
	TestEqual(TEXT("Other pawn cannot release worker"), Leak->State.Repairer.Get(), A);
	Leak->StopRepair(A); Leak->Tick(1);
	TestNull(TEXT("Release frees station immediately"), Leak->State.Repairer.Get());
	TestEqual(TEXT("Partial repair is retained"), Leak->State.Repair, 20);
	TestTrue(TEXT("Cargo can be picked up between repair attempts"), Cargo->TryGrab(A));
	TestFalse(TEXT("Cargo work competes with repair"), Leak->TryRepairPulse(A, 1));
	A->ReleaseCargo();
	Cargo->SetActorLocation(FVector(-300, -300, 100)); // Keep the valid line-of-sight fixture clear.
	TestTrue(TEXT("Worker can resume"), Leak->TryRepairPulse(A, 1));
	A->GetCharacterMovement()->Velocity = FVector(100, 0, 0); Leak->Tick(.1f);
	TestNull(TEXT("Moving cancels repair"), Leak->State.Repairer.Get());
	A->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	TestTrue(TEXT("Resume after movement"), Leak->TryRepairPulse(A, 1));
	PC->UnPossess(); Leak->Tick(.1f);
	TestNull(TEXT("Disconnected/unpossessed worker is released"), Leak->State.Repairer.Get());
	B->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	TestFalse(TEXT("Airborne worker cannot repair"), Leak->TryRepairPulse(B, 1));
	B->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Second player can take over partial work"), Leak->TryRepairPulse(B, 1));
	Leak->Tick(4);
	TestEqual(TEXT("One player can finish repair"), Leak->State.Phase, ECalmyLeakPhase::Draining);
	TestEqual(TEXT("Repair capped at 100"), Leak->State.Repair, 100);
	TestNull(TEXT("Sealed valve has no worker"), Leak->State.Repairer.Get());
	Leak->Tick(10);
	TestEqual(TEXT("Flood is fully recoverable"), Leak->State.Phase, ECalmyLeakPhase::Resolved);
	TestFalse(TEXT("Power recovers after water drains"), Leak->State.bPowerDegraded);
	TestEqual(TEXT("Movement restored after draining"), B->GetCharacterMovement()->GetMaxSpeed(), 450.f);
	const int32 Cooldown = Leak->State.CooldownSeconds;
	TestTrue(TEXT("Director chooses a bounded recovery cooldown"), Cooldown >= FCalmyEmergencyDirectorPolicy::CooldownMinSeconds &&
		Cooldown <= FCalmyEmergencyDirectorPolicy::CooldownMaxSeconds);
	Leak->Tick(Cooldown - 1);
	TestEqual(TEXT("Cooldown prevents an early second event"), Leak->State.Phase, ECalmyLeakPhase::Resolved);
	TestEqual(TEXT("Cooldown cannot alter the active event serial"), Leak->State.EventSerial, 1);
	Leak->Tick(1);
	TestEqual(TEXT("Second event starts only after cooldown"), Leak->State.Phase, ECalmyLeakPhase::Warning);
	TestEqual(TEXT("Second event receives the next serial"), Leak->State.EventSerial, 2);
	TestEqual(TEXT("Second event resets prior repair progress"), Leak->State.Repair, 0);
	TestEqual(TEXT("Second event starts with a dry deck"), Leak->State.Water, 0);
	Leak->Tick(1000);
	TestEqual(TEXT("Active event cannot stack another event"), Leak->State.EventSerial, 2);
	TestTrue(TEXT("Final event remains repairable"), Leak->TryRepairPulse(B, 1));
	Leak->Tick(5);
	TestEqual(TEXT("Final event enters draining"), Leak->State.Phase, ECalmyLeakPhase::Draining);
	Leak->Tick(2);
	TestEqual(TEXT("Final event resolves"), Leak->State.Phase, ECalmyLeakPhase::Resolved);
	TestEqual(TEXT("Reached cap has no stale cooldown"), Leak->State.CooldownSeconds, 0);
	Leak->Tick(1000);
	TestEqual(TEXT("Reached cap remains resolved"), Leak->State.Phase, ECalmyLeakPhase::Resolved);
	Shift->State.Round = 2; Leak->Tick(.1f);
	TestEqual(TEXT("New round resets event"), Leak->State.Phase, ECalmyLeakPhase::Idle);
	TestEqual(TEXT("Generation advanced"), Leak->State.Round, 2);
	TestEqual(TEXT("Repair resets with round"), Leak->State.Repair, 0);
	TestEqual(TEXT("Director serial resets with round"), Leak->State.EventSerial, 0);
	TestTrue(TEXT("New round has a fresh bounded schedule"), Leak->State.NextEventSeconds >= FCalmyEmergencyDirectorPolicy::InitialDelayMinSeconds &&
		Leak->State.NextEventSeconds <= FCalmyEmergencyDirectorPolicy::InitialDelayMaxSeconds);
	TestFalse(TEXT("New round begins on normal power"), Leak->State.bPowerDegraded);
	TestFalse(TEXT("Old round cannot repair a new one"), Leak->TryRepairPulse(B, 1));
	Shift->State.Phase = ECalmyShiftPhase::Complete; Leak->Tick(.1f);
	TestEqual(TEXT("Contract end suspends emergency"), Leak->State.Phase, ECalmyLeakPhase::Suspended);
	TestFalse(TEXT("Contract end cannot leave degraded lighting"), Leak->State.bPowerDegraded);
	World->DestroyWorld(false);
	return true;
}
#endif
