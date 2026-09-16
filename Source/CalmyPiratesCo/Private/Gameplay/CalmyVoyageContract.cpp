#include "Gameplay/CalmyVoyageContract.h"
#include "Misc/ConfigCacheIni.h"

namespace CalmyContractPrivate
{
	static const TCHAR* Section = TEXT("Calmy.VoyageContract");
}

FCalmyVoyageContract FCalmyVoyageContractStore::ShortWatch()
{
	FCalmyVoyageContract C;
	C.ContractId = TEXT("short_watch");
	C.DisplayName = TEXT("Short Coastal Run");
	C.WatchDurationSeconds = 480;
	C.RequiredStandard = 1;
	C.RequiredBulky = 0;
	C.RequiredFragile = 0;
	C.RequiredUnstable = 0;
	C.bStorm = false;
	C.bLeak = false;
	C.bFuel = true;
	return C;
}

FCalmyVoyageContract FCalmyVoyageContractStore::StandardG3b()
{
	FCalmyVoyageContract C;
	C.ContractId = TEXT("standard_g3b");
	C.DisplayName = TEXT("Standard Night Watch");
	C.WatchDurationSeconds = 1080;
	C.RequiredStandard = 1;
	C.RequiredBulky = 1;
	C.RequiredFragile = 1;
	C.RequiredUnstable = 1;
	C.bStorm = true;
	C.bLeak = true;
	C.bFuel = true;
	return C;
}

FCalmyVoyageContract FCalmyVoyageContractStore::HeavyWatch()
{
	FCalmyVoyageContract C;
	C.ContractId = TEXT("heavy_watch");
	C.DisplayName = TEXT("Heavy Storm Contract");
	C.WatchDurationSeconds = 1440;
	C.RequiredStandard = 1;
	C.RequiredBulky = 1;
	C.RequiredFragile = 1;
	C.RequiredUnstable = 1;
	C.bStorm = true;
	C.bLeak = true;
	C.bFuel = true;
	return C;
}

TArray<FCalmyVoyageContract> FCalmyVoyageContractStore::Catalogue()
{
	return {ShortWatch(), StandardG3b(), HeavyWatch()};
}

FCalmyVoyageContract FCalmyVoyageContractStore::ById(const FString& ContractId)
{
	for (const FCalmyVoyageContract& C : Catalogue())
	{
		if (C.ContractId.Equals(ContractId, ESearchCase::IgnoreCase)) return C;
	}
	return StandardG3b();
}

void FCalmyVoyageContractStore::SaveAccepted(const FCalmyVoyageContract& Contract)
{
	GConfig->SetString(CalmyContractPrivate::Section, TEXT("ContractId"), *Contract.ContractId, GGameUserSettingsIni);
	GConfig->SetString(CalmyContractPrivate::Section, TEXT("DisplayName"), *Contract.DisplayName, GGameUserSettingsIni);
	GConfig->SetInt(CalmyContractPrivate::Section, TEXT("WatchDurationSeconds"), Contract.WatchDurationSeconds, GGameUserSettingsIni);
	GConfig->SetInt(CalmyContractPrivate::Section, TEXT("RequiredStandard"), Contract.RequiredStandard, GGameUserSettingsIni);
	GConfig->SetInt(CalmyContractPrivate::Section, TEXT("RequiredBulky"), Contract.RequiredBulky, GGameUserSettingsIni);
	GConfig->SetInt(CalmyContractPrivate::Section, TEXT("RequiredFragile"), Contract.RequiredFragile, GGameUserSettingsIni);
	GConfig->SetInt(CalmyContractPrivate::Section, TEXT("RequiredUnstable"), Contract.RequiredUnstable, GGameUserSettingsIni);
	GConfig->SetBool(CalmyContractPrivate::Section, TEXT("Storm"), Contract.bStorm, GGameUserSettingsIni);
	GConfig->SetBool(CalmyContractPrivate::Section, TEXT("Leak"), Contract.bLeak, GGameUserSettingsIni);
	GConfig->SetBool(CalmyContractPrivate::Section, TEXT("Fuel"), Contract.bFuel, GGameUserSettingsIni);
	GConfig->SetBool(CalmyContractPrivate::Section, TEXT("Accepted"), true, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

FCalmyVoyageContract FCalmyVoyageContractStore::LoadAccepted()
{
	FString Id;
	if (!GConfig->GetString(CalmyContractPrivate::Section, TEXT("ContractId"), Id, GGameUserSettingsIni) || Id.IsEmpty())
	{
		return StandardG3b();
	}
	// Only the id is read back. Terms (cargo, hazards, duration) always come from the shipped catalogue, so an edited
	// ini cannot empty the manifest or strip hazards; the saved copies are informational.
	FCalmyVoyageContract C = ById(Id);
	C.WatchDurationSeconds = FMath::Clamp(C.WatchDurationSeconds, 300, 1800);
	return C;
}

void FCalmyVoyageContractStore::ClearAccepted()
{
	GConfig->SetBool(CalmyContractPrivate::Section, TEXT("Accepted"), false, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

bool FCalmyVoyageContractStore::HasAccepted()
{
	bool Accepted = false;
	return GConfig->GetBool(CalmyContractPrivate::Section, TEXT("Accepted"), Accepted, GGameUserSettingsIni) && Accepted;
}
