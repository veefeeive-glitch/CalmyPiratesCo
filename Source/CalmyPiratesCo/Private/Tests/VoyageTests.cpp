#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyVoyageCycleTest, "CalmyPiratesCo.Gameplay.VoyageCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyVoyageCycleTest::RunTest(const FString&)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("OceanFreighterR7")), nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, Spawn);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(18, 10, .5f));

	auto* Host = World->SpawnActor<ACalmyCharacter>(FVector(-350, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* Standard = World->SpawnActor<ACalmyCargo>(FVector(-150, 0, 100), FRotator::ZeroRotator, Spawn);
	Standard->Kind = ECalmyCargoKind::Standard;
	Standard->CargoId = 1;
	auto* Bulky = World->SpawnActor<ACalmyCargo>(FVector(-150, 220, 100), FRotator::ZeroRotator, Spawn);
	Bulky->Kind = ECalmyCargoKind::Bulky;
	Bulky->CargoId = 2;
	auto* Fragile = World->SpawnActor<ACalmyCargo>(FVector(-150, 440, 100), FRotator::ZeroRotator, Spawn);
	Fragile->Kind = ECalmyCargoKind::Fragile;
	Fragile->CargoId = 3;
	Host->DispatchBeginPlay();
	Standard->DispatchBeginPlay();
	Bulky->DispatchBeginPlay();
	Fragile->DispatchBeginPlay();

	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(250, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DeliveryZone->SetBoxExtent(FVector(200, 150, 150));
	Shift->ContractDurationSeconds = 60;
	Shift->DispatchBeginPlay();
	TestEqual(TEXT("Pre-voyage lab manifest takes every crate"), Shift->State.Required, 3);

	auto* Voyage = World->SpawnActor<ACalmyVoyageDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Voyage->DispatchBeginPlay();
	TestNotNull(TEXT("Voyage director spawned"), Voyage);
	TestEqual(TEXT("Briefing phase after begin"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));
	TestEqual(TEXT("Curated manifest keeps Standard+Bulky only"), Shift->State.Required, 2);
	TestTrue(TEXT("Leak fixture armed on ship watch"), Voyage->State.bLeakArmed || ACalmyLeak::Find(World) != nullptr);
	TestTrue(TEXT("Shift timer frozen in briefing"), ACalmyVoyageDirector::ShouldFreezeShiftTimer(World));

	const int32 SecondsBefore = Shift->State.SecondsRemaining;
	Shift->Tick(5.f);
	Voyage->Tick(5.f);
	TestEqual(TEXT("Frozen briefing does not burn contract time"), Shift->State.SecondsRemaining, SecondsBefore);
	TestEqual(TEXT("Still briefing after five seconds"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));

	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetAsLocalPlayerController();
	PC->Possess(Host);
	TestTrue(TEXT("Host can skip briefing"), Voyage->TryBeginWatch(Host));
	TestEqual(TEXT("Active after begin watch"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Active));
	TestFalse(TEXT("Active watch runs the timer"), ACalmyVoyageDirector::ShouldFreezeShiftTimer(World));

	auto Place = [](ACalmyCargo* Cargo, FVector Position)
	{
		Cargo->SetActorLocationAndRotation(Position, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		Cargo->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Cargo->Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	};
	Place(Standard, FVector(250, 0, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("Standard accepted into curated watch"), Shift->State.Delivered, 1);
	Place(Fragile, FVector(250, 40, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("Off-manifest Fragile ignored"), Shift->State.Delivered, 1);
	Place(Bulky, FVector(250, 80, 50));
	Shift->Tick(1.f);
	TestEqual(TEXT("Bulky completes curated watch"), Shift->State.Delivered, 2);
	TestEqual(TEXT("Shift complete"), static_cast<uint8>(Shift->State.Phase), static_cast<uint8>(ECalmyShiftPhase::Complete));
	Voyage->Tick(.1f);
	TestEqual(TEXT("Voyage mirrors outcome"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Outcome));
	TestTrue(TEXT("Outcome marks success"), Voyage->State.bShiftComplete);

	TestTrue(TEXT("Host restart opens new briefing"), Voyage->TryRestartWatch(Host, Voyage->State.Round));
	TestEqual(TEXT("New briefing round"), Voyage->State.Round, 2);
	TestEqual(TEXT("Restart returns to briefing"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));
	TestEqual(TEXT("Restart rebuilds curated manifest"), Shift->State.Required, 2);

	World->DestroyWorld(false);
	return true;
}
#endif
