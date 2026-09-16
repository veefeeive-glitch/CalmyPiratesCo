#include "Testing/CalmyPlayabilityProbe.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyFuelStation.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/FlySwarmMob.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace CalmyPlayability
{
	// Ship-local survey volume (UE cm) and grid.
	constexpr float Step = 25.f;
	// Half-step origin: R7 bulkheads, deck edges and door planes sit on whole 25 cm lines, and a column running exactly
	// through a 16 cm bulkhead plate finds no floor inside the doorway, cutting the link through every transverse door.
	constexpr float MinX = -6287.5f, MaxX = 6100.f, MinY = -1262.5f, MaxY = 1275.f, TopZ = 2900.f, BottomZ = 100.f;
	// Player capsule is 34 x 88 (176 cm tall); the survey capsule is lifted by the step height so stair nosings do not
	// read as walls, and its top stays at the player's 176 cm so low tank-top doorways are judged like the real player.
	constexpr float Radius = 32.f, StepUp = 45.f, HalfHeight = 65.f;
	constexpr float WalkableNormalZ = .643f; // 50 degrees
	constexpr float LinkDZ = 50.f, DropCm = 120.f;
	constexpr double SliceSeconds = .045;
	static const int32 DX[8] = {1, 0, 1, -1, -1, 0, -1, 1};
	static const int32 DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	FString Vec(const FVector& V) { return FString::Printf(TEXT("[%.0f,%.0f,%.0f]"), V.X, V.Y, V.Z); }
}

bool UCalmyPlayabilityProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value; const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyPlayabilityProbe="), Value);
#endif
}

void UCalmyPlayabilityProbe::Finish(bool bOK, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY %s %s"), bOK ? TEXT("DONE") : TEXT("FAIL"), *Detail);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bOK ? 0 : 1);
}

FVector UCalmyPlayabilityProbe::ToWorld(const FVector& Local) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetActorTransform().TransformPosition(Ship->GetRestTransform().InverseTransformPosition(Local)) : Local;
}

FVector UCalmyPlayabilityProbe::ToLocal(const FVector& World) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetRestTransform().TransformPosition(Ship->GetActorTransform().InverseTransformPosition(World)) : World;
}

FVector UCalmyPlayabilityProbe::LocalDirToWorld(const FVector& Direction) const
{
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	return Ship ? Ship->GetActorTransform().TransformVectorNoScale(Ship->GetRestTransform().InverseTransformVectorNoScale(Direction)) : Direction;
}

bool UCalmyPlayabilityProbe::CapsuleClear(const FVector& FloorLocal, uint8 Variant) const
{
	using namespace CalmyPlayability;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(PlayabilityStand), true);
	for (const auto& A : Dynamic) if (A.IsValid()) Q.AddIgnoredActor(A.Get());
	if (Variant == 1 && Guardrails.IsValid()) Q.AddIgnoredComponent(Guardrails.Get());
	if (Variant == 2 && Details.IsValid()) Q.AddIgnoredComponent(Details.Get());
	const FVector Up = LocalDirToWorld(FVector::UpVector);
	const FQuat Rot = FQuat::FindBetweenNormals(FVector::UpVector, Up);
	return !GetWorld()->OverlapBlockingTestByChannel(ToWorld(FloorLocal) + Up * (StepUp + HalfHeight + 1), Rot, ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius, HalfHeight), Q);
}

bool UCalmyPlayabilityProbe::SweepClear(const FVector& FromFloorLocal, const FVector& ToFloorLocal, uint8 Variant) const
{
	using namespace CalmyPlayability;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(PlayabilityLink), true);
	for (const auto& A : Dynamic) if (A.IsValid()) Q.AddIgnoredActor(A.Get());
	if (Variant == 1 && Guardrails.IsValid()) Q.AddIgnoredComponent(Guardrails.Get());
	if (Variant == 2 && Details.IsValid()) Q.AddIgnoredComponent(Details.Get());
	const FVector Up = LocalDirToWorld(FVector::UpVector);
	const FQuat Rot = FQuat::FindBetweenNormals(FVector::UpVector, Up);
	const float Z = FMath::Max(FromFloorLocal.Z, ToFloorLocal.Z) + StepUp + HalfHeight + 1;
	const FVector A = ToWorld(FVector(FromFloorLocal.X, FromFloorLocal.Y, Z));
	const FVector B = ToWorld(FVector(ToFloorLocal.X, ToFloorLocal.Y, Z));
	return !GetWorld()->SweepTestByChannel(A, B, Rot, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Q);
}

void UCalmyPlayabilityProbe::SampleColumn(int32 IX, int32 IY)
{
	using namespace CalmyPlayability;
	const float LX = MinX + IX * Step, LY = MinY + IY * Step;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(PlayabilityFloor), true);
	for (const auto& A : Dynamic) if (A.IsValid()) Q.AddIgnoredActor(A.Get());
	const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
	float CurZ = TopZ;
	for (int32 N = 0; N < 24 && CurZ > BottomZ; ++N)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, ToWorld(FVector(LX, LY, CurZ)), ToWorld(FVector(LX, LY, BottomZ)), ECC_Pawn, Q)) break;
		FVector L = ToLocal(Hit.ImpactPoint);
		FVector NL = Ship ? Ship->GetRestTransform().TransformVectorNoScale(Ship->GetActorTransform().InverseTransformVectorNoScale(Hit.ImpactNormal)) : Hit.ImpactNormal;
		CurZ = L.Z - 4.f;
		if (NL.Z < WalkableNormalZ)
		{
			// A column that lands exactly on a stair nosing or deck edge reports the vertical face: look 4 cm aside for
			// the tread/deck top at this height, otherwise every sixth column of a 30 cm-tread stair drops out.
			bool bFound = false;
			static const FVector2D Aside[] = {FVector2D(4, 0), FVector2D(-4, 0), FVector2D(0, 4), FVector2D(0, -4)};
			for (const FVector2D& O : Aside)
			{
				FHitResult Near;
				if (!GetWorld()->LineTraceSingleByChannel(Near, ToWorld(FVector(LX + O.X, LY + O.Y, L.Z + 30)), ToWorld(FVector(LX + O.X, LY + O.Y, L.Z - 30)), ECC_Pawn, Q)) continue;
				const FVector NearL = ToLocal(Near.ImpactPoint);
				const FVector NearN = Ship ? Ship->GetRestTransform().TransformVectorNoScale(Ship->GetActorTransform().InverseTransformVectorNoScale(Near.ImpactNormal)) : Near.ImpactNormal;
				if (NearN.Z >= WalkableNormalZ && NearL.Z >= L.Z - 30) { L.Z = NearL.Z; NL = NearN; bFound = true; break; }
			}
			if (!bFound) continue;
			CurZ = FMath::Min(CurZ, L.Z - 4.f);
		}
		const FVector Floor(LX, LY, L.Z);
		FCell Cell; Cell.IX = IX; Cell.IY = IY; Cell.Z = L.Z;
		if (CapsuleClear(Floor, 0)) Cell.Stand = 1 | 2 | 4;
		else
		{
			if (Guardrails.IsValid() && CapsuleClear(Floor, 1)) Cell.Stand |= 2;
			if (Details.IsValid() && CapsuleClear(Floor, 2)) Cell.Stand |= 4;
		}
		if (!Cell.Stand) continue;
		Columns.FindOrAdd(IX * NY + IY).Add(Cells.Num());
		Cells.Add(Cell);
	}
}

int32 UCalmyPlayabilityProbe::NeighbourCell(const FCell& Cell, int32 DXv, int32 DYv) const
{
	using namespace CalmyPlayability;
	const int32 X = Cell.IX + DXv, Y = Cell.IY + DYv;
	if (X < 0 || Y < 0 || X >= NX || Y >= NY) return INDEX_NONE;
	const TArray<int32>* Column = Columns.Find(X * NY + Y);
	if (!Column) return INDEX_NONE;
	int32 Best = INDEX_NONE; float BestDZ = LinkDZ;
	for (int32 I : *Column) { const float D = FMath::Abs(Cells[I].Z - Cell.Z); if (D <= BestDZ) { BestDZ = D; Best = I; } }
	return Best;
}

void UCalmyPlayabilityProbe::LinkCell(int32 Index)
{
	using namespace CalmyPlayability;
	FCell& Cell = Cells[Index];
	const FVector From(MinX + Cell.IX * Step, MinY + Cell.IY * Step, Cell.Z);
	for (int32 D = 0; D < 8; ++D)
	{
		const int32 N = NeighbourCell(Cell, DX[D], DY[D]);
		if (N != INDEX_NONE)
		{
			if (D >= 4) continue; // links are symmetric: the analyzer mirrors the forward half
			const FCell& Other = Cells[N];
			const FVector To(MinX + Other.IX * Step, MinY + Other.IY * Step, Other.Z);
			if (SweepClear(From, To, 0)) { for (uint8& P : Cell.Pass) P |= 1 << D; continue; }
			if (Guardrails.IsValid() && SweepClear(From, To, 1)) Cell.Pass[1] |= 1 << D;
			if (Details.IsValid() && SweepClear(From, To, 2)) Cell.Pass[2] |= 1 << D;
			continue;
		}
		if (!(Cell.Stand & 1)) continue;
		// No floor within a step that way: can the player walk off, and how far is the fall?
		const FVector Off(From.X + DX[D] * Step, From.Y + DY[D] * Step, Cell.Z);
		if (!SweepClear(From, Off, 0)) continue;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(PlayabilityDrop), true);
		for (const auto& A : Dynamic) if (A.IsValid()) Q.AddIgnoredActor(A.Get());
		FHitResult Hit;
		const float Below = GetWorld()->LineTraceSingleByChannel(Hit, ToWorld(Off + FVector(0, 0, 10)), ToWorld(Off - FVector(0, 0, 3000)), ECC_Pawn, Q)
			? float(Cell.Z - ToLocal(Hit.ImpactPoint).Z) : 3000.f;
		if (Below <= DropCm) continue;
		// A slot narrower than the player (walkway-to-shell gaps, grating seams) is not a fall: the capsule must fit
		// in the hole a step's height below the deck it left.
		FCollisionQueryParams Fit(SCENE_QUERY_STAT(PlayabilityDropFit), true);
		for (const auto& A : Dynamic) if (A.IsValid()) Fit.AddIgnoredActor(A.Get());
		const FVector Up = LocalDirToWorld(FVector::UpVector);
		// Test volume wholly below the deck plate (top 25 cm under the floor) and 45 cm out, so the plate edge itself
		// never counts as "too narrow" while a hull-to-walkway slot still does.
		const FVector Hole(From.X + DX[D] * (Step + 20), From.Y + DY[D] * (Step + 20), Cell.Z - 25 - HalfHeight);
		if (GetWorld()->OverlapBlockingTestByChannel(ToWorld(Hole), FQuat::FindBetweenNormals(FVector::UpVector, Up), ECC_Pawn,
			FCollisionShape::MakeCapsule(Radius, HalfHeight), Fit)) continue;
		Cell.Drop |= 1 << D;
	}
}

void UCalmyPlayabilityProbe::AddTarget(const FString& Kind, const FString& Name, const FVector& Local, float RadiusCm, bool bNeedSight, const AActor* Ignore)
{
	using namespace CalmyPlayability;
	TArray<FString> Use;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(PlayabilitySight), true);
	for (const auto& A : Dynamic) if (A.IsValid()) Q.AddIgnoredActor(A.Get());
	if (Ignore) Q.AddIgnoredActor(Ignore);
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		const FCell& C = Cells[I];
		if (!(C.Stand & 1)) continue;
		const FVector Eye(MinX + C.IX * Step, MinY + C.IY * Step, C.Z + 88.f);
		if (FVector::DistSquared(Eye, Local) > FMath::Square(RadiusCm)) continue;
		FHitResult Hit;
		if (bNeedSight && GetWorld()->LineTraceSingleByChannel(Hit, ToWorld(Eye), ToWorld(Local), ECC_Visibility, Q)) continue;
		Use.Add(FString::FromInt(I));
	}
	Targets.Add(FString::Printf(TEXT("{\"kind\":\"%s\",\"name\":\"%s\",\"local\":%s,\"radius\":%.0f,\"sight\":%s,\"use_cells\":[%s]}"),
		*Kind, *Name, *Vec(Local), RadiusCm, bNeedSight ? TEXT("true") : TEXT("false"), *FString::Join(Use, TEXT(","))));
}

void UCalmyPlayabilityProbe::CollectTargets()
{
	using namespace CalmyPlayability;
	UWorld* World = GetWorld();
	for (TActorIterator<APlayerStart> It(World); It; ++It)
		AddTarget(TEXT("spawn"), It->GetName(), ToLocal(It->GetActorLocation()) - FVector(0, 0, 90), 60, false, nullptr);
	for (TActorIterator<ACalmyCargo> It(World); It; ++It)
		AddTarget(TEXT("cargo"), FString::Printf(TEXT("cargo_%d"), It->CargoId), ToLocal(It->GetActorLocation()), 220, true, *It);
	for (TActorIterator<ACalmyShift> It(World); It; ++It)
	{
		// Delivery: cells whose floor lies inside the zone box (the cargo is set down there).
		const FTransform Zone = It->DeliveryZone->GetComponentTransform();
		const FVector Extent = It->DeliveryZone->GetUnscaledBoxExtent();
		TArray<FString> Use;
		for (int32 I = 0; I < Cells.Num(); ++I)
		{
			const FCell& C = Cells[I];
			if (!(C.Stand & 1)) continue;
			const FVector P = Zone.InverseTransformPosition(ToWorld(FVector(MinX + C.IX * Step, MinY + C.IY * Step, C.Z + 30)));
			if (FMath::Abs(P.X) <= Extent.X && FMath::Abs(P.Y) <= Extent.Y && FMath::Abs(P.Z) <= Extent.Z) Use.Add(FString::FromInt(I));
		}
		Targets.Add(FString::Printf(TEXT("{\"kind\":\"delivery\",\"name\":\"%s\",\"local\":%s,\"radius\":%.0f,\"sight\":false,\"use_cells\":[%s]}"),
			*It->GetName(), *Vec(ToLocal(It->GetActorLocation())), float(Extent.Size2D()), *FString::Join(Use, TEXT(","))));
	}
	for (TActorIterator<ACalmyLeak> It(World); It; ++It) AddTarget(TEXT("leak_valve"), It->GetName(), ToLocal(It->GetActorLocation()), 180, true, *It);
	for (TActorIterator<ACalmyFuelStation> It(World); It; ++It) AddTarget(TEXT("fuel_pump"), It->GetName(), ToLocal(It->GetActorLocation()), 220, false, *It);
	for (TActorIterator<ACalmyCrewHelper> It(World); It; ++It) AddTarget(TEXT("crew_helper"), It->GetName(), ToLocal(It->GetActorLocation()), 220, true, *It);
	for (TActorIterator<ACalmyBridgeTerminal> It(World); It; ++It) AddTarget(TEXT("bridge_terminal"), It->GetName(), ToLocal(It->GetActorLocation()), 250, true, *It);
	for (TActorIterator<ACalmyDoor> It(World); It; ++It)
	{
		// Both faces of the doorway, 80 cm out from the leaf plane at mid-width.
		const FTransform T = It->GetActorTransform();
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector P = ToLocal(T.TransformPosition(FVector(-It->WidthCm * .5f, Side ? 80.f : -80.f, 0)));
			AddTarget(TEXT("door"), FString::Printf(TEXT("door_%d_%s"), It->DoorId, Side ? TEXT("b") : TEXT("a")), P + FVector(0, 0, 88), 55, false, nullptr);
		}
	}
}

void UCalmyPlayabilityProbe::WriteOutputs()
{
	using namespace CalmyPlayability;
	const FString Folder = FPaths::ProjectDir() / TEXT("Saved/Verification") / FPaths::GetCleanFilename(Dir);
	IFileManager::Get().MakeDirectory(*Folder, true);
	FString Csv = TEXT("id,ix,iy,x,y,z,stand,pass0,pass1,pass2,drop\n");
	Csv.Reserve(Cells.Num() * 48);
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		const FCell& C = Cells[I];
		Csv += FString::Printf(TEXT("%d,%d,%d,%.0f,%.0f,%.1f,%d,%d,%d,%d,%d\n"), I, C.IX, C.IY, MinX + C.IX * Step, MinY + C.IY * Step, C.Z,
			C.Stand, C.Pass[0], C.Pass[1], C.Pass[2], C.Drop);
	}
	FFileHelper::SaveStringToFile(Csv, *(Folder / TEXT("cells.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const FString Json = FString::Printf(TEXT("{\n\"map\":\"%s\",\n\"grid\":{\"step\":%.0f,\"min_x\":%.0f,\"min_y\":%.0f,\"nx\":%d,\"ny\":%d},\n\"capsule\":{\"radius\":%.0f,\"step_up\":%.0f,\"half_height\":%.0f},\n\"doors_opened\":%d,\n\"guardrails\":%s,\n\"details\":%s,\n\"targets\":[\n%s\n]\n}\n"),
		*GetWorld()->GetMapName(), Step, MinX, MinY, NX, NY, Radius, StepUp, HalfHeight, Doors, Guardrails.IsValid() ? TEXT("true") : TEXT("false"),
		Details.IsValid() ? TEXT("true") : TEXT("false"), *FString::Join(Targets, TEXT(",\n")));
	FFileHelper::SaveStringToFile(Json, *(Folder / TEXT("targets.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void UCalmyPlayabilityProbe::Tick(float DT)
{
	using namespace CalmyPlayability;
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	const double Now = FPlatformTime::Seconds();
	if (!Start)
	{
		Start = Now;
		FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
		if (Dir.IsEmpty()) Dir = TEXT("ShipPlayability-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
		NX = FMath::FloorToInt((MaxX - MinX) / Step) + 1;
		NY = FMath::FloorToInt((MaxY - MinY) / Step) + 1;
	}
	if (Now - Start > 1500) { Finish(false, FString::Printf(TEXT("timeout stage=%d cursor=%d cells=%d"), Stage, Cursor, Cells.Num())); return; }
	if (auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (PC->IsPauseMenuOpen()) PC->ResumeFromMenu();
		if (auto* Body = Cast<ACharacter>(PC->GetPawn())) Body->GetCharacterMovement()->DisableMovement();
	}
	StageAge += DT;
	if (Stage == 0)
	{
		if (Now - Start < 5) return;
		if (!ACalmyShipMotion::Find(GetWorld())) { Finish(false, TEXT("no_ship")); return; }
		for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It) { It->SetAutoTarget(true); ++Doors; }
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			const UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
			const FString Asset = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : FString();
			if (Asset.StartsWith(TEXT("SM_R7_81_GUARDRAILS"))) Guardrails = It->GetStaticMeshComponent();
			if (Asset.StartsWith(TEXT("SM_R7_82_DETAILS"))) Details = It->GetStaticMeshComponent();
		}
		// Bare-deck survey for rebuilding S2 from scratch: the current guardrails are ignored by every query, so the drops
		// found are the real deck edges, including edges a misplaced rail currently hides or cuts off.
		if (FParse::Param(FCommandLine::Get(), TEXT("CalmyPlayabilityIgnoreRails")) && Guardrails.IsValid())
		{
			Dynamic.Add(Guardrails->GetOwner());
			Guardrails.Reset();
			UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY IGNORE_GUARDRAILS"));
		}
		UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY START grid=%dx%d doors_opened=%d guardrails=%d details=%d"), NX, NY, Doors, Guardrails.IsValid(), Details.IsValid());
		GLog->Flush();
		Stage = 1; StageAge = 0;
		return;
	}
	if (Stage == 1)
	{
		if (StageAge < 9) return;
		// Characters, cargo, the crew helper and flies move: they are not part of the ship's walkable layout.
		for (TActorIterator<ACharacter> It(GetWorld()); It; ++It) Dynamic.Add(*It);
		for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It) Dynamic.Add(*It);
		for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It) Dynamic.Add(*It);
		for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It)
			if (!It->IsOpen()) UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY DOOR_NOT_OPEN id=%d angle=%.1f"), It->DoorId, It->Angle);
		Stage = 2; Cursor = 0;
		return;
	}
	const double SliceEnd = FPlatformTime::Seconds() + SliceSeconds;
	if (Stage == 2)
	{
		while (Cursor < NX * NY && FPlatformTime::Seconds() < SliceEnd) { SampleColumn(Cursor / NY, Cursor % NY); ++Cursor; }
		if (Cursor >= NX * NY)
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY SAMPLED cells=%d seconds=%.0f"), Cells.Num(), Now - Start);
			GLog->Flush();
			Stage = 3; Cursor = 0;
		}
		return;
	}
	if (Stage == 3)
	{
		while (Cursor < Cells.Num() && FPlatformTime::Seconds() < SliceEnd) LinkCell(Cursor++);
		if (Cursor >= Cells.Num())
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_PLAYABILITY LINKED cells=%d seconds=%.0f"), Cells.Num(), Now - Start);
			GLog->Flush();
			Stage = 4;
		}
		return;
	}
	if (Stage == 4)
	{
		CollectTargets();
		WriteOutputs();
		Finish(true, FString::Printf(TEXT("cells=%d targets=%d report=Saved/Verification/%s"), Cells.Num(), Targets.Num(), *Dir));
	}
}
