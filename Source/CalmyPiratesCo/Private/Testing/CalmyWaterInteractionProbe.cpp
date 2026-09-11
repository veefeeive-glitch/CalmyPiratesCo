#include "Testing/CalmyWaterInteractionProbe.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

bool UCalmyWaterInteractionProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value;const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(),TEXT("CalmyWaterInteractionProbe="),Value);
#endif
}
void UCalmyWaterInteractionProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_WATER_INTERACTION %s %s %s mask=%d old_event=%d peak_height=%.3f peak_velocity=%.3f mesh_error=%.4f vertex_checks=%d"),
        *Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail,ObservedMask,OldEvent,PeakHeight,PeakVelocity,MaxMeshError,VertexChecks);
    GLog->Flush();Finished=true;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyWaterInteractionProbe::Tick(float Dt)
{
    if(Finished || !GetWorld()->HasBegunPlay()) return;
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyWaterInteractionProbe="),Role);
    const bool Visual=Role==TEXT("Visual");
    if(!Visual && ((Role==TEXT("Host") && GetWorld()->GetNetMode()!=NM_ListenServer)
        || (Role!=TEXT("Host") && GetWorld()->GetNetMode()!=NM_Client)))return;
    auto* Ocean=ACalmyOcean::Find(GetWorld());if(!Ocean)return;
    Age+=Dt;
    if(Age>85){Finish(false,TEXT("timeout"));return;}
    if(!Started)
    {
        if(!GetWorld()->GetMapName().EndsWith(TEXT("WaterLab"))){Finish(false,TEXT("fixture requires WaterLab"));return;}
        Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_WATER_INTERACTION %s START"),*Role);GLog->Flush();
    }
    // Test-only spawn above open water. Gravity/production buoyancy cause entry;
    // no actor teleports, scripted impulses or transforms after fixture creation.
    if(Ocean->HasAuthority() && Spawned<4 && Age>6+Spawned*4)
    {
        const FTransform T(FRotator(0,Spawned*25,Spawned%2?75:0),FVector(4800,-1500+Spawned*1000,Ocean->GetActorLocation().Z+500));
        auto* Cargo=GetWorld()->SpawnActorDeferred<ACalmyCargo>(ACalmyCargo::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Cargo->CargoId=1001+Spawned;Cargo->Kind=static_cast<ECalmyCargoKind>(Spawned);Cargo->FinishSpawning(T);
        UE_LOG(LogTemp,Display,TEXT("CALMY_WATER_INTERACTION DROP kind=%d mass=%.1f"),Spawned,Cargo->Mesh->GetMass());GLog->Flush();++Spawned;
    }
    const double Now=Ocean->WaveTime();
    for(const auto& D:Ocean->GetDisturbances())
    {
        const double EventAge=Now-D.Birth;
        for(int32 Kind=0;Kind<4;++Kind)
            if(FVector::Dist2D(D.Position,FVector(4800,-1500+Kind*1000,0))<350 && D.Amplitude>3)ObservedMask|=1<<Kind;
        if(!SeenSerials.Contains(D.Serial))
        {
            SeenSerials.Add(D.Serial);
            if(EventAge>.2 && EventAge<4.8)OldEvent=true;
            UE_LOG(LogTemp,Display,TEXT("CALMY_WATER_INTERACTION EVENT serial=%d birth=%.4f amplitude=%.4f x=%.3f y=%.3f received_age=%.3f"),D.Serial,D.Birth,D.Amplitude,D.Position.X,D.Position.Y,EventAge);
        }
        const FVector Crest=D.Position+FVector(180*EventAge,0,0);
        const float H=Ocean->DisturbanceHeightAt(Crest,Now);
        const float V=Ocean->VerticalVelocityAt(Crest)-ACalmyOcean::WaveVerticalVelocity(FVector2D(Crest-Ocean->GetActorLocation()),Now);
        if(!FMath::IsFinite(H)||!FMath::IsFinite(V)){Finish(false,TEXT("nonfinite_field"));return;}
        PeakHeight=FMath::Max(PeakHeight,FMath::Abs(H));PeakVelocity=FMath::Max(PeakVelocity,FMath::Abs(V));
        // Compare actual render mesh vertices with the physics evaluator at the
        // recorded mesh time. Covers displacement binding, not only event count.
        if(auto* Section=Ocean->Surface->GetProcMeshSection(0))
        {
            const FVector Local=Crest-Ocean->GetActorLocation();
            const int32 X=FMath::Clamp(FMath::RoundToInt(Local.X/100)+96,33,159);
            const int32 Y=FMath::Clamp(FMath::RoundToInt(Local.Y/100)+96,33,159);
            const FVector P=Section->ProcVertexBuffer[Y*193+X].Position;
            const double T=Ocean->GetSurfaceSampleTime();
            const float Expected=ACalmyOcean::WaveOffset(FVector2D(P),T)+Ocean->DisturbanceHeightAt(Ocean->GetActorLocation()+FVector(P.X,P.Y,0),T);
            MaxMeshError=FMath::Max(MaxMeshError,FMath::Abs(float(P.Z)-Expected));++VertexChecks;
        }
    }
    if(Ocean->GetDisturbances().Num()>48 || PeakHeight>24.001 || MaxMeshError>.05)
    {Finish(false,TEXT("field_bound_or_surface_mismatch"));return;}
    if(!Ready && (ObservedMask&3)==3)
    {Ready=true;UE_LOG(LogTemp,Display,TEXT("CALMY_WATER_INTERACTION %s READY"),*Role);GLog->Flush();}
    if(Visual)
    {
        for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)
            if(It->IsLocallyControlled() && Cast<APlayerController>(It->GetController()))
            {
                if(!VisualPlaced)
                {
                    // Camera-only pose, explicitly not player traversal evidence.
                    It->GetCharacterMovement()->DisableMovement();
                    It->SetActorLocation(FVector(4530,-1660,Ocean->GetActorLocation().Z+100),false,nullptr,ETeleportType::TeleportPhysics);
                    It->GetController()->SetControlRotation((FVector(4800,-1500,Ocean->GetActorLocation().Z)-It->GetActorLocation()-FVector(0,0,64)).Rotation());
                    if(auto* Rig=It->GetCrewPresentation())Rig->ResetBodies();VisualPlaced=true;
                }
            }
        const double Times[]={5.5,7.3,7.65,8.2,10.0};
        if(LastShot<4 && Age>Times[LastShot+1])
        {
            FString Dir;FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Dir);
            const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Dir));
            IFileManager::Get().MakeDirectory(*Path,true);
            FScreenshotRequest::RequestScreenshot(Path/FString::Printf(TEXT("Water%d.png"),++LastShot),false,false);
        }
    }
    if(Age>(Role==TEXT("Host")?65.:32.))
    {
        int32 Bodies=0;
        for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)if(It->CargoId>=1001&&It->CargoId<=1004)
        {
            ++Bodies;
            if(It->ResetCount || It->Carry.Holder || !It->Mesh || It->GetActorLocation().ContainsNaN()
                || It->Mesh->GetPhysicsLinearVelocity().ContainsNaN() || It->Mesh->GetPhysicsLinearVelocity().Size()>1200
                || FMath::Abs(It->GetActorLocation().Z-Ocean->HeightAt(It->GetActorLocation()))>120)
            {Finish(false,TEXT("cargo_not_stably_floating"));return;}
        }
        const bool Events=Role==TEXT("LateJoin") ? ObservedMask!=0&&OldEvent : ObservedMask==15;
        const float Acceleration=Ocean->GetPeakDisturbanceAcceleration();
        Finish(Bodies==4 && Events && PeakHeight>1 && PeakVelocity>1 && VertexChecks>0 && Acceleration>1,
            FString::Printf(TEXT("four_cargo_bodies=%d real_gravity_entry=1 no_rescue=1 applied_acceleration_delta=%.3f approximate_heightfield_not_full_fluid=1"),Bodies,Acceleration));
    }
}
