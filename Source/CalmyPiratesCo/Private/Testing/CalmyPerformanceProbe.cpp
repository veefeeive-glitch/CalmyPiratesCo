#include "Testing/CalmyPerformanceProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/FlySwarmMob.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
bool UCalmyPerformanceProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V; const auto* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyPerformanceProbe="),V);
#endif
}
void UCalmyPerformanceProbe::Tick(float Dt)
{
    if(Finished||!GetWorld()->HasBegunPlay())return;
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());if(!PC||!PC->GetPawn())return;
    PC->ResumeFromMenu(); const double Now=FPlatformTime::Seconds();
    if(!Camera.IsValid())
    {
        FParse::Value(FCommandLine::Get(),TEXT("CalmyPerformanceProbe="),Label);
        Start=Previous=Now;Camera=GetWorld()->SpawnActor<ACameraActor>();
        Camera->SetActorLocation(FVector(-1100,-220,460));Camera->SetActorRotation((FVector(-800,0,390)-Camera->GetActorLocation()).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(90);Camera->GetCameraComponent()->bConstrainAspectRatio=false;
        PC->GetPawn()->SetActorLocation(FVector(-1200,-200,310));
        if(auto* C=Cast<ACharacter>(PC->GetPawn()))C->GetCharacterMovement()->DisableMovement();
        if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        UE_LOG(LogTemp,Display,TEXT("CALMY_PERF %s START fixed_hold_camera"),*Label);
    }
    PC->SetViewTarget(Camera.Get());
    double BrainMs=0;
    for(TActorIterator<AFlySwarmMob> It(GetWorld());It;++It){It->bRoamShip=false;BrainMs+=It->GetLastSimulationMilliseconds();}
    if(Now-Start>8)
    {
        Frames.Add((Now-Previous)*1000);Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
        Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Brain.Add(BrainMs);
    }
    Previous=Now;
    if(Now-Start<33)return;
    auto Mean=[](const TArray<double>& A){double Sum=0;for(double V:A)Sum+=V;return Sum/FMath::Max(1,A.Num());};
    Frames.Sort(); const double FrameMean=Mean(Frames);
    UE_LOG(LogTemp,Display,TEXT("CALMY_PERF %s RESULT frames=%d fps=%.2f frame_ms=%.3f p95_ms=%.3f game_ms=%.3f render_ms=%.3f gpu_ms=%.3f swarm_step_ms=%.3f"),*Label,Frames.Num(),1000/FrameMean,FrameMean,Frames[FMath::Min(Frames.Num()-1,FMath::FloorToInt(Frames.Num()*.95))],Mean(Game),Mean(Render),Mean(Gpu),Mean(Brain));
    GLog->Flush();Finished=true;FPlatformMisc::RequestExitWithStatus(false,0);
}
