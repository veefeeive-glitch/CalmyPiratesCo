#include "Testing/CalmyWaterProbe.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"

bool UCalmyWaterProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value;const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyWaterProbe="),Value);
#endif
}
void UCalmyWaterProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_WATER %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyWaterProbe::Tick(float DT)
{
    if(Role.IsEmpty()) FParse::Value(FCommandLine::Get(),TEXT("CalmyWaterProbe="),Role);
    if(Stage==999 || !GetWorld()->HasBegunPlay()) return;
    if(Role!=TEXT("Visual") && ((Role==TEXT("Host") && GetWorld()->GetNetMode()!=NM_ListenServer) || (Role!=TEXT("Host") && GetWorld()->GetNetMode()!=NM_Client))) return;
    Age+=DT;
    ACalmyCharacter* Pawn=nullptr;ACalmyCargo* Float=nullptr;
    auto* Ocean=ACalmyOcean::Find(GetWorld());
    for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It) if(It->IsLocallyControlled() && Cast<APlayerController>(It->GetController())) Pawn=*It;
    for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It) if(It->CargoId==1) Float=*It;
    if(Age>110) { Finish(false,FString::Printf(TEXT("timeout stage=%d pos=%s"),Stage,Pawn?*Pawn->GetActorLocation().ToString():TEXT("no pawn")));return; }
    if(!Pawn || !Ocean || !Float) return;
    if(Role==TEXT("Visual"))
    {
        // Render-only inspection fixture. These poses are NOT traversal evidence.
        const FVector Poses[]={FVector(1000,-350,95),FVector(4700,-3800,2200),FVector(3100,0,-210)};
        const FVector Targets[]={FVector(3200,0,-220),FVector(600,0,-100),FVector(1400,0,0)};
        const int32 Shot=Stage/2;
        if(Shot>=3) { Finish(true,TEXT("render_only three_view_capture"));return; }
        if(Stage%2==0)
        {
            Pawn->GetCharacterMovement()->DisableMovement();
            Pawn->SetActorLocation(Poses[Shot],false,nullptr,ETeleportType::TeleportPhysics);
            Pawn->GetController()->SetControlRotation((Targets[Shot]-Poses[Shot]-FVector(0,0,64)).Rotation());
            StageTime=Age;++Stage;
        }
        else if(Age-StageTime>8)
        {
            const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification/WaterLab-Visual"));
            IFileManager::Get().MakeDirectory(*Dir,true);
            FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("View%d.png"),Shot+1),false,false);
            ++Stage;
        }
        return;
    }
    if(!bStarted)
    {
        bStarted=true;
        if(Role==TEXT("Host"))
        {
            // Test fixture initial placement only; every player route uses movement input.
            Float->SetActorLocation(FVector(3500,300,-80),false,nullptr,ETeleportType::TeleportPhysics);
            Float->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
            auto* Control=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(3600,1100,-80),FRotator::ZeroRotator);
            GravityControl=Control->GetStaticMeshComponent();GravityControl->SetMobility(EComponentMobility::Movable);
            GravityControl->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            GravityControl->SetWorldScale3D(FVector(.7));GravityControl->SetCollisionProfileName(TEXT("PhysicsActor"));GravityControl->SetSimulatePhysics(true);
        }
        UE_LOG(LogTemp,Display,TEXT("CALMY_WATER %s START"),*Role);GLog->Flush();
    }
    const float Lane=Role==TEXT("Host")?-160.f:(Role==TEXT("Client")?0.f:160.f);
    auto Move=[Pawn](FVector2D Target) { const FVector2D D=Target-FVector2D(Pawn->GetActorLocation());if(D.Size()<22) return true;Pawn->MoveIntent(D.GetSafeNormal());return false; };
    auto Next=[&](int32 N) { Stage=N;StageTime=Age;UE_LOG(LogTemp,Display,TEXT("CALMY_WATER %s STAGE %d pos=%s swimming=%d floatZ=%.1f waterZ=%.1f"),*Role,N,*Pawn->GetActorLocation().ToString(),Pawn->IsInOcean(),Float->GetActorLocation().Z,Ocean->HeightAt(Float->GetActorLocation()));GLog->Flush(); };
    if(Stage==0 && Move(FVector2D(-200,Lane))) Next(1);
    else if(Stage==1 && Move(FVector2D(1400,Lane))) Next(2);
    else if(Stage==2 && Move(FVector2D(3200,Lane))) Next(3);
    else if(Stage==3)
    {
        if(!Pawn->IsInOcean() || Pawn->ResetCount) { Finish(false,TEXT("water_entry_or_instant_rescue"));return; }
        if(!bWaterGrabAttempted)
        {
            // The current drifts the crate during late join. Swim into genuine
            // grab range, so rejection cannot pass just because of distance.
            if(FVector::Distance(Pawn->GetActorLocation(),Float->GetActorLocation())>600)
            { Move(FVector2D(Float->GetActorLocation())+FVector2D(-200,Lane));return; }
            RejectionsBeforeWater=Pawn->RejectedCommands;bWaterGrabAttempted=true;
            Pawn->ServerGrab(Float,++Pawn->LastIssuedSequence);
        }
        if(Move(FVector2D(3450,Lane))) Next(4);
    }
    else if(Stage==4 && Age-StageTime>5)
    {
        if(Pawn->HeldCargo || Pawn->RejectedCommands<=RejectionsBeforeWater) { Finish(false,TEXT("swimming_grab_not_rejected"));return; }
        if(FMath::Abs(Float->GetActorLocation().Z-Ocean->HeightAt(Float->GetActorLocation()))>120 || Float->ResetCount) { Finish(false,TEXT("buoyancy_not_sustained"));return; }
        if(Role==TEXT("Host") && (!GravityControl || GravityControl->GetComponentLocation().Z>-700)) { Finish(false,TEXT("gravity_negative_control"));return; }
        UE_LOG(LogTemp,Display,TEXT("CALMY_WATER %s READY"),*Role);GLog->Flush();Next(5);
    }
    else if(Stage==5 && Move(FVector2D(2800,Lane))) Next(6);
    else if(Stage==6 && Move(FVector2D(2400,Lane))) Next(7);
    else if(Stage==7 && Move(FVector2D(1400,Lane))) Next(8);
    else if(Stage==8)
    {
        if(Pawn->IsInOcean() || !Pawn->GetCharacterMovement()->IsMovingOnGround() || Pawn->ResetCount) { Finish(false,TEXT("ramp_exit_failed"));return; }
        if(Role!=TEXT("Host") || Age>80) Finish(true,TEXT("entry=1 swimming=1 ramp_exit=1 floating_cargo=1 swimming_grab_rejected=1 no_instant_rescue=1"));
    }
}
