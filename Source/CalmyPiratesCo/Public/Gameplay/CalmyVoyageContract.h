#pragma once
#include "CoreMinimal.h"
#include "CalmyVoyageContract.generated.h"

/** Host-selected voyage brief that survives PortHub → OceanFreighterR7 travel. */
USTRUCT(BlueprintType)
struct FCalmyVoyageContract
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString ContractId = TEXT("standard_g3b");
	UPROPERTY(BlueprintReadOnly) FString DisplayName = TEXT("Standard Night Watch");
	UPROPERTY(BlueprintReadOnly) int32 WatchDurationSeconds = 1080;
	UPROPERTY(BlueprintReadOnly) int32 RequiredStandard = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredBulky = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredFragile = 1;
	UPROPERTY(BlueprintReadOnly) int32 RequiredUnstable = 1;
	UPROPERTY(BlueprintReadOnly) bool bStorm = true;
	UPROPERTY(BlueprintReadOnly) bool bLeak = true;
	UPROPERTY(BlueprintReadOnly) bool bFuel = true;
};

/** GConfig slot so ServerTravel keeps the accepted mission without URL options. */
struct CALMYPIRATESCO_API FCalmyVoyageContractStore
{
	static FCalmyVoyageContract ShortWatch();
	static FCalmyVoyageContract StandardG3b();
	static FCalmyVoyageContract HeavyWatch();
	static TArray<FCalmyVoyageContract> Catalogue();
	static FCalmyVoyageContract ById(const FString& ContractId);
	static void SaveAccepted(const FCalmyVoyageContract& Contract);
	static FCalmyVoyageContract LoadAccepted();
	static void ClearAccepted();
	static bool HasAccepted();
};
