#include "Testing/CalmyFlySwarmProbe.h"
#include "Gameplay/FlySwarmMob.h"
#include "Gameplay/CalmyGameMode.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmyFlySwarmProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value; const auto* W = Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyFlySwarmProbe="), Value);
#endif
}
void UCalmyFlySwarmProbe::Finish(bool OK, const FString& Detail)
{
    UE_LOG(LogTemp, Display, TEXT("CALMY_FLY_SWARM_TEST %s %s %s"), *Role, OK ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    GLog->Flush(); Age = -10000; FPlatformMisc::RequestExitWithStatus(false, OK ? 0 : 1);
}
void UCalmyFlySwarmProbe::Tick(float DT)
{
    if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyFlySwarmProbe="), Role);
    if (Age < 0 || !GetWorld()->HasBegunPlay()) return;
    const bool Visual = Role == TEXT("Visual");
    if (Visual)
        if (auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController())) PC->ResumeFromMenu();
    const bool Host = Role == TEXT("Host"), Solo = Role == TEXT("Solo") || Visual;
    const bool Placed = FParse::Param(FCommandLine::Get(), TEXT("FlyPlaced"));
    const int32 Expected = Placed ? 32 : 200;
    if (!Solo && GetWorld()->GetNetMode() != (Host ? NM_ListenServer : NM_Client)) return;
    Age += DT;
    if (Age > 150) { Finish(false, TEXT("timeout")); return; }
    if (Host)
    {
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (It->Get()) SeenControllers.Add(It->Get()->GetUniqueID());
        if (SeenControllers.Num() >= 3) AllPeersAge += DT;
    }
    if (!Placed && (Host || Solo) && !bSpawned && Age > 2)
    {
        FVector Center(0, 0, 180);
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (const APawn* P = It->Get()->GetPawn()) { Center += P->GetActorLocation(); break; }
        const FTransform Transform(FRotator::ZeroRotator, Center);
        AFlySwarmMob* Swarm = GetWorld()->SpawnActorDeferred<AFlySwarmMob>(AFlySwarmMob::StaticClass(), Transform);
        if (!Swarm) { Finish(false, TEXT("spawn_failed")); return; }
        Swarm->FlyCount = 200;
        int32 Hz = 25; FParse::Value(FCommandLine::Get(), TEXT("FlyHz="), Hz);
        Swarm->SimulationHz = Hz;
        Swarm->Tags.Add(TEXT("FlyStressTest"));
        // Persistent explicit trigger exercises sensing independently of player placement.
        ATargetPoint* Danger = GetWorld()->SpawnActor<ATargetPoint>(Center, FRotator::ZeroRotator);
        Swarm->DangerSources.Add(Danger);
        Swarm->FinishSpawning(Transform);
        bSpawned = true;
    }
    AFlySwarmMob* Swarm = nullptr;
    for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It)
        if (It->ActorHasTag(Placed ? TEXT("FlyR2_Gameplay") : TEXT("FlyStressTest"))) { Swarm = *It; break; }
    if (!Swarm || Swarm->GetVisibleFlyCount() != Expected) return;
    if (Placed && (!Swarm->FlyMesh || !Swarm->FlyMesh->GetPathName().Contains(TEXT("/Game/Creatures/FlyR2/SM_Fly_R2"))))
    { Finish(false, TEXT("placeholder_in_game")); return; }
    if (Swarm->GetSimulatedFlyCount() != ((Host || Solo) ? Expected : 0)) { Finish(false, TEXT("wrong_brain_authority")); return; }
    FTransform Pose; Swarm->FlyInstances->GetInstanceTransform(0, Pose, true);
    if (!bObserved)
    {
        bObserved = true; FirstPosition = Pose.GetLocation();
        UE_LOG(LogTemp, Display, TEXT("CALMY_FLY_SWARM_TEST %s %s"), *Role, Host ? TEXT("START") : TEXT("READY"));
        GLog->Flush();
    }
    ObservedSeconds += DT;
    bMoved |= FVector::Dist(FirstPosition, Pose.GetLocation()) > 5.;
    for (int32 I = 0; I < Expected; ++I)
    {
        Swarm->FlyInstances->GetInstanceTransform(I, Pose, true);
        if (Pose.ContainsNaN() || FVector::Dist(Pose.GetLocation(), Swarm->GetActorLocation()) > Swarm->RoamRadius + 1.)
        { Finish(false, TEXT("invalid_or_unbounded_pose")); return; }
    }
    const double StepMs = Swarm->GetLastSimulationMilliseconds();
    MaxStepMs = FMath::Max(MaxStepMs, StepMs); SumStepMs += StepMs; ++Samples;
    if (Visual)
    {
        if (!ReviewCamera.IsValid())
        {
            ReviewCamera = GetWorld()->SpawnActor<ACameraActor>();
            ReviewCamera->GetCameraComponent()->SetFieldOfView(65.f);
            GetWorld()->GetFirstPlayerController()->SetViewTarget(ReviewCamera.Get());
        }
        FVector Target = Swarm->GetActorLocation();
        FVector Offset(-260,-190,90);
        if (ObservedSeconds > 4)
        {
            Swarm->FlyInstances->GetInstanceTransform(0, Pose, true);
            Target = Pose.GetLocation(); Offset = FVector(12,-17,12);
        }
        ReviewCamera->SetActorLocation(Target+Offset);
        ReviewCamera->SetActorRotation((-Offset).Rotation());
        if ((ScreenshotStage == 0 && ObservedSeconds > 3) || (ScreenshotStage == 1 && ObservedSeconds > 6))
        {
            FString Dir; FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/(ScreenshotStage==0?TEXT("01-swarm.png"):TEXT("02-fly-closeup.png")), false, false);
            ++ScreenshotStage;
        }
    }
    if (Host ? AllPeersAge > 14.f : ObservedSeconds > 8.f)
    {
        const uint64 NeuralMs = Swarm->GetNeuralMilliseconds();
        const double ClockError = FMath::Abs(double(NeuralMs) - ObservedSeconds * 1000.);
        const bool BrainOK = !(Host || Solo) || ((Placed || Swarm->GetEscapeEvents() > 0) && ClockError < 250.);
        Finish(bMoved && BrainOK, FString::Printf(TEXT("instances=%d brains=%d moved=%d neural_ms=%llu clock_error_ms=%.1f escape_events=%llu step_ms_avg=%.2f max=%.2f placed=%d mesh=%s"),
            Expected, Swarm->GetSimulatedFlyCount(), bMoved, NeuralMs, ClockError, Swarm->GetEscapeEvents(), SumStepMs / FMath::Max(1, Samples), MaxStepMs, Placed, *Swarm->FlyMesh->GetPathName()));
    }
}
