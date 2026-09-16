#include "Testing/CalmyFlyTravelProbe.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/FlySwarmMob.h"
#include "Gameplay/CalmyGameMode.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
bool UCalmyFlyTravelProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V; const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyFlyTravelProbe="),V);
#endif
}
void UCalmyFlyTravelProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_FLY_TRAVEL_TEST %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);
    GLog->Flush();Finished=true;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyFlyTravelProbe::Tick(float Dt)
{
    if(Finished || !GetWorld()->HasBegunPlay())return;
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyFlyTravelProbe="),Role);
    const bool Visual=Role==TEXT("Visual"), Server=Role==TEXT("Host") || Visual;
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());if(!PC || !PC->GetPawn())return;
    PC->ResumeFromMenu();
    AFlySwarmMob* F=nullptr;int32 Count=0;
    for(TActorIterator<AFlySwarmMob> It(GetWorld());It;++It){F=*It;++Count;}
    if(!F)return;
    if(Count!=1 || !F->IsRoamingShip()){Finish(false,TEXT("expected_one_autonomous_colony"));return;}
    Age+=Dt;
    if(!Started)
    {
        Started=true;Initial=Previous=F->GetActorLocation();
        UE_LOG(LogTemp,Display,TEXT("CALMY_FLY_TRAVEL_TEST %s START"),*Role);GLog->Flush();
    }
    if(!Ready && Age>2){Ready=true;UE_LOG(LogTemp,Display,TEXT("CALMY_FLY_TRAVEL_TEST %s READY"),*Role);GLog->Flush();}
    const FVector Center=F->GetActorLocation();
    if(Visual)
    {
        if(!Camera.IsValid()) {Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(65);}
        FVector Eye=Center+FVector(-210,-180,85); FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FlyPhoto),true,F);
        if(GetWorld()->LineTraceSingleByChannel(Hit,Center,Eye,ECC_Visibility,Q))Eye=Hit.Location+Hit.Normal*8;
        Camera->SetActorLocation(Eye);Camera->SetActorRotation((Center-Eye).Rotation());PC->SetViewTarget(Camera.Get());
        if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        if(Photos<3 && Age>12+Photos*28)
        {
            FString Dir;FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Dir);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/FString::Printf(TEXT("%02d-travelling-colony.png"),++Photos),false,false);
        }
    }
    // The freighter drifts and heels, carrying the colony with it. Compare samples in the ship frame so ship motion
    // between two probe ticks is not mistaken for a fly moving through a wall.
    const ACalmyShipMotion* Ship=ACalmyShipMotion::Find(GetWorld());
    const FTransform ShipFrame=Ship?Ship->GetActorTransform():FTransform::Identity;
    const FVector PreviousNow=ShipFrame.TransformPosition(PreviousLocal);
    Walked+=FVector::Dist(Center,Previous);MaxDistance=FMath::Max(MaxDistance,float(FVector::Dist(Center,Initial)));
    if(Server && bHavePreviousLocal && FVector::Dist(Center,PreviousNow)>1 && !F->CanFlySegment(PreviousNow,Center,17.f))
    {Finish(false,TEXT("leader_crossed_collision"));return;}
    Previous=Center;PreviousLocal=ShipFrame.InverseTransformPosition(Center);bHavePreviousLocal=true;
    LeftHold|=Center.Z>1100 || Center.X<-2100 || Center.X>1100 || FMath::Abs(Center.Y)>850;
    Rested|=F->Activity==EFlyActivity::Rest;
    MaxStep=FMath::Max(MaxStep,F->GetLastSimulationMilliseconds());
    const auto& Poses=F->GetFlyPoses();
    if(Poses.Num()==32)
    {
        for(int32 I=0;I<Poses.Num();++I)
        {
            const FVector P=Poses[I].Position;
            MaxSpread=FMath::Max(MaxSpread,float(FVector::Dist(P,Center)));
            const FVector From=PreviousFlies.Num()==32?ShipFrame.TransformPosition(PreviousFlies[I]):P;
            if(Server && PreviousFlies.Num()==32 && FVector::Dist(From,P)>1 && !F->CanFlySegment(From,P,2.f))
            {Finish(false,FString::Printf(TEXT("fly_crossed_collision i=%d from=%s to=%s"),I,*From.ToString(),*P.ToString()));return;}
        }
        PreviousFlies.Reset();for(const auto& P:Poses)PreviousFlies.Add(ShipFrame.InverseTransformPosition(FVector(P.Position)));
    }
    if(Age-LastReport>10)
    {
        LastReport=Age;UE_LOG(LogTemp,Display,TEXT("FLY_TRAVEL age=%.0f center=%s target=%s walked=%.0f spread=%.0f activity=%d step_ms=%.2f"),Age,*Center.ToString(),*F->TravelTarget.ToString(),Walked,MaxSpread,int32(F->Activity),MaxStep);GLog->Flush();
    }
    if(Age>(Server?125.f:Role==TEXT("LateJoin")?80.f:85.f))
    {
        const bool OK=LeftHold && MaxDistance>1000 && Walked>1500 && MaxSpread<350 && Poses.Num()==32
            && (Server?F->GetSimulatedFlyCount()==32:F->GetSimulatedFlyCount()==0);
        Finish(OK,FString::Printf(TEXT("one_mob=1 left_hold=%d walked=%.0f max_distance=%.0f spread=%.0f rest=%d poses=%d brains=%d max_step_ms=%.2f"),LeftHold,Walked,MaxDistance,MaxSpread,Rested,Poses.Num(),F->GetSimulatedFlyCount(),MaxStep));
    }
}
