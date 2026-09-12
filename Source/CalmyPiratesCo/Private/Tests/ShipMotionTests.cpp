#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyHullProxy.h"
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