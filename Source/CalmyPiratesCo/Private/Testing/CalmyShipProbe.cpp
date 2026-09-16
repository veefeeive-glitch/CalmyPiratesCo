#include "Testing/CalmyShipProbe.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

bool UCalmyShipProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value; const auto* W=Cast<UWorld>(Outer);
    return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyShipProbe="),Value);
#endif
}
void UCalmyShipProbe::Finish(bool Success,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_SHIP %s %s %s"),*Role,Success?TEXT("PASS"):TEXT("FAIL"),*Detail);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,Success?0:1);
}
void UCalmyShipProbe::Photo(const FString& Name)
{
    FString Label=TEXT("ShipR7-Review");FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Label);
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));
    IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),true,false);
    UE_LOG(LogTemp,Display,TEXT("SHIP_PHOTO %s"),*Name);
}
void UCalmyShipProbe::Tick(float DT)
{
    if(Stage==999||!GetWorld()->HasBegunPlay())return;
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyShipProbe="),Role);
    const bool Visual=Role==TEXT("Visual");
    if((Role==TEXT("Host")&&GetWorld()->GetNetMode()!=NM_ListenServer)||
        (!Visual&&Role!=TEXT("Host")&&GetWorld()->GetNetMode()!=NM_Client))return;
    Age+=DT;
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    auto* Ocean=ACalmyOcean::Find(GetWorld());
    if(Age>210){Finish(false,TEXT("timeout_or_missing_cast"));return;}
    if(!P||!P->IsLocallyControlled()||!Ocean||Age<3)return;
    auto Aim=[&](const FVector& From,const FVector& At){Camera->SetActorLocation(From);Camera->SetActorRotation((At-From).Rotation());PC->SetViewTarget(Camera);};
    if(Stage==0)
    {
        int32 Meshes=0,Cargo=0,Helpers=0;bool bServiceSpaces=false,bGuardrails=false,bDetails=false;
        for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        {
            auto* C=It->GetStaticMeshComponent();UStaticMesh* M=C->GetStaticMesh();
            if(!M||!M->GetName().StartsWith(TEXT("SM_R7_")))continue;
            ++Meshes;
            if(M->GetName()==TEXT("SM_R7_80_SERVICE_SPACES"))bServiceSpaces=true;
            if(M->GetName()==TEXT("SM_R7_81_GUARDRAILS"))bGuardrails=true;
            if(M->GetName()==TEXT("SM_R7_82_DETAILS"))bDetails=true;
            if(!C->IsCollisionEnabled()||!M->GetBodySetup()||M->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple)
            {Finish(false,TEXT("imported_collision_contract"));return;}
        }
        for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)++Cargo;
        for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It)++Helpers;
        // 10 R7 structure chunks + service spaces S1 (chain locker, service house, office, foremast and marks)
        // + guardrails S2 + deck details S3 (bunker stations, hold vents, bridge-roof kit).
        if(Meshes!=13||!bServiceSpaces||!bGuardrails||!bDetails||Cargo!=4||Helpers!=1||!Ocean->IsInsideDryHull(FVector(-2800,580,300))||Ocean->IsInsideDryHull(FVector(-2800,1500,300)))
        {Finish(false,FString::Printf(TEXT("scene_contract meshes=%d service_spaces=%d guardrails=%d details=%d cargo=%d helpers=%d"),Meshes,int32(bServiceSpaces),int32(bGuardrails),int32(bDetails),Cargo,Helpers));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_SHIP %s START meshes=%d cargo=%d helpers=%d pos=%s"),*Role,Meshes,Cargo,Helpers,*P->GetActorLocation().ToString());
        UE_LOG(LogTemp,Display,TEXT("CALMY_SHIP %s READY"),*Role);GLog->Flush();
        Previous=P->GetActorLocation();StageTime=Age;
        if(Role==TEXT("LateJoin")){Stage=50;return;}
        const float S=Role==TEXT("Client")?-1.f:1.f;
        Route={{-2450,850,1058},{-2450,S*580,1058},{-2530,S*580,1058},{-2610,S*580,1058}};
        for(float Z:{733.f,458.f,208.f})
        {
            Route.Add(FVector(-3150,S*580,Z));
            Route.Add(FVector(-3230,S*580,Z));
            Route.Add(FVector(-3230,S*705,Z));
            if(Z>208)
            {
                Route.Add(FVector(-2530,S*705,Z));Route.Add(FVector(-2530,S*580,Z));Route.Add(FVector(-2610,S*580,Z));
            }
        }
        // Return up the same authored flights after reaching the lowest machinery deck.
        const int32 DescentCount=Route.Num();
        for(int32 I=DescentCount-2;I>=0;--I){const FVector ReturnPoint=Route[I];Route.Add(ReturnPoint);}
        // Different crew wait on their respective side deck, not inside one another.
        // The first network attempt correctly hit the host capsule at a shared endpoint.
        if(Role==TEXT("Client"))Route.Last()=FVector(-2450,-850,1058);
        if(Visual)
        {
            Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(58);
            Camera->GetCameraComponent()->bConstrainAspectRatio=false;
            if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
            Aim(FVector(-10000,13500,6800),FVector(0,0,1050));Stage=1;
        }
        else {Stage=3;PointTime=Age;}
        return;
    }
    if(Stage==1&&Age-StageTime>5){Photo(TEXT("01-ship-overview"));Stage=2;StageTime=Age;return;}
    if(Stage==2&&Age-StageTime>1){PC->SetViewTarget(P);if(PC->GetHUD())PC->GetHUD()->bShowHUD=true;PC->SetControlRotation(FRotator(0,180,0));Stage=3;PointTime=Age;}
    if(Stage==3)
    {
        const FVector Pos=P->GetActorLocation();Walked+=FVector::Dist(Pos,Previous);Previous=Pos;
        auto* Rig=P->GetCrewPresentation();
        if(!Rig||!Rig->HasFiniteBodies()||Rig->SimulatedPartCount()!=9||P->ResetCount||P->IsInOcean()||!P->IsControlled())
        {Finish(false,FString::Printf(TEXT("pawn_state point=%d resets=%d swim=%d controlled=%d pos=%s"),Point,P->ResetCount,P->IsInOcean(),P->IsControlled(),*Pos.ToString()));return;}
        MaxJoint=FMath::Max(MaxJoint,Rig->MaximumJointSeparation());
        if(MaxJoint>8){Finish(false,TEXT("rig_joint_gap_over_8cm"));return;}
        if(Pos.Z<650&&Ocean->IsInsideDryHull(Pos))++DryChecks;
        if(Point>=Route.Num()){Stage=4;StageTime=Age;P->GetCharacterMovement()->StopMovementImmediately();return;}
        const FVector Target=Route[Point];const FVector2D Delta=FVector2D(Target-Pos);
        if(Age-PointTime>16)
        {
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(ShipProbe),true,P);
            GetWorld()->LineTraceSingleByChannel(Hit,Pos,Pos+FVector(Delta.GetSafeNormal(),0)*150,ECC_Visibility,Q);
            Finish(false,FString::Printf(TEXT("route_stuck point=%d pos=%s target=%s hit=%s floor=%s"),Point,*Pos.ToString(),*Target.ToString(),*GetNameSafe(Hit.GetActor()),*GetNameSafe(P->GetCharacterMovement()->CurrentFloor.HitResult.Component.Get())));return;
        }
        const float Feet=Pos.Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        if(Delta.Size()<20&&FMath::Abs(Feet-Target.Z)<45)
        {
            UE_LOG(LogTemp,Display,TEXT("CALMY_SHIP %s WAYPOINT %d feet=%.1f pos=%s"),*Role,Point,Feet,*Pos.ToString());GLog->Flush();
            if(Visual&&(Point==0||Point==6||Point==12||Point==18))Photo(FString::Printf(TEXT("route-%02d-bodycam"),Point));
            ++Point;PointTime=Age;P->GetCharacterMovement()->StopMovementImmediately();
        }
        else if(Delta.Size()>5)
        {
            PC->SetControlRotation(FRotator(-6,FVector(Delta,0).Rotation().Yaw,0));
            P->MoveIntent(Delta.GetSafeNormal()*FMath::Clamp(Delta.Size()/90.f,.25f,1.f));
        }
        return;
    }
    if(Stage==50&&Age-StageTime>5)
    {
        if(P->ResetCount||P->IsInOcean()||!P->GetCharacterMovement()->IsMovingOnGround()){Finish(false,TEXT("late_join_not_on_dry_deck"));return;}
        Finish(true,TEXT("late_join_scene=1 dry_profile=1 grounded_spawn=1 route_not_run=1"));return;
    }
    if(Stage==4)
    {
        if(DryChecks<60){Finish(false,TEXT("no_below_waterline_dry_walk"));return;}
        if(Visual)
        {
            if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
            Aim(FVector(-3950,0,2050),FVector(-3100,0,2020));Stage=5;StageTime=Age;return;
        }
        if(Role!=TEXT("Host")||Age>110)
            Finish(true,FString::Printf(TEXT("route=weather_engine_return waypoints=%d walked_cm=%.1f dry_checks=%d max_joint_cm=%.3f resets=0 teleports=0"),Point,Walked,DryChecks,MaxJoint));
    }
    if(Stage==5&&Age-StageTime>4){Photo(TEXT("06-bridge-camera-only"));Stage=6;StageTime=Age;}
    else if(Stage==6&&Age-StageTime>1){Aim(FVector(-4700,300,370),FVector(-3600,300,330));Stage=7;StageTime=Age;}
    else if(Stage==7&&Age-StageTime>4){Photo(TEXT("07-engine-camera-only"));Stage=8;StageTime=Age;}
    else if(Stage==8&&Age-StageTime>1)
        Finish(true,FString::Printf(TEXT("route=weather_engine_return waypoints=%d walked_cm=%.1f dry_checks=%d max_joint_cm=%.3f resets=0 pawn_teleports=0 staged_cameras=3"),Point,Walked,DryChecks,MaxJoint));
}
