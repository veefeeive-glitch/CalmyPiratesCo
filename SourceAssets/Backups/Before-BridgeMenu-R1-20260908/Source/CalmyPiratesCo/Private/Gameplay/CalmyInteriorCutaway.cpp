#include "Gameplay/CalmyInteriorCutaway.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

ACalmyInteriorCutaway::ACalmyInteriorCutaway()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .08f;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RoofMesh(
		TEXT("/Game/Art/FreighterInterior/Meshes/SM_InteriorCeiling4m/StaticMeshes/SM_InteriorCeiling4m.SM_InteriorCeiling4m"));
	for (int32 X = 0; X < 2; ++X)
	{
		for (int32 Y = 0; Y < 3; ++Y)
		{
			const int32 Index = X * 3 + Y;
			auto* Panel = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("RoofPanel_%d"), Index));
			Panel->SetupAttachment(SceneRoot);
			Panel->SetStaticMesh(RoofMesh.Object);
			Panel->SetRelativeLocation(FVector(X == 0 ? -225.f : 225.f, (Y - 1) * 465.f, 280.f));
			Panel->SetRelativeScale3D(FVector(1.125f, 1.17f, 1.f));
			Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Panel->SetCastShadow(true);
			RoofPanels.Add(Panel);
		}
	}
}

void ACalmyInteriorCutaway::BeginPlay()
{
	Super::BeginPlay();
	const FName OccluderTag(TEXT("CalmyLowerDeckOccluder"));
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(OccluderTag))
		{
			if (UStaticMeshComponent* Mesh = It->FindComponentByClass<UStaticMeshComponent>())
			{
				MainDeckOccluders.Add(Mesh);
			}
		}
	}
}

bool ACalmyInteriorCutaway::ShouldHideRoof(const FVector& LocalPawnLocation)
{
	const bool bInsideSuperstructure = FMath::Abs(LocalPawnLocation.X) <= 500.f && LocalPawnLocation.Z >= -100.f;
	const bool bInsideLowerDeck = LocalPawnLocation.X >= -500.f && LocalPawnLocation.X <= 2500.f && LocalPawnLocation.Z < -100.f;
	return (bInsideSuperstructure || bInsideLowerDeck) && FMath::Abs(LocalPawnLocation.Y) <= 735.f &&
		LocalPawnLocation.Z >= -400.f && LocalPawnLocation.Z <= 360.f;
}

bool ACalmyInteriorCutaway::ShouldHideMainDeck(const FVector& LocalPawnLocation)
{
	return LocalPawnLocation.X >= -500.f && LocalPawnLocation.X <= 2500.f &&
		FMath::Abs(LocalPawnLocation.Y) <= 735.f && LocalPawnLocation.Z >= -400.f && LocalPawnLocation.Z < -80.f;
}

void ACalmyInteriorCutaway::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector LocalLocation = LocalPawn ? GetActorTransform().InverseTransformPosition(LocalPawn->GetActorLocation()) : FVector::ZeroVector;
	const bool bHasLocalPawn = LocalPawn && LocalPawn->IsLocallyControlled();
	const bool bShouldHideRoof = bHasLocalPawn && ShouldHideRoof(LocalLocation);
	const bool bShouldHideMainDeck = bHasLocalPawn && ShouldHideMainDeck(LocalLocation);
	if (bShouldHideRoof != bRoofHidden)
	{
		bRoofHidden = bShouldHideRoof;
		for (const TObjectPtr<UStaticMeshComponent>& Panel : RoofPanels) if (Panel) Panel->SetVisibility(!bRoofHidden, true);
	}
	if (bShouldHideMainDeck != bMainDeckHidden)
	{
		bMainDeckHidden = bShouldHideMainDeck;
		for (const TWeakObjectPtr<UStaticMeshComponent>& Mesh : MainDeckOccluders) if (Mesh.IsValid()) Mesh->SetVisibility(!bMainDeckHidden, true);
	}
}
