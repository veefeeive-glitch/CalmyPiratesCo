#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyFuelStation.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyVoyageContract.h"
#include "Gameplay/CalmyMissionBoard.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Core/CalmySession.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyVoyageCycleTest, "CalmyPiratesCo.Gameplay.VoyageCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyVoyageCycleTest::RunTest(const FString&)
{
	FCalmyVoyageContractStore::ClearAccepted();
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
	auto* Unstable = World->SpawnActor<ACalmyCargo>(FVector(-150, 660, 100), FRotator::ZeroRotator, Spawn);
	Unstable->Kind = ECalmyCargoKind::Unstable;
	Unstable->CargoId = 4;
	Host->DispatchBeginPlay();
	Standard->DispatchBeginPlay();
	Bulky->DispatchBeginPlay();
	Fragile->DispatchBeginPlay();
	Unstable->DispatchBeginPlay();

	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(250, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DeliveryZone->SetBoxExtent(FVector(320, 280, 180));
	Shift->ContractDurationSeconds = 60;
	Shift->DispatchBeginPlay();
	TestEqual(TEXT("Pre-voyage lab manifest takes every crate"), Shift->State.Required, 4);

	auto* Voyage = World->SpawnActor<ACalmyVoyageDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Voyage->DispatchBeginPlay();
	TestNotNull(TEXT("Voyage director spawned"), Voyage);
	TestEqual(TEXT("Briefing phase after begin"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));
	TestEqual(TEXT("Curated manifest keeps all four kinds"), Shift->State.Required, 4);
	TestEqual(TEXT("Default G3b watch duration applied"), Shift->ContractDurationSeconds, 1080);
	TestTrue(TEXT("Leak fixture armed on ship watch"), Voyage->State.bLeakArmed || ACalmyLeak::Find(World) != nullptr);
	TestTrue(TEXT("Fuel station armed on ship watch"), Voyage->State.bFuelStationArmed || ACalmyFuelStation::Find(World) != nullptr);
	TestTrue(TEXT("Shift timer frozen in briefing"), ACalmyVoyageDirector::ShouldFreezeShiftTimer(World));
	TestEqual(TEXT("Fuel starts full"), Voyage->State.FuelPercent, 100);

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

	Voyage->Tick(200.f);
	TestTrue(TEXT("Storm window raises sea state"), Voyage->State.StormPercent > 0);
	if (auto* Ocean = ACalmyOcean::Find(World))
	{
		TestTrue(TEXT("Ocean storm intensity follows voyage"), Ocean->StormIntensity > 0.f);
	}
	const int32 FuelBeforePulse = Voyage->State.FuelPercent;
	TestTrue(TEXT("Fuel drains during the watch"), FuelBeforePulse < 100);
	Voyage->ApplyFuelPulse(25.f);
	TestTrue(TEXT("Fuel pulse raises meter"), Voyage->State.FuelPercent > FuelBeforePulse);

	auto Place = [&](ACalmyCargo* Cargo, FVector Position)
	{
		Cargo->SetActorLocationAndRotation(Position, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		Cargo->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Cargo->Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		for (int32 I = 0; I < 3; ++I) Shift->Tick(1.f);
	};
	Place(Standard, FVector(250, 0, 50));
	TestEqual(TEXT("Standard accepted into curated watch"), Shift->State.Delivered, 1);
	Place(Fragile, FVector(230, 40, 50));
	TestEqual(TEXT("Fragile accepted"), Shift->State.Delivered, 2);
	Place(Unstable, FVector(270, -40, 50));
	TestEqual(TEXT("Unstable accepted"), Shift->State.Delivered, 3);
	Place(Bulky, FVector(250, 20, 50));
	TestEqual(TEXT("Bulky completes curated watch"), Shift->State.Delivered, 4);
	TestEqual(TEXT("Shift complete"), static_cast<uint8>(Shift->State.Phase), static_cast<uint8>(ECalmyShiftPhase::Complete));
	Voyage->Tick(.1f);
	TestEqual(TEXT("Voyage mirrors outcome"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Outcome));
	TestTrue(TEXT("Outcome marks success"), Voyage->State.bShiftComplete);

	TestTrue(TEXT("Host restart opens new briefing"), Voyage->TryRestartWatch(Host, Voyage->State.Round));
	TestEqual(TEXT("New briefing round"), Voyage->State.Round, 2);
	TestEqual(TEXT("Restart returns to briefing"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));
	TestEqual(TEXT("Restart rebuilds curated manifest"), Shift->State.Required, 4);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyVoyageLoopPortTest, "CalmyPiratesCo.Gameplay.VoyageLoopPort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyVoyageLoopPortTest::RunTest(const FString&)
{
	FCalmyVoyageContractStore::ClearAccepted();
	const FCalmyVoyageContract Short = FCalmyVoyageContractStore::ShortWatch();
	FCalmyVoyageContractStore::SaveAccepted(Short);
	TestTrue(TEXT("contract accepted persisted"), FCalmyVoyageContractStore::HasAccepted());
	TestEqual(TEXT("short contract id"), FCalmyVoyageContractStore::LoadAccepted().ContractId, Short.ContractId);
	GConfig->SetInt(TEXT("Calmy.VoyageContract"), TEXT("RequiredStandard"), 0, GGameUserSettingsIni);
	GConfig->SetInt(TEXT("Calmy.VoyageContract"), TEXT("WatchDurationSeconds"), 5, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Calmy.VoyageContract"), TEXT("Fuel"), !Short.bFuel, GGameUserSettingsIni);
	const FCalmyVoyageContract Reloaded = FCalmyVoyageContractStore::LoadAccepted();
	TestEqual(TEXT("edited ini cannot change cargo terms"), Reloaded.RequiredStandard, Short.RequiredStandard);
	TestEqual(TEXT("edited ini cannot change watch duration"), Reloaded.WatchDurationSeconds, Short.WatchDurationSeconds);
	TestEqual(TEXT("edited ini cannot strip hazards"), Reloaded.bFuel, Short.bFuel);
	FCalmyVoyageContractStore::SaveAccepted(Short);

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* Port = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("PortHub")), nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Host = Port->SpawnActor<ACalmyCharacter>(FVector(200, 0, 100), FRotator::ZeroRotator, Spawn);
	Host->DispatchBeginPlay();
	auto* Board = Port->SpawnActor<ACalmyMissionBoard>(FVector(200, 0, 40), FRotator::ZeroRotator, Spawn);
	Board->DispatchBeginPlay();
	TestTrue(TEXT("port map recognized"), ACalmyMissionBoard::IsPortMap(Port));
	TestNotNull(TEXT("mission board present"), ACalmyMissionBoard::Find(Port));
	auto* PC = Port->SpawnActor<APlayerController>();
	PC->SetAsLocalPlayerController();
	PC->Possess(Host);
	Host->SetActorLocation(Board->GetActorLocation());
	TestTrue(TEXT("board step cycles"), Board->TryStep(Host, 1));
	TestEqual(TEXT("selected heavy after step from default 1"), Board->SelectedIndex, 2);

	UWorld* Ship = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("OceanFreighterR7")), nullptr, true, ERHIFeatureLevel::Num, &Init);
	auto* Floor = Ship->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, Spawn);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(18, 10, .5f));
	auto* Standard = Ship->SpawnActor<ACalmyCargo>(FVector(-150, 0, 100), FRotator::ZeroRotator, Spawn);
	Standard->Kind = ECalmyCargoKind::Standard;
	Standard->CargoId = 1;
	Standard->DispatchBeginPlay();
	auto* Shift = Ship->SpawnActor<ACalmyShift>(FVector(250, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DeliveryZone->SetBoxExtent(FVector(320, 280, 180));
	Shift->DispatchBeginPlay();
	auto* Voyage = Ship->SpawnActor<ACalmyVoyageDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Voyage->DispatchBeginPlay();
	TestEqual(TEXT("ship briefing uses accepted short contract"), Voyage->State.ContractId, Short.ContractId);
	TestEqual(TEXT("short watch duration"), Voyage->WatchDurationSeconds, Short.WatchDurationSeconds);
	TestEqual(TEXT("short curated needs only standard"), Shift->State.Required, 1);
	TestEqual(TEXT("briefing names contract"), Voyage->State.ContractName, Short.DisplayName);
	TestEqual(TEXT("snapshot replicates watch length"), Voyage->State.WatchSeconds, Short.WatchDurationSeconds);
	TestFalse(TEXT("port hand-off consumed on the ship"), FCalmyVoyageContractStore::HasAccepted());
	TestNull(TEXT("leak-free contract spawns no valve"), ACalmyLeak::Find(Ship));

	auto* ShipHost = Ship->SpawnActor<ACalmyCharacter>(FVector(-350, 0, 100), FRotator::ZeroRotator, Spawn);
	ShipHost->DispatchBeginPlay();
	auto* ShipPC = Ship->SpawnActor<APlayerController>();
	ShipPC->SetAsLocalPlayerController();
	ShipPC->Possess(ShipHost);
	TestTrue(TEXT("begin watch"), Voyage->TryBeginWatch(ShipHost));
	Voyage->Tick(200.f);
	TestEqual(TEXT("storm-free contract never storms"), Voyage->State.StormPercent, 0);
	Standard->SetActorLocationAndRotation(FVector(250, 0, 50), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	Standard->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Standard->Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	for (int32 I = 0; I < 3; ++I) Shift->Tick(1.f);
	Voyage->Tick(.1f);
	TestEqual(TEXT("short watch outcomes"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Outcome));
	TestTrue(TEXT("return allowlisted"), FCalmySession::IsAllowlistedVoyageTravel(TEXT("OceanFreighterR7"), FCalmySession::PortHubMap()));

	FCalmyVoyageContractStore::ClearAccepted();
	Port->DestroyWorld(false);
	Ship->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyVoyageFuelStarvationTest, "CalmyPiratesCo.Gameplay.VoyageFuelStarvation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalmyVoyageFuelStarvationTest::RunTest(const FString&)
{
	FCalmyVoyageContractStore::SaveAccepted(FCalmyVoyageContractStore::ShortWatch());
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("OceanFreighterR7")), nullptr, true, ERHIFeatureLevel::Num, &Init);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, Spawn);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(18, 10, .5f));
	auto* Host = World->SpawnActor<ACalmyCharacter>(FVector(-350, 0, 100), FRotator::ZeroRotator, Spawn);
	Host->DispatchBeginPlay();
	auto* Standard = World->SpawnActor<ACalmyCargo>(FVector(-150, 0, 100), FRotator::ZeroRotator, Spawn);
	Standard->Kind = ECalmyCargoKind::Standard;
	Standard->CargoId = 1;
	Standard->DispatchBeginPlay();
	auto* Shift = World->SpawnActor<ACalmyShift>(FVector(250, 0, 100), FRotator::ZeroRotator, Spawn);
	Shift->DispatchBeginPlay();
	auto* Voyage = World->SpawnActor<ACalmyVoyageDirector>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Voyage->DispatchBeginPlay();
	TestNotNull(TEXT("fuel pump spawned for fuel contract"), ACalmyFuelStation::Find(World));
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetAsLocalPlayerController();
	PC->Possess(Host);
	TestTrue(TEXT("begin watch"), Voyage->TryBeginWatch(Host));
	const int32 Round = Voyage->State.Round;
	Voyage->Tick(600.f);
	TestEqual(TEXT("fuel starvation ends the watch"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Outcome));
	TestTrue(TEXT("starvation flagged"), Voyage->State.bFuelStarved && Voyage->State.bShiftFailed);
	TestEqual(TEXT("shift is still loading after starvation"), static_cast<uint8>(Shift->State.Phase), static_cast<uint8>(ECalmyShiftPhase::Loading));
	TestEqual(TEXT("storm cleared on outcome"), Voyage->State.StormPercent, 0);
	TestTrue(TEXT("host can restart after fuel starvation"), Voyage->TryRestartWatch(Host, Round));
	TestEqual(TEXT("restart opens a new briefing"), static_cast<uint8>(Voyage->State.Phase), static_cast<uint8>(ECalmyVoyagePhase::Briefing));
	TestEqual(TEXT("restart advances the round"), Voyage->State.Round, Round + 1);
	TestEqual(TEXT("restart refills fuel"), Voyage->State.FuelPercent, 100);
	FCalmyVoyageContractStore::ClearAccepted();
	World->DestroyWorld(false);
	return true;
}
#endif
