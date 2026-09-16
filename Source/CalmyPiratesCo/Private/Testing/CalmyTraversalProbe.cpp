#include "Testing/CalmyTraversalProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Core/GameplayPolicies.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace CalmyTraversal
{
	constexpr float CapsuleRadius = 34.f;
	constexpr float CapsuleHalfHeight = 88.f;
	constexpr float ReachCm = 45.f;
	constexpr float StuckSeconds = 3.5f;
	constexpr float EdgePushSeconds = 1.2f;
	constexpr float EdgeFallCm = 120.f;
	static const FVector EdgeDirs[] = {FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0)};
	static const TCHAR* EdgeNames[] = {TEXT("fwd"), TEXT("aft"), TEXT("stbd"), TEXT("port")};
	FString Vec(const FVector& V) { return FString::Printf(TEXT("[%.0f,%.0f,%.0f]"), V.X, V.Y, V.Z); }
	FString Escape(FString S) { return S.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")); }
}

bool UCalmyTraversalProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value; const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyTraversalProbe="), Value);
#endif
}

void UCalmyTraversalProbe::Finish(bool bOK, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_TRAVERSAL %s %s"), bOK ? TEXT("DONE") : TEXT("FAIL"), *Detail);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bOK ? 0 : 1);
}

FVector UCalmyTraversalProbe::ToWorld(const FVector& Local) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetActorTransform().TransformPosition(Ship->GetRestTransform().InverseTransformPosition(Local)) : Local;
}

FVector UCalmyTraversalProbe::ToLocal(const FVector& World) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetRestTransform().TransformPosition(Ship->GetActorTransform().InverseTransformPosition(World)) : World;
}

FVector UCalmyTraversalProbe::ShipDirToWorld(const FVector& ShipLocalDir) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	if (!Ship) return ShipLocalDir;
	const FVector RestDir = Ship->GetRestTransform().InverseTransformVectorNoScale(ShipLocalDir);
	return Ship->GetActorTransform().TransformVectorNoScale(RestDir).GetSafeNormal2D();
}

bool UCalmyTraversalProbe::LoadRoutes()
{
	FString Path = FPaths::ProjectDir() / TEXT("Evidence/ShipTraversal/R6-Recheck-20260905-223837/route-input-snapshot.json");
	FParse::Value(FCommandLine::Get(), TEXT("CalmyTraversalRoutes="), Path);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) return false;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Root->TryGetArrayField(TEXT("routes"), List)) return false;
	auto ReadPoints = [](const TArray<TSharedPtr<FJsonValue>>& Array)
	{
		TArray<FVector> Points;
		for (const TSharedPtr<FJsonValue>& P : Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& XYZ = P->AsArray();
			if (XYZ.Num() == 3) Points.Add(FVector(XYZ[0]->AsNumber(), XYZ[1]->AsNumber(), XYZ[2]->AsNumber()));
		}
		return Points;
	};
	for (const TSharedPtr<FJsonValue>& Value : *List)
	{
		const TSharedPtr<FJsonObject> Object = Value->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(TEXT("points"), Points)) continue;
		FRoute Route; Route.Name = Object->GetStringField(TEXT("name")); Route.Points = ReadPoints(*Points);
		if (Route.Points.Num() >= 2) Routes.Add(MoveTemp(Route));
	}
	// R7 moved a few room routes; replay the reviewed coordinates, not the R6 ones.
	FString Changes = FPaths::ProjectDir() / TEXT("Evidence/ShipTraversal/R7-Cargo-20260906/acceptance.json");
	FParse::Value(FCommandLine::Get(), TEXT("CalmyTraversalChanges="), Changes);
	TSharedPtr<FJsonObject> Accept;
	if (FFileHelper::LoadFileToString(Text, *Changes) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Accept) && Accept.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Moved = nullptr;
		if (Accept->TryGetArrayField(TEXT("reviewed_route_changes"), Moved))
			for (const TSharedPtr<FJsonValue>& Value : *Moved)
			{
				const TSharedPtr<FJsonObject> Change = Value->AsObject();
				const TArray<TSharedPtr<FJsonValue>>* After = nullptr;
				if (!Change.IsValid() || !Change->TryGetArrayField(TEXT("after"), After)) continue;
				const FString Name = Change->GetStringField(TEXT("name"));
				for (FRoute& Route : Routes) if (Route.Name == Name) Route.Points = ReadPoints(*After);
			}
	}
	return Routes.Num() > 0;
}

ACharacter* UCalmyTraversalProbe::SpawnWalker(const FVector& Floor)
{
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ACharacter* Body = GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(), ToWorld(Floor + FVector(0, 0, CalmyTraversal::CapsuleHalfHeight + 4)), FRotator::ZeroRotator, Spawn);
	if (!Body) return nullptr;
	// Same capsule and floor rules as ACalmyCharacter; walkers ignore each other.
	Body->GetCapsuleComponent()->SetCapsuleSize(CalmyTraversal::CapsuleRadius, CalmyTraversal::CapsuleHalfHeight);
	Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	if (Body->GetMesh()) Body->GetMesh()->SetVisibility(false);
	UCharacterMovementComponent* Move = Body->GetCharacterMovement();
	Move->bRunPhysicsWithNoController = true;
	Move->MaxWalkSpeed = FCalmyMovementPolicy::MaxSpeedCentimetersPerSecond * .7f;
	Move->SetWalkableFloorAngle(50.f);
	Move->MaxStepHeight = 45.f;
	Move->bOrientRotationToMovement = true;
	Move->SetMovementMode(MOVE_Walking);
	return Body;
}

void UCalmyTraversalProbe::StartJob(FWalker& W)
{
	const int32 Index = NextJob++;
	W = FWalker();
	W.Job = Index;
	const FJob& Job = Jobs[Index];
	const FVector StartFloor = Job.bEdge ? Job.EdgePoint : (Job.bReverse ? Routes[Job.Route].Points.Last() : Routes[Job.Route].Points[0]);
	ACharacter* Body = SpawnWalker(StartFloor);
	if (!Body) { EndJob(W, false, TEXT("spawn_failed"), FString(), StartFloor); return; }
	W.Body = Body;
	W.SegmentAge = -.6f; // land first
	W.BestDistance = BIG_NUMBER;
}

FString UCalmyTraversalProbe::BlockingHitName(ACharacter* Body, const FVector& WorldDir) const
{
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TraversalBlock), false, Body);
	const FVector From = Body->GetActorLocation();
	if (!GetWorld()->SweepSingleByChannel(Hit, From, From + WorldDir * 90.f, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(CalmyTraversal::CapsuleRadius - 4, CalmyTraversal::CapsuleHalfHeight - 10), Query))
		return TEXT("none");
	// Actor labels do not exist in -game; the static mesh asset names the real blocker (hull chunk, S1, door...).
	const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	const FString Asset = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : GetNameSafe(Hit.GetComponent());
	const FVector Local = ToLocal(Hit.ImpactPoint);
	return FString::Printf(TEXT("%s/%s@%.0f,%.0f,%.0f"), *GetNameSafe(Hit.GetActor()), *Asset, Local.X, Local.Y, Local.Z);
}

void UCalmyTraversalProbe::EndJob(FWalker& W, bool bPass, const FString& Reason, const FString& Hit, const FVector& Local)
{
	const FJob& Job = Jobs[W.Job];
	if (Job.bEdge)
	{
		++EdgeTotal;
		if (!bPass) ++EdgePointsWithFalls;
		Results.Add(FString::Printf(TEXT("{\"type\":\"edge\",\"point\":%s,\"pass\":%s,\"falls\":[%s]}"),
			*CalmyTraversal::Vec(Job.EdgePoint), bPass ? TEXT("true") : TEXT("false"), *FString::Join(W.EdgeFalls, TEXT(","))));
		if (!bPass) UE_LOG(LogTemp, Display, TEXT("CALMY_TRAVERSAL EDGE_FALL point=%s %s"), *CalmyTraversal::Vec(Job.EdgePoint), *FString::Join(W.EdgeFalls, TEXT(" ")));
	}
	else
	{
		++WalkTotal;
		if (bPass) ++WalkPassed;
		const FString Name = Routes[Job.Route].Name + (Job.bReverse ? TEXT("_reverse") : TEXT("_forward"));
		Results.Add(FString::Printf(TEXT("{\"type\":\"walk\",\"name\":\"%s\",\"pass\":%s,\"reason\":\"%s\",\"segment\":%d,\"local\":%s,\"hit\":\"%s\"}"),
			*CalmyTraversal::Escape(Name), bPass ? TEXT("true") : TEXT("false"), *Reason, W.Segment, *CalmyTraversal::Vec(Local), *CalmyTraversal::Escape(Hit)));
		if (!bPass) UE_LOG(LogTemp, Display, TEXT("CALMY_TRAVERSAL WALK_FAIL %s reason=%s segment=%d local=%s hit=%s"), *Name, *Reason, W.Segment, *CalmyTraversal::Vec(Local), *Hit);
	}
	if (ACharacter* Body = W.Body.Get()) Body->Destroy();
	W.Body.Reset();
	W.Job = -1;
}

void UCalmyTraversalProbe::StepWalk(FWalker& W, float DT)
{
	ACharacter* Body = W.Body.Get();
	const FJob& Job = Jobs[W.Job];
	const TArray<FVector>& Points = Routes[Job.Route].Points;
	auto PointAt = [&](int32 I) { return Job.bReverse ? Points[Points.Num() - 1 - I] : Points[I]; };
	if (!Body) { EndJob(W, false, TEXT("walker_lost"), FString(), PointAt(0)); return; }
	W.SegmentAge += DT;
	const FVector Feet = ToLocal(Body->GetActorLocation()) - FVector(0, 0, CalmyTraversal::CapsuleHalfHeight);
	if (W.SegmentAge < 0) return;
	const FVector Target = PointAt(W.Segment);
	const FVector Delta = Target - Feet;
	const float Flat = float(Delta.Size2D());
	if (W.SegmentLimit <= 0) W.SegmentLimit = FMath::Max(10.f, float(FVector::Dist(PointAt(W.Segment - 1), Target)) / 200.f + 6.f);
	if (Flat < CalmyTraversal::ReachCm && FMath::Abs(Delta.Z) < 140.f)
	{
		if (++W.Segment >= Points.Num()) { EndJob(W, true, TEXT("arrived"), FString(), Feet); return; }
		W.SegmentAge = 0; W.SegmentLimit = 0; W.BestDistance = BIG_NUMBER; W.SinceProgress = 0;
		return;
	}
	const FVector WorldDir = (ToWorld(Target) - ToWorld(Feet)).GetSafeNormal2D();
	Body->AddMovementInput(WorldDir, 1.f);
	if (Flat < W.BestDistance - 10.f) { W.BestDistance = Flat; W.SinceProgress = 0; }
	else W.SinceProgress += DT;
	const float LowFloor = FMath::Min(PointAt(W.Segment).Z, PointAt(W.Segment - 1).Z);
	if (Feet.Z < LowFloor - 180.f) { EndJob(W, false, TEXT("fell"), BlockingHitName(Body, WorldDir), Feet); return; }
	if (W.SinceProgress > CalmyTraversal::StuckSeconds) { EndJob(W, false, TEXT("stuck"), BlockingHitName(Body, WorldDir), Feet); return; }
	if (W.SegmentAge > W.SegmentLimit) { EndJob(W, false, TEXT("timeout"), BlockingHitName(Body, WorldDir), Feet); return; }
}

void UCalmyTraversalProbe::StepEdge(FWalker& W, float DT)
{
	ACharacter* Body = W.Body.Get();
	const FJob& Job = Jobs[W.Job];
	if (!Body) { EndJob(W, false, TEXT("walker_lost"), FString(), Job.EdgePoint); return; }
	W.EdgeAge += DT;
	if (W.EdgeAge < .5f) return; // land at the point first
	const FVector Feet = ToLocal(Body->GetActorLocation()) - FVector(0, 0, CalmyTraversal::CapsuleHalfHeight);
	Body->AddMovementInput(ShipDirToWorld(CalmyTraversal::EdgeDirs[W.EdgeDir]), 1.f);
	const bool bFalling = Body->GetCharacterMovement()->IsFalling();
	W.FallingSeconds = bFalling ? W.FallingSeconds + DT : 0;
	const float Drop = float(Job.EdgePoint.Z - Feet.Z);
	// Walking down stairs is not a fall: only sustained free fall with a real drop counts as an unguarded edge.
	const bool bFell = bFalling && W.FallingSeconds > .3f && Drop > CalmyTraversal::EdgeFallCm;
	if (bFell || W.EdgeAge > .5f + CalmyTraversal::EdgePushSeconds)
	{
		if (bFell)
			W.EdgeFalls.Add(FString::Printf(TEXT("{\"dir\":\"%s\",\"from\":%s,\"to\":%s,\"drop_cm\":%.0f}"), CalmyTraversal::EdgeNames[W.EdgeDir],
				*CalmyTraversal::Vec(Job.EdgePoint), *CalmyTraversal::Vec(Feet), Drop));
		if (++W.EdgeDir >= 4) { EndJob(W, W.EdgeFalls.Num() == 0, TEXT("edge"), FString(), Job.EdgePoint); return; }
		Body->SetActorLocation(ToWorld(Job.EdgePoint + FVector(0, 0, CalmyTraversal::CapsuleHalfHeight + 4)), false, nullptr, ETeleportType::TeleportPhysics);
		Body->GetCharacterMovement()->StopMovementImmediately();
		Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		W.EdgeAge = 0; W.FallingSeconds = 0;
	}
}

void UCalmyTraversalProbe::WriteReport() const
{
	const FString Folder = FPaths::ProjectDir() / TEXT("Saved/Verification") / FPaths::GetCleanFilename(Dir);
	IFileManager::Get().MakeDirectory(*Folder, true);
	const FString Json = FString::Printf(TEXT("{\n\"map\":\"%s\",\n\"walks_passed\":%d,\n\"walks_total\":%d,\n\"edge_points\":%d,\n\"edge_points_with_falls\":%d,\n\"results\":[\n%s\n]\n}\n"),
		*GetWorld()->GetMapName(), WalkPassed, WalkTotal, EdgeTotal, EdgePointsWithFalls, *FString::Join(Results, TEXT(",\n")));
	FFileHelper::SaveStringToFile(Json, *(Folder / TEXT("traversal-report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void UCalmyTraversalProbe::Tick(float DT)
{
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	const double Now = FPlatformTime::Seconds();
	if (!Start)
	{
		Start = Now;
		FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
		if (Dir.IsEmpty()) Dir = TEXT("ShipTraversal-UE-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
		FParse::Value(FCommandLine::Get(), TEXT("CalmyTraversalParallel="), Parallel);
		Parallel = FMath::Clamp(Parallel, 1, 32);
	}
	if (Now - Start > 1700) { WriteReport(); Finish(false, FString::Printf(TEXT("timeout jobs=%d/%d"), NextJob, Jobs.Num())); return; }
	auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC || !PC->GetPawn()) return;
	if (PC->IsPauseMenuOpen()) PC->ResumeFromMenu();
	StageAge += DT;
	if (Stage == 0)
	{
		if (Now - Start < 5) return;
		if (!ACalmyShipMotion::Find(GetWorld())) { Finish(false, TEXT("no_ship")); return; }
		if (!LoadRoutes()) { Finish(false, TEXT("routes_missing")); return; }
		// The player pawn stays parked at the bridge; every door is swung open for the walkers.
		if (auto* Move = Cast<ACharacter>(PC->GetPawn()) ? Cast<ACharacter>(PC->GetPawn())->GetCharacterMovement() : nullptr) Move->DisableMovement();
		int32 Doors = 0;
		for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It) { It->SetAutoTarget(true); ++Doors; }
		for (int32 R = 0; R < Routes.Num(); ++R) { Jobs.Add({R, false}); Jobs.Add({R, true}); }
		TSet<FIntVector> Seen;
		for (const FRoute& Route : Routes)
			for (const FVector& P : Route.Points)
			{
				const FIntVector Key(FMath::RoundToInt(P.X / 50), FMath::RoundToInt(P.Y / 50), FMath::RoundToInt(P.Z / 50));
				if (Seen.Contains(Key)) continue;
				Seen.Add(Key);
				FJob Edge; Edge.bEdge = true; Edge.EdgePoint = P; Jobs.Add(Edge);
			}
		UE_LOG(LogTemp, Display, TEXT("CALMY_TRAVERSAL START routes=%d walks=%d edge_points=%d doors_opened=%d parallel=%d"),
			Routes.Num(), Routes.Num() * 2, Jobs.Num() - Routes.Num() * 2, Doors, Parallel);
		GLog->Flush();
		Stage = 1; StageAge = 0;
		return;
	}
	if (Stage == 1)
	{
		if (StageAge < 9) return;
		Walkers.SetNum(Parallel);
		Stage = 2;
		return;
	}
	bool bBusy = false;
	for (FWalker& W : Walkers)
	{
		if (W.Job < 0 && NextJob < Jobs.Num()) StartJob(W);
		if (W.Job < 0) continue;
		bBusy = true;
		if (Jobs[W.Job].bEdge) StepEdge(W, DT); else StepWalk(W, DT);
	}
	if (!bBusy && NextJob >= Jobs.Num())
	{
		WriteReport();
		Finish(true, FString::Printf(TEXT("walks=%d/%d edge_points_with_falls=%d/%d report=Saved/Verification/%s/traversal-report.json"),
			WalkPassed, WalkTotal, EdgePointsWithFalls, EdgeTotal, *Dir));
	}
}
