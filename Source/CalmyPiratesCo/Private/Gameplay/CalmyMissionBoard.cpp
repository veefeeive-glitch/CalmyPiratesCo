#include "Gameplay/CalmyMissionBoard.h"
#include "Gameplay/CalmyCharacter.h"
#include "Core/CalmySession.h"
#include "Core/CalmyAudio.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACalmyMissionBoard::ACalmyMissionBoard()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	UseZone = CreateDefaultSubobject<UBoxComponent>(TEXT("UseZone"));
	UseZone->SetupAttachment(RootComponent);
	UseZone->SetBoxExtent(FVector(180.f, 180.f, 140.f));
	UseZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Pedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pedestal"));
	Pedestal->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Pedestal->SetStaticMesh(Cube.Object);
		Pedestal->SetRelativeScale3D(FVector(1.2f, .4f, 1.6f));
		Pedestal->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
		Pedestal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Options = FCalmyVoyageContractStore::Catalogue();
}

ACalmyMissionBoard* ACalmyMissionBoard::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyMissionBoard> It(World); It; ++It) return *It;
	return nullptr;
}

bool ACalmyMissionBoard::IsPortMap(const UWorld* World)
{
	if (!World) return false;
	if (World->GetMapName().Contains(TEXT("PortHub"))) return true;
	if (World->URL.Map.Contains(TEXT("PortHub"))) return true;
	return World->GetName().Contains(TEXT("PortHub"));
}

void ACalmyMissionBoard::BeginPlay()
{
	Super::BeginPlay();
	Options = FCalmyVoyageContractStore::Catalogue();
	SelectedIndex = FMath::Clamp(SelectedIndex, 0, Options.Num() - 1);
}

void ACalmyMissionBoard::Tick(float)
{
	// Authority-only travel is driven by TryAccept; tick kept for future board FX.
}

void ACalmyMissionBoard::OnRep_Selected()
{
}

FString ACalmyMissionBoard::PromptText() const
{
	if (!Options.IsValidIndex(SelectedIndex)) return TEXT("MISSION BOARD");
	const FCalmyVoyageContract& C = Options[SelectedIndex];
	return FString::Printf(
		TEXT("MISSION BOARD | %s | ~%d min | crates S%d B%d F%d U%d | Arrows select · E accept (host)"),
		*C.DisplayName, FMath::Max(1, C.WatchDurationSeconds / 60),
		C.RequiredStandard, C.RequiredBulky, C.RequiredFragile, C.RequiredUnstable);
}

bool ACalmyMissionBoard::TryStep(ACalmyCharacter* Requester, int32 Direction)
{
	const auto* PC = IsValid(Requester) ? Cast<APlayerController>(Requester->GetController()) : nullptr;
	if (!HasAuthority() || !PC || !PC->IsLocalController() || Options.Num() <= 0) return false;
	const bool Near = (UseZone && UseZone->IsOverlappingActor(Requester)) ||
		FVector::DistSquared(Requester->GetActorLocation(), GetActorLocation()) < FMath::Square(280.f);
	if (!Near) return false;
	SelectedIndex = (SelectedIndex + Direction + Options.Num()) % Options.Num();
	FCalmyAudio::Play2D(this, ECalmySoundCue::UiSelect);
	ForceNetUpdate();
	return true;
}

bool ACalmyMissionBoard::TryAccept(ACalmyCharacter* Requester)
{
	const auto* PC = IsValid(Requester) ? Cast<APlayerController>(Requester->GetController()) : nullptr;
	// One travel per accept: repeated requests while ServerTravel is pending are ignored.
	if (!HasAuthority() || bTravelRequested || !PC || !PC->IsLocalController() || !Options.IsValidIndex(SelectedIndex)) return false;
	const bool Near = (UseZone && UseZone->IsOverlappingActor(Requester)) ||
		FVector::DistSquared(Requester->GetActorLocation(), GetActorLocation()) < FMath::Square(280.f);
	if (!Near) return false;
	if (!FCalmySession::CanRequestSessionTravel(GetWorld()->GetNetMode(), true)) return false;
	if (!FCalmySession::IsAllowlistedVoyageTravel(GetWorld()->GetMapName(), ShipMapPath())) return false;
	const FCalmyVoyageContract Contract = Options[SelectedIndex];
	FCalmyVoyageContractStore::SaveAccepted(Contract);
	AcceptedContractId = Contract.ContractId;
	FCalmyAudio::Play2D(this, ECalmySoundCue::PortAccept);
	ForceNetUpdate();
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client) return false;
	FCalmyAudio::EnsureWorldAmbience(World);
	bTravelRequested = true;
	if (!FPackageName::DoesPackageExist(ShipMapPath()) || !World->ServerTravel(ShipMapPath(), false))
	{
		bTravelRequested = false;
		UE_LOG(LogTemp, Error, TEXT("CALMY_PORT accept_travel_failed contract=%s"), *Contract.ContractId);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CALMY_PORT accept_travel contract=%s -> %s"), *Contract.ContractId, ShipMapPath());
	return true;
}

void ACalmyMissionBoard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACalmyMissionBoard, SelectedIndex);
	DOREPLIFETIME(ACalmyMissionBoard, AcceptedContractId);
}
