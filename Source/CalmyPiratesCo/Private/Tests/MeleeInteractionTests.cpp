#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyGameMode.h"
#include "Core/GameplayPolicies.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeThresholdTest,"CalmyPiratesCo.Melee.ProvocationAndTargeting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMeleeThresholdTest::RunTest(const FString&)
{
    UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    FActorSpawnParameters S; S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* A=W->SpawnActor<ACalmyCharacter>(FVector(0,0,100),FRotator::ZeroRotator,S);
    auto* B=W->SpawnActor<ACalmyCharacter>(FVector(0,350,100),FRotator::ZeroRotator,S);
    auto* M=W->SpawnActor<ACalmyCrewHelper>(FVector(140,0,100),FRotator(0,180,0),S);
    TestTrue(TEXT("Aim resolves Misha at short reach"),A->FindMeleeTarget()==M);
    TestTrue(TEXT("Empty-hand swing accepted"),A->PerformPunch());
    TestEqual(TEXT("One confirmed hit"),M->MeleeHitsReceived,1);
    TestFalse(TEXT("Immediate spam cannot swing again"),A->PerformPunch());
    TestFalse(TEXT("First hit does not retaliate"),M->IsDefending());
    M->ReceiveMeleeHit(B);
    M->ReceiveMeleeHit(A);
    TestFalse(TEXT("Different attackers do not combine three hits"),M->IsDefending());
    TestEqual(TEXT("A has two hits"),M->GetProvocation(A),2);
    M->ReceiveMeleeHit(A);
    TestTrue(TEXT("Third hit selects only its attacker"),M->Aggressor==A);
    TestTrue(TEXT("Misha can physically answer with the same attack resolver"),M->PerformPunch());
    TestEqual(TEXT("Retaliation damages attacker"),A->Health,88.f);
    TestEqual(TEXT("Bystander unhurt"),B->Health,100.f);
    for(int32 I=0;I<20;++I)B->ReceiveMeleeHit(A);
    TestEqual(TEXT("Crew punching crew is a shove, not damage"),B->Health,100.f);
    TestEqual(TEXT("Crew shove still registers as a hit"),B->MeleeHitsReceived,20);
    TestTrue(TEXT("Crew shove never knocks down"),B->Lifecycle==ECalmyPlayerLifecycleState::Controlled);
    TestEqual(TEXT("Crew provokes Misha for 6"),FCalmyMeleePolicy::DamageFor(false,true),6.f);
    TestEqual(TEXT("Misha answers for 12"),FCalmyMeleePolicy::DamageFor(true,false),12.f);
    A->SetActorRotation(FRotator(0,180,0)); TestNull(TEXT("Facing away misses"),A->FindMeleeTarget());
    A->SetActorRotation(FRotator::ZeroRotator); M->SetActorLocation(FVector(500,0,100));
    TestNull(TEXT("Beyond reach misses"),A->FindMeleeTarget());
    M->SetActorLocation(FVector(140,0,100));
    auto* Wall=W->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(4,100,100));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Wall->SetActorLocation(FVector(80,0,120));
    TestNull(TEXT("Wall occludes short punch"),A->FindMeleeTarget());
    W->DestroyWorld(false); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyWheelTest,"CalmyPiratesCo.Interaction.RadialSelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlyWheelTest::RunTest(const FString&)
{
    TestEqual(TEXT("Centre cancels"),ACalmyPlayerController::WheelSector(FVector2D(0,0),200),-1);
    TestEqual(TEXT("Outside wheel does not select"),ACalmyPlayerController::WheelSector(FVector2D(250,0),200),-1);
    TestEqual(TEXT("Top greets"),ACalmyPlayerController::WheelSector(FVector2D(0,-130),200),0);
    TestEqual(TEXT("Right calms"),ACalmyPlayerController::WheelSector(FVector2D(130,0),200),2);
    TestEqual(TEXT("Bottom recalls"),ACalmyPlayerController::WheelSector(FVector2D(0,130),200),4);
    TestEqual(TEXT("Left shoos away"),ACalmyPlayerController::WheelSector(FVector2D(-130,0),200),6);
    TestEqual(TEXT("Upper-right be wary"),ACalmyPlayerController::WheelSector(FVector2D(92,-92),200),1);
    TestEqual(TEXT("Lower-right safe here"),ACalmyPlayerController::WheelSector(FVector2D(92,92),200),3);
    return true;
}
#endif
