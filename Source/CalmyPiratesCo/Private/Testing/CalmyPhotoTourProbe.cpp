#include "Testing/CalmyPhotoTourProbe.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

bool UCalmyPhotoTourProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value; const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyPhotoTour="), Value);
#endif
}

void UCalmyPhotoTourProbe::Finish(bool bOK, const FString& Detail)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_PHOTO_TOUR %s %s"), bOK ? TEXT("DONE") : TEXT("FAIL"), *Detail);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, bOK ? 0 : 1);
}

bool UCalmyPhotoTourProbe::LoadShots()
{
	if (Source == TEXT("doors"))
	{
		TArray<ACalmyDoor*> Doors;
		for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It) if (It->DoorId < 1000) Doors.Add(*It);
		Doors.Sort([](const ACalmyDoor& A, const ACalmyDoor& B) { return A.DoorId < B.DoorId; });
		const ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
		auto Local = [Ship](const FVector& W) { return Ship ? Ship->GetRestTransform().TransformPosition(Ship->GetActorTransform().InverseTransformPosition(W)) : W; };
		for (int32 Pass = 0; Pass < 2; ++Pass)
			for (ACalmyDoor* D : Doors)
			{
				const FTransform T = D->GetActorTransform();
				const FVector Mid = Local(T.TransformPosition(FVector(-D->WidthCm * .5f, 0, 105)));
				for (int32 Side = 0; Side < (Pass == 0 ? 2 : 1); ++Side)
				{
					FShot S;
					S.Name = FString::Printf(TEXT("door_%02d_%s_%s"), D->DoorId, Pass == 0 ? TEXT("closed") : TEXT("open"), Side ? TEXT("b") : TEXT("a"));
					// Far enough back to see the whole frame, sill and head joints against the bulkhead, but never inside the
					// opposite wall of a narrow corridor: back off only until 35 cm short of the first thing behind the eye.
					const FVector From = T.TransformPosition(FVector(-D->WidthCm * .5f, Side ? 30.f : -30.f, 150));
					const FVector Far = T.TransformPosition(FVector(-D->WidthCm * .5f, Side ? 280.f : -280.f, 150));
					FHitResult Behind; FCollisionQueryParams Q(SCENE_QUERY_STAT(PhotoTourBack), true, D);
					float Back = 280.f;
					if (GetWorld()->LineTraceSingleByChannel(Behind, From, Far, ECC_Visibility, Q)) Back = FMath::Clamp(30.f + Behind.Distance - 35.f, 90.f, 280.f);
					S.Eye = Local(T.TransformPosition(FVector(-D->WidthCm * .5f, Side ? Back : -Back, 150)));
					S.Target = Mid; S.Fov = 62; S.Doors = Pass;
					Shots.Add(S);
				}
			}
		return Shots.Num() > 0;
	}
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Source)) return false;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Root->TryGetArrayField(TEXT("shots"), List)) return false;
	auto Vec = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Key)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O->TryGetArrayField(Key, A) && A->Num() == 3 ? FVector((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber()) : FVector::ZeroVector;
	};
	for (const TSharedPtr<FJsonValue>& V : *List)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O.IsValid()) continue;
		FShot S; S.Name = O->GetStringField(TEXT("name")); S.Eye = Vec(O, TEXT("eye")); S.Target = Vec(O, TEXT("target"));
		double Fov = 70; O->TryGetNumberField(TEXT("fov"), Fov); S.Fov = float(Fov);
		FString DoorsMode; O->TryGetStringField(TEXT("doors"), DoorsMode);
		S.Doors = DoorsMode == TEXT("open") ? 1 : DoorsMode == TEXT("closed") ? 0 : -1;
		Shots.Add(S);
	}
	return Shots.Num() > 0;
}

void UCalmyPhotoTourProbe::Tick(float DT)
{
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	const double Now = FPlatformTime::Seconds();
	if (!Start)
	{
		Start = Now;
		FParse::Value(FCommandLine::Get(), TEXT("CalmyPhotoTour="), Source);
		FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
		if (Dir.IsEmpty()) Dir = TEXT("PhotoTour-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
	}
	if (Now - Start > 1200) { Finish(false, FString::Printf(TEXT("timeout shots=%d/%d"), Taken, Shots.Num())); return; }
	auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC || !PC->GetPawn()) return;
	if (PC->IsPauseMenuOpen()) PC->ResumeFromMenu();
	if (auto* Body = Cast<ACharacter>(PC->GetPawn())) { Body->GetCharacterMovement()->DisableMovement(); Body->SetActorHiddenInGame(true); }
	if (PC->MyHUD) PC->MyHUD->bShowHUD = false; // geometry review: no prompts, arrow or FPS over the frame
	if (Stage == 0)
	{
		if (Now - Start < 6) return;
		if (!LoadShots()) { Finish(false, TEXT("no_shots source=") + Source); return; }
		auto* Cam = GetWorld()->SpawnActor<ACameraActor>();
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		Camera = Cam;
		UE_LOG(LogTemp, Display, TEXT("CALMY_PHOTO_TOUR START shots=%d source=%s"), Shots.Num(), *Source);
		Stage = 1; Index = 0; Since = Now;
		return;
	}
	if (Index >= Shots.Num()) { Finish(true, FString::Printf(TEXT("shots=%d dir=Saved/Verification/%s"), Taken, *Dir)); return; }
	const FShot& S = Shots[Index];
	if (Stage == 1)
	{
		// Door state changes need the leaves to finish swinging before the frame.
		double Wait = .7;
		if (S.Doors >= 0 && S.Doors != DoorState)
		{
			for (TActorIterator<ACalmyDoor> It(GetWorld()); It; ++It) It->SetAutoTarget(S.Doors == 1);
			DoorState = S.Doors; Wait = 5.0;
		}
		ACalmyShipMotion* Ship = ACalmyShipMotion::Find(GetWorld());
		ACameraActor* Cam = Camera.Get();
		if (!Cam) { Finish(false, TEXT("camera_lost")); return; }
		const FTransform Frame = Ship ? Ship->GetRestTransform().Inverse() * Ship->GetActorTransform() : FTransform::Identity;
		Cam->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Cam->SetActorTransform(FTransform((S.Target - S.Eye).Rotation(), S.Eye) * Frame);
		if (Ship) Cam->AttachToActor(Ship, FAttachmentTransformRules::KeepWorldTransform);
		Cam->GetCameraComponent()->SetFieldOfView(S.Fov);
		PC->SetViewTarget(Cam);
		Since = Now + Wait - .7;
		Stage = 2;
		return;
	}
	if (Stage == 2 && Now - Since > .7)
	{
		const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Verification") / FPaths::GetCleanFilename(Dir));
		IFileManager::Get().MakeDirectory(*Folder, true);
		FScreenshotRequest::RequestScreenshot(Folder / (S.Name + TEXT(".png")), false, false);
		++Taken; Since = Now; Stage = 3;
		return;
	}
	if (Stage == 3 && Now - Since > .35) { ++Index; Stage = 1; }
}
