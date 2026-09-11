#include "Testing/CalmyOceanPhysicsProbe.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/EngineVersion.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Algo/Reverse.h"

namespace {
FVector ReadPoint(const TSharedPtr<FJsonValue>& V) { const auto& A=V->AsArray(); return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()); }
TSharedPtr<FJsonValue> JsonPoint(const FVector& P) { return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)}); }
float CapsuleHeight() { return GetDefault<ACalmyCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(); }
float CapsuleRadius() { return GetDefault<ACalmyCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(); }
}
bool UCalmyOceanPhysicsProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    const auto* W=Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Param(FCommandLine::Get(),TEXT("CalmyOceanPhysics"));
#endif
}
void UCalmyOceanPhysicsProbe::Start()
{
    Started=true;
    CarryAudit=FParse::Param(FCommandLine::Get(),TEXT("OceanCarryAudit"));
    NpcAudit=FParse::Param(FCommandLine::Get(),TEXT("OceanNpcAudit"));
    CarryAudit|=NpcAudit;
    FString Raw;
    const FString Input=FPaths::ProjectSavedDir()/TEXT("OceanPhysics/ship.json");
    TSharedPtr<FJsonObject> Data;
    if (!FFileHelper::LoadFileToString(Raw,*Input) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Data))
    { UE_LOG(LogTemp,Error,TEXT("OCEAN_PHYSICS input unavailable")); Finished=true; FPlatformMisc::RequestExitWithStatus(false,2); return; }
    SourceHash=Data->GetStringField(TEXT("sha256"));
    // Lab is restricted to the engine's empty Entry map: no stable game-map actors are removed.
    if (!GetWorld()->GetMapName().EndsWith(TEXT("Entry")))
    { UE_LOG(LogTemp,Error,TEXT("OCEAN_PHYSICS requires /Engine/Maps/Entry")); Finished=true; FPlatformMisc::RequestExitWithStatus(false,2); return; }
    for (TActorIterator<ACalmyCharacter> It(GetWorld());It;++It) { It->SetActorEnableCollision(false); It->SetActorTickEnabled(false); It->GetCharacterMovement()->DisableMovement(); }
    for (const auto& CV:Data->GetArrayField(TEXT("chunks")))
    {
        const auto C=CV->AsObject();
        AActor* Actor=GetWorld()->SpawnActor<AActor>();
        UProceduralMeshComponent* Mesh=NewObject<UProceduralMeshComponent>(Actor,FName(*C->GetStringField(TEXT("name"))));
        Actor->SetRootComponent(Mesh);Actor->AddInstanceComponent(Mesh);
        Mesh->bUseComplexAsSimpleCollision=true;Mesh->bUseAsyncCooking=false;
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->RegisterComponent();
        TArray<FVector> V; TArray<int32> T;
        for (const auto& P:C->GetArrayField(TEXT("vertices"))) V.Add(ReadPoint(P));
        for (const auto& Index:C->GetArrayField(TEXT("triangles"))) T.Add(int32(Index->AsNumber()));
        Mesh->CreateMeshSection(0,V,T,TArray<FVector>(),TArray<FVector2D>(),TArray<FColor>(),TArray<FProcMeshTangent>(),true);
    }
    // Positive and negative controls calibrate the lab before its results can
    // be treated as evidence: cross a 15cm step; be stopped by a 2m wall.
    TArray<TSharedPtr<FJsonValue>> Routes=Data->GetArrayField(TEXT("routes"));
    for(int32 Test=0;Test<2;++Test)
    {
        const double Y=4000+Test*500;
        for(int32 Part=0;Part<2;++Part)
        {
            AActor* A=GetWorld()->SpawnActor<AActor>();
            auto* Mesh=NewObject<UProceduralMeshComponent>(A,FName(*FString::Printf(TEXT("CONTROL_%d_%d"),Test,Part)));
            A->SetRootComponent(Mesh);A->AddInstanceComponent(Mesh);Mesh->RegisterComponent();
            Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->bUseAsyncCooking=false;
            const double H=Test?200:15;
            Mesh->SetWorldLocation(FVector(200,Y,Part?H/2:-10));
            TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FProcMeshTangent> Tan;
            UKismetProceduralMeshLibrary::GenerateBoxMesh(Part?FVector(50,100,H/2):FVector(300,100,10),V,T,N,UV,Tan);
            Mesh->CreateMeshSection(0,V,T,N,UV,TArray<FColor>(),Tan,true);
        }
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Test?TEXT("CONTROL_wall"):TEXT("CONTROL_step15cm"));
        R->SetBoolField(TEXT("expected_blocked"),Test==1);
        R->SetArrayField(TEXT("points"),{JsonPoint(FVector(0,Y,0)),JsonPoint(FVector(400,Y,0))});
        Routes.Add(MakeShared<FJsonValueObject>(R));
    }
    for (const auto& RV:Routes)
    {
        auto R=RV->AsObject();
        const FString RouteName=R->GetStringField(TEXT("name"));
        const bool Control=RouteName.StartsWith(TEXT("CONTROL_"));
        if(CarryAudit && !Control && !(RouteName.StartsWith(TEXT("r6_")) || RouteName.StartsWith(TEXT("AUDIT_weather_to_hold_")) || RouteName==TEXT("AUDIT_mess_to_bridge"))) continue;
        for (int32 Direction=0;Direction<(CarryAudit && !Control?4:2);++Direction)
        {
            FOceanWalk W;W.Bulky=Direction>=2;
            W.Name=RouteName+(Direction%2?TEXT("_reverse"):TEXT("_forward"));
            if(CarryAudit && !Control) W.Name+=W.Bulky?TEXT("_bulky"):TEXT("_standard");
            R->TryGetBoolField(TEXT("expected_blocked"),W.ExpectedBlocked);
            for (const auto& P:R->GetArrayField(TEXT("points"))) W.Points.Add(ReadPoint(P));
            if(Direction%2) Algo::Reverse(W.Points);
            if(W.Points.Num()<2) continue;
            if(!CarryAudit) SpawnWalk(W);
            Walks.Add(W);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("OCEAN_PHYSICS START walks=%d hash=%s capsule=%.1fx%.1fcm isolated_pawns=1"),Walks.Num(),*SourceHash,CapsuleRadius(),CapsuleHeight());GLog->Flush();
}
void UCalmyOceanPhysicsProbe::SpawnWalk(FOceanWalk& W)
{
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto Spawn=[&]() {
        auto* P=GetWorld()->SpawnActor<ACalmyCharacter>(W.Points[0]+FVector(0,0,CapsuleHeight()+12),FRotator::ZeroRotator,Params);
        // Isolated movement lab only: suppress small-map rescue, not movement physics.
        P->SetActorTickEnabled(false);P->SetReplicates(false);
        P->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
        auto* M=P->GetCharacterMovement();M->bRunPhysicsWithNoController=true;
        M->SetMovementMode(MOVE_Walking);M->Activate(true);M->SetComponentTickEnabled(true);
        return P;
    };
    W.Pawn=Spawn();W.Previous=W.Pawn->GetActorLocation();
    if(CarryAudit && !W.Name.StartsWith(TEXT("CONTROL_")))
    {
        // Sequential local controlled bots, NOT network players or multiplayer proof.
        auto* Leader=GetWorld()->SpawnActor<APlayerController>();Leader->SetAsLocalPlayerController();Leader->Possess(W.Pawn);
        if(W.Bulky)
        {
            if(NpcAudit)
            {
                auto* NPC=GetWorld()->SpawnActor<ACalmyCrewHelper>(W.Pawn->GetActorLocation(),FRotator::ZeroRotator,Params);
                NPC->SetActorTickEnabled(false);NPC->SetReplicates(false);NPC->AssignLeader(W.Pawn);
                NPC->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
                W.Helper=NPC;
            }
            else {W.Helper=Spawn();auto* C=GetWorld()->SpawnActor<APlayerController>();C->SetAsLocalPlayerController();C->Possess(W.Helper);}
        }
        W.Pawn->GetCharacterMovement()->MaxWalkSpeed=W.Bulky?260.f:450.f;
    }
}
void UCalmyOceanPhysicsProbe::Complete(FOceanWalk& W,bool Passed,const FString& Reason)
{
    if(W.ExpectedBlocked) Passed=!Passed && Reason==TEXT("stuck");
    W.Done=true;
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),W.Name);R->SetBoolField(TEXT("pass"),Passed);
    R->SetStringField(TEXT("reason"),Reason);R->SetNumberField(TEXT("segment"),W.Target);R->SetNumberField(TEXT("seconds"),W.Age);
    if(CarryAudit) {
        R->SetBoolField(TEXT("actual_cargo_grabbed"),W.Grabbed);
        R->SetStringField(TEXT("cargo_kind"),W.Bulky?TEXT("Bulky"):TEXT("Standard"));
        R->SetStringField(TEXT("cargo_contact"),W.CargoContact);
        R->SetNumberField(TEXT("cargo_overlap_seconds"),W.CargoOverlapTime);
        R->SetField(TEXT("cargo_position"),JsonPoint(W.CargoPosition));
        R->SetNumberField(TEXT("cargo_yaw"),W.CargoRotation.Rotator().Yaw);
        if(W.Helper) R->SetField(TEXT("helper_position"),JsonPoint(W.Helper->GetActorLocation()));
    }
    R->SetField(TEXT("position"),JsonPoint(W.Pawn->GetActorLocation()));R->SetField(TEXT("target"),JsonPoint(W.Points[FMath::Min(W.Target,W.Points.Num()-1)]));
    if(!Passed || W.ExpectedBlocked)
    {
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(OceanBlocked),false,W.Pawn);Q.bReturnFaceIndex=true;
        FVector Dir=(W.Points[FMath::Min(W.Target,W.Points.Num()-1)]-W.Pawn->GetActorLocation());Dir.Z=0;
        const FVector Pos=W.Pawn->GetActorLocation();
        GetWorld()->SweepSingleByChannel(Hit,Pos,Pos+Dir.GetSafeNormal()*90,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(CapsuleRadius(),CapsuleHeight()),Q);
        R->SetStringField(TEXT("hit_component"),Hit.GetComponent()?Hit.GetComponent()->GetName():TEXT("none"));R->SetNumberField(TEXT("face"),Hit.FaceIndex);
        if(W.ExpectedBlocked) Passed=Passed && Hit.GetComponent() && Hit.GetComponent()->GetName()==TEXT("CONTROL_1_1");
    }
    R->SetBoolField(TEXT("pass"),Passed);
    R->SetArrayField(TEXT("trajectory"),W.Samples);
    Results.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("OCEAN_PHYSICS %s %s %s pos=%s target=%s"),Passed?TEXT("PASS"):TEXT("FAIL"),*W.Name,*Reason,*W.Pawn->GetActorLocation().ToString(),*W.Points.Last().ToString());
    if(W.Cargo) { W.Cargo->Destroy();W.Cargo=nullptr; }
    if(W.Helper) { if(auto* C=W.Helper->GetController()) C->Destroy();W.Helper->Destroy();W.Helper=nullptr; }
    if(CarryAudit) if(auto* C=W.Pawn->GetController()) C->Destroy();
    W.Pawn->Destroy();W.Pawn=nullptr;
}
void UCalmyOceanPhysicsProbe::Finish()
{
    Finished=true;int32 Passed=0;for(const auto& R:Results) if(R->AsObject()->GetBoolField(TEXT("pass"))) ++Passed;
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("source_sha256"),SourceHash);
    Report->SetStringField(TEXT("scope"),TEXT("Unreal Chaos per-triangle collision, production CharacterMovement and capsule; isolated forward/reverse walks; NOT multiplayer or all-space guarantee"));
    Report->SetBoolField(TEXT("carry_audit"),CarryAudit);
    Report->SetBoolField(TEXT("npc_audit"),NpcAudit);
    if(CarryAudit) Report->SetStringField(TEXT("cargo_overlap_pose"),TEXT("observed_mesh_component_transform_after_production_tick"));
    Report->SetStringField(TEXT("engine_version"),FEngineVersion::Current().ToString());
    Report->SetNumberField(TEXT("capsule_radius_cm"),CapsuleRadius());Report->SetNumberField(TEXT("capsule_half_height_cm"),CapsuleHeight());
    Report->SetNumberField(TEXT("max_step_height_cm"),GetDefault<ACalmyCharacter>()->GetCharacterMovement()->MaxStepHeight);
    Report->SetNumberField(TEXT("walkable_floor_angle"),GetDefault<ACalmyCharacter>()->GetCharacterMovement()->GetWalkableFloorAngle());
    Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Results.Num());Report->SetArrayField(TEXT("walks"),Results);
    int32 Controls=0,ControlPassed=0;
    for(const auto& R:Results) if(R->AsObject()->GetStringField(TEXT("name")).StartsWith(TEXT("CONTROL_"))) { ++Controls;if(R->AsObject()->GetBoolField(TEXT("pass"))) ++ControlPassed; }
    Report->SetNumberField(TEXT("controls_total"),Controls);Report->SetNumberField(TEXT("controls_passed"),ControlPassed);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FString Label=TEXT("latest");FParse::Value(FCommandLine::Get(),TEXT("OceanReport="),Label);
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("OceanPhysics")/(FPaths::GetCleanFilename(Label)+TEXT(".json"))));
    UE_LOG(LogTemp,Display,TEXT("OCEAN_PHYSICS COMPLETE %d/%d"),Passed,Results.Num());GLog->Flush();
    FPlatformMisc::RequestExitWithStatus(false,Passed==Results.Num() && Passed>0?0:1);
}
void UCalmyOceanPhysicsProbe::Tick(float Dt)
{
    if(Finished || !GetWorld()->HasBegunPlay()) return;
    if(!Started) { Start();return; }
    for(auto& W:Walks)
    {
        if(W.Done) continue;W.Age+=Dt;
        if(!W.Pawn) SpawnWalk(W);
        bool WaitForHelper=false;
        FVector Pos=W.Pawn->GetActorLocation();const FVector Foot=Pos-FVector(0,0,CapsuleHeight());
        if(W.Age>=W.NextSample)
        {
            W.NextSample=W.Age+.25f;
            auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("t"),W.Age);Sample->SetField(TEXT("position"),JsonPoint(Pos));
            Sample->SetBoolField(TEXT("grounded"),W.Pawn->GetCharacterMovement()->IsMovingOnGround());W.Samples.Add(MakeShared<FJsonValueObject>(Sample));
        }
        if(W.Age<.6f) { W.Previous=Pos;if(CarryAudit) break;continue; }
        if(CarryAudit && !W.Name.StartsWith(TEXT("CONTROL_")))
        {
            FVector Heading=W.Points[W.Target]-Foot;Heading.Z=0;
            if(!Heading.IsNearlyZero()) { W.Pawn->SetActorRotation(Heading.Rotation());W.Pawn->GetController()->SetControlRotation(Heading.Rotation()); }
            if(!W.Cargo) {
                const FTransform T(W.Pawn->GetActorRotation(),W.Pawn->GetCarryLocation());
                W.Cargo=GetWorld()->SpawnActorDeferred<ACalmyCargo>(ACalmyCargo::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                W.Cargo->Kind=W.Bulky?ECalmyCargoKind::Bulky:ECalmyCargoKind::Standard;W.Cargo->FinishSpawning(T);
                W.Grabbed=W.Cargo->TryGrab(W.Pawn);
                if(!W.Grabbed) { Complete(W,false,TEXT("cargo_grab_rejected"));break; }
            }
            if(W.Cargo->Carry.Holder!=W.Pawn) { Complete(W,false,TEXT("cargo_lost_or_helper_separated"));break; }
            if(auto* NPC=Cast<ACalmyCrewHelper>(W.Helper))
            {
                NPC->UpdateAssistance(Dt);
                const float Gap=FVector::Distance(NPC->GetActorLocation(),Pos);
                if(Gap>90) {NPC->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);W.Pawn->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);}
                WaitForHelper=Gap>235 || !W.Cargo->HasNearbyHelper(W.Pawn);
            }
            else if(W.Helper) {
                const FVector HP=W.Helper->GetActorLocation()-FVector(0,0,CapsuleHeight());
                const float Gap=FVector::Dist(HP,Foot);
                // Shared spawn only during setup; physical pawn collision once apart.
                if(Gap>90) { W.Helper->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);W.Pawn->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block); }
                if(W.HelperTarget<W.Points.Num()-1 && FVector::Dist2D(HP,W.Points[W.HelperTarget])<18 && FMath::Abs(HP.Z-W.Points[W.HelperTarget].Z)<32) ++W.HelperTarget;
                if(Gap>105 || !W.Cargo->HasNearbyHelper(W.Pawn)) {
                    FVector H=W.Points[W.HelperTarget]-HP;H.Z=0;
                    const float Throttle=FMath::Clamp((Gap-95.f)/100.f,.1f,.8f);
                    W.Helper->AddMovementInput(H.GetSafeNormal(),FMath::Min(double(Throttle),H.Size()/100.),true);
                }
                WaitForHelper=Gap>210 || !W.Cargo->HasNearbyHelper(W.Pawn);
                const FVector Ahead=W.Helper->GetActorLocation()-W.Pawn->GetActorLocation();
                if(Gap<145 && FVector::DotProduct(Ahead,Heading.GetSafeNormal())>30)
                {
                    // A returning carrier can meet the follower on a hairpin.
                    // Yield through ordinary movement, only into a swept-clear lane.
                    const FVector Side=FVector::CrossProduct(Heading.GetSafeNormal(),FVector::UpVector);
                    FCollisionQueryParams YieldQ(SCENE_QUERY_STAT(OceanHelperYield),false,W.Helper);
                    YieldQ.AddIgnoredActor(W.Pawn);YieldQ.AddIgnoredActor(W.Cargo);
                    for(float Sign:{1.f,-1.f}) {
                        FHitResult YieldHit;
                        const FVector From=W.Helper->GetActorLocation();
                        if(!GetWorld()->SweepSingleByChannel(YieldHit,From,From+Side*Sign*110,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(CapsuleRadius(),CapsuleHeight()-2),YieldQ)) {
                            W.Helper->ConsumeMovementInputVector();W.Helper->AddMovementInput(Side*Sign,.5f,true);WaitForHelper=true;break;
                        }
                    }
                }
            }
            // The production held mesh disables collision: independently query its
            // real oriented box so a visually wall-clipping crate cannot earn PASS.
            // Observe the mesh actually placed by the production cargo Tick,
            // not a second prediction from GetCarryLocation in the test itself.
            const FBox LocalBounds=W.Cargo->Mesh->GetStaticMesh()->GetBoundingBox();
            const FVector CP=W.Cargo->Mesh->GetComponentTransform().TransformPosition(LocalBounds.GetCenter());
            W.CargoPosition=CP;W.CargoRotation=W.Cargo->Mesh->GetComponentQuat();
            const FVector Extent=LocalBounds.GetExtent()*W.Cargo->Mesh->GetComponentScale().GetAbs()-FVector(1);
            FCollisionQueryParams Q(SCENE_QUERY_STAT(OceanCargoOverlap),false,W.Pawn);Q.AddIgnoredActor(W.Cargo);if(W.Helper) Q.AddIgnoredActor(W.Helper);
            TArray<FOverlapResult> Hits;
            const bool Overlap=GetWorld()->OverlapMultiByChannel(Hits,CP,W.CargoRotation,ECC_WorldStatic,FCollisionShape::MakeBox(Extent),Q);
            if(Overlap) { W.CargoOverlapTime+=Dt;for(const auto& H:Hits) if(H.bBlockingHit && H.GetComponent()) W.CargoContact=H.GetComponent()->GetName(); }
            else W.CargoOverlapTime=0;
            if(W.CargoOverlapTime>.15f) { Complete(W,false,TEXT("held_cargo_intersects_geometry"));break; }
        }
        if(WaitForHelper) { W.Still=0;if(W.Age>180) Complete(W,false,TEXT("helper_route_timeout"));break; }
        const FVector Target=W.Points[W.Target];FVector Delta=Target-Foot;
        if(Delta.Size2D()<18 && FMath::Abs(Delta.Z)<32 && W.Pawn->GetCharacterMovement()->IsMovingOnGround())
        {
            ++W.Target;W.Still=0;
            if(W.Target>=W.Points.Num()) { Complete(W,true,TEXT("walked"));continue; }
        }
        if(Pos.Z < FMath::Min(W.Points[W.Target].Z,W.Points[W.Target-1].Z)-100)
        { Complete(W,false,TEXT("fell_below_route"));continue; }
        if(FVector::DistSquared(Pos,W.Previous)<.04) W.Still+=Dt;else W.Still=0;
        W.Previous=Pos;
        if(W.Still>3 || W.Age>180) { Complete(W,false,W.Still>3?TEXT("stuck"):TEXT("timeout"));continue; }
        FVector Intent=W.Points[W.Target]-Foot;Intent.Z=0;
        W.Pawn->AddMovementInput(Intent.GetSafeNormal(),FMath::Min(1.0,Intent.Size()/100.0),true);
        if(CarryAudit) break; // Independent cases cannot supply each other's helpers.
    }
    if(Walks.Num()>0 && Results.Num()==Walks.Num()) Finish();
}
