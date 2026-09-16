#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyHullProxy.h"
#include "Gameplay/FlySwarmMob.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyOceanStormScaleTest,"CalmyPiratesCo.Water.StormScale",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyOceanStormScaleTest::RunTest(const FString&)
{
	TestTrue(TEXT("Quiet SeaStrength"),FMath::IsNearlyEqual(ACalmyOcean::SeaStrength(0),.65f));
	TestTrue(TEXT("Packet SeaStrength"),FMath::IsNearlyEqual(ACalmyOcean::SeaStrength(40),2.65f,1.e-5f));
	TestEqual(TEXT("Calm storm scale"),ACalmyOcean::StormScale(0.f,1.85f),1.f);
	TestTrue(TEXT("Full storm scale"),FMath::IsNearlyEqual(ACalmyOcean::StormScale(1.f,1.85f),1.85f));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyShipMotionLimitsTest,"CalmyPiratesCo.Ship.MotionLimitsAndHelm",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyShipMotionLimitsTest::RunTest(const FString&)
{
	const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACalmyOcean* Ocean=World->SpawnActor<ACalmyOcean>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
	ACalmyShipMotion* Ship=World->SpawnActor<ACalmyShipMotion>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
	if(!Ocean||!Ship){World->DestroyWorld(false);return false;}
	Ship->SetStormBoost(0.f); float Heave=0,Heel=0,Drift=0; Ship->GetEffectiveMotionLimits(Heave,Heel,Drift);
	TestTrue(TEXT("Boost0 heave finite"),Heave+0.01f>=Ship->MaxHeaveCm);
	Ship->SetStormBoost(1.f); Ship->GetEffectiveMotionLimits(Heave,Heel,Drift);
	TestTrue(TEXT("Boost1 raises heave"),Heave+0.01f>=Ship->MaxHeaveCm*Ship->StormHeaveGain);
	Ship->SetHelmInput(4.f,-3.f);
	TestTrue(TEXT("Throttle"),FMath::IsNearlyEqual(Ship->GetHelmThrottle(),1.f));
	TestTrue(TEXT("Rudder"),FMath::IsNearlyEqual(Ship->GetHelmRudder(),-1.f));
	TestNotNull(TEXT("Boarding"),Ship->BoardingRamp.Get());
	World->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyShipRiderDeltaTest,"CalmyPiratesCo.Ship.RidersKeepSmallMotion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyShipRiderDeltaTest::RunTest(const FString&)
{
	// 200 frames of a slow heel + drift, each far below the old 0.05 tolerance, with the hull away from the world
	// origin so a delta that pivots on the origin instead of the hull is caught.
	const FVector Harbour(52000,-18000,300);
	FTransform Ship(FRotator::ZeroRotator,Harbour),LastApplied=Ship;
	const FTransform RiderLocal(FRotator(0,30,0),FVector(-3080,-170,1370));
	FTransform Rider=RiderLocal*Ship;int32 Applied=0;
	for(int32 Frame=0;Frame<200;++Frame)
	{
		Ship=FTransform(FRotator(0,0,.01f*(Frame+1)),Harbour+FVector(.02f*(Frame+1),0,0));
		FTransform Delta;if(ACalmyShipMotion::ConsumeRiderDelta(LastApplied,Ship,Delta)){Rider=Rider*Delta;++Applied;}
	}
	FTransform Final;if(ACalmyShipMotion::ConsumeRiderDelta(LastApplied,Ship,Final))Rider=Rider*Final;
	const FVector Expected=(RiderLocal*Ship).GetLocation();
	TestTrue(TEXT("Sub-threshold frames are applied or accumulated"),Applied>0);
	TestTrue(TEXT("Rider stays fixed in the ship frame after a 2 degree heel"),FVector::Dist(Rider.GetLocation(),Expected)<.5f);
	TestTrue(TEXT("Rider orientation follows the hull"),Rider.GetRotation().Equals((RiderLocal*Ship).GetRotation(),1.e-3f));
	TestFalse(TEXT("No motion reports no delta"),ACalmyShipMotion::ConsumeRiderDelta(LastApplied,Ship,Final));

	const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFlySwarmMob* Swarm=World->SpawnActor<AFlySwarmMob>(RiderLocal.GetLocation(),FRotator::ZeroRotator,Spawn);
	if(!Swarm){World->DestroyWorld(false);return false;}
	const FTransform Tiny(FRotator(0,0,.001f),FVector::ZeroVector);
	Swarm->ApplyShipMotion(Tiny);
	TestTrue(TEXT("Colony applies a tiny ship delta"),FVector::Dist(Swarm->GetActorLocation(),Tiny.TransformPosition(RiderLocal.GetLocation()))<.01f);
	World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyHullProxyDefaultsTest,"CalmyPiratesCo.Ship.HullProxyDefaults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyHullProxyDefaultsTest::RunTest(const FString&)
{
	const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACalmyHullProxy* Proxy=World->SpawnActor<ACalmyHullProxy>(FVector(0,0,200),FRotator::ZeroRotator,Spawn);
	if(!Proxy){World->DestroyWorld(false);return false;}
	TestTrue(TEXT("Mass"),FMath::IsNearlyEqual(Proxy->HullMassKg,1200.f));
	TestNotNull(TEXT("Hull"),Proxy->Hull.Get()); TestNotNull(TEXT("Deck"),Proxy->Deck.Get());
	World->DestroyWorld(false); return true;
}
#endif