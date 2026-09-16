#include "Gameplay/CalmyAmbientBeds.h"
#include "Gameplay/CalmyPreferences.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyMissionBoard.h"
#include "Gameplay/CalmyLeak.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ACalmyAmbientBeds::ACalmyAmbientBeds()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Noise.Initialize(508);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Wind = CreateDefaultSubobject<UAudioComponent>(TEXT("Wind"));
	Hull = CreateDefaultSubobject<UAudioComponent>(TEXT("Hull"));
	Sea = CreateDefaultSubobject<UAudioComponent>(TEXT("Sea"));
	Alarm = CreateDefaultSubobject<UAudioComponent>(TEXT("Alarm"));
	Wind->SetupAttachment(RootComponent);
	Hull->SetupAttachment(RootComponent);
	Sea->SetupAttachment(RootComponent);
	Alarm->SetupAttachment(RootComponent);
	for (UAudioComponent* Comp : {Wind.Get(), Hull.Get(), Sea.Get(), Alarm.Get()})
	{
		Comp->bAutoActivate = false;
		Comp->bIsUISound = false;
		Comp->ComponentTags.Add(TEXT("Ambience"));
	}
}

ACalmyAmbientBeds* ACalmyAmbientBeds::Find(const UWorld* World)
{
	if (World) for (TActorIterator<ACalmyAmbientBeds> It(World); It; ++It) return *It;
	return nullptr;
}

ACalmyAmbientBeds* ACalmyAmbientBeds::Ensure(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer || !World->AllowAudioPlayback()) return nullptr;
	if (ACalmyAmbientBeds* Existing = Find(World)) return Existing;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACalmyAmbientBeds* Beds = World->SpawnActor<ACalmyAmbientBeds>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
#if WITH_EDITOR
	if (Beds) Beds->SetActorLabel(TEXT("CalmyAmbientBeds"));
#endif
	return Beds;
}

void ACalmyAmbientBeds::BeginPlay()
{
	Super::BeginPlay();
	RebuildBeds();
	// Beds are built locally after joining: pick up a storm or alarm that is already running.
	if (const auto* Voyage = ACalmyVoyageDirector::Find(GetWorld())) SetStormLevel(Voyage->State.StormPercent / 100.f);
	if (const auto* Leak = ACalmyLeak::Find(GetWorld()))
		SetLeakAlarm(Leak->State.Phase == ECalmyLeakPhase::Warning || Leak->State.Phase == ECalmyLeakPhase::Leaking);
}

void ACalmyAmbientBeds::EndPlay(const EEndPlayReason::Type Reason)
{
	for (UAudioComponent* Comp : {Wind.Get(), Hull.Get(), Sea.Get(), Alarm.Get()})
	{
		if (Comp) Comp->Stop();
	}
	Super::EndPlay(Reason);
}

void ACalmyAmbientBeds::Feed(USoundWaveProcedural* Wave, int32 Kind, float Gain)
{
	if (!Wave) return;
	constexpr int32 Rate = 22050;
	constexpr int32 Chunk = Rate / 4; // 250 ms
	if (Wave->GetAvailableAudioByteCount() > Chunk * int32(sizeof(int16))) return;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(Chunk);
	for (int32 I = 0; I < Chunk; ++I)
	{
		Phase += 1.f / Rate;
		const float T = Phase;
		float Low = Noise.FRandRange(-1.f, 1.f);
		float Value = 0.f;
		if (Kind == 0) // wind
		{
			Value = Low * (.55f + .45f * FMath::Sin(2.f * PI * T / 16.f)) * (.35f + Storm01 * .65f);
			Value += .02f * FMath::Sin(2.f * PI * 71.f * T);
		}
		else if (Kind == 1) // hull
		{
			const float Pulse = FMath::Pow(FMath::Max(0.f, FMath::Sin(2.f * PI * T / 16.f)), 8.f);
			Value = .04f * FMath::Sin(2.f * PI * 43.f * T) + Pulse * (.05f + Storm01 * .08f) * FMath::Sin(2.f * PI * 117.f * T);
		}
		else if (Kind == 2) // sea
		{
			Value = Low * (.25f + Storm01 * .55f) + .03f * FMath::Sin(2.f * PI * 28.f * T + Storm01);
		}
		else // alarm
		{
			Value = bLeakAlarm ? FMath::Sin(2.f * PI * (880.f + 40.f * FMath::Sin(2.f * PI * 4.f * T)) * T) * .35f : 0.f;
		}
		Samples[I] = static_cast<int16>(FMath::Clamp(Value * Gain, -1.f, 1.f) * 22000.f);
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

void ACalmyAmbientBeds::RebuildBeds()
{
	if (!GetWorld() || !GetWorld()->AllowAudioPlayback()) return;
	auto Make = [this](TObjectPtr<USoundWaveProcedural>& Wave, UAudioComponent* Comp, float Volume)
	{
		Wave = NewObject<USoundWaveProcedural>(this);
		Wave->SetSampleRate(22050);
		Wave->NumChannels = 1;
		Wave->bLooping = true;
		Wave->Duration = 3600.f;
		Comp->SetSound(Wave);
		Comp->SetVolumeMultiplier(Volume);
		Comp->Play();
	};
	const UCalmyPreferences* Pref = UCalmyPreferences::Get();
	const float Amb = Pref ? Pref->AmbienceVolume * Pref->MasterVolume : 1.f;
	const bool bPort = ACalmyMissionBoard::IsPortMap(GetWorld());
	const bool bShip = ACalmyVoyageDirector::IsShipMap(GetWorld());
	Make(WindWave, Wind, Amb * (bShip || bPort ? .35f : .15f));
	Make(HullWave, Hull, Amb * (bShip ? .28f : .08f));
	Make(SeaWave, Sea, Amb * (bShip || bPort ? .22f : .05f));
	Make(AlarmWave, Alarm, Amb * .2f);
	Alarm->SetVolumeMultiplier(0.f);
}

void ACalmyAmbientBeds::SetStormLevel(float InStorm)
{
	Storm01 = FMath::Clamp(InStorm, 0.f, 1.f);
	if (Sea) Sea->SetVolumeMultiplier((UCalmyPreferences::Get() ? UCalmyPreferences::Get()->AmbienceVolume * UCalmyPreferences::Get()->MasterVolume : 1.f) * (.22f + Storm01 * .55f));
	if (Wind) Wind->SetVolumeMultiplier((UCalmyPreferences::Get() ? UCalmyPreferences::Get()->AmbienceVolume * UCalmyPreferences::Get()->MasterVolume : 1.f) * (.35f + Storm01 * .5f));
}

void ACalmyAmbientBeds::SetLeakAlarm(bool bActive)
{
	bLeakAlarm = bActive;
	if (Alarm)
	{
		const float Amb = UCalmyPreferences::Get() ? UCalmyPreferences::Get()->AmbienceVolume * UCalmyPreferences::Get()->MasterVolume : 1.f;
		Alarm->SetVolumeMultiplier(bActive ? Amb * .35f : 0.f);
	}
}

void ACalmyAmbientBeds::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld() || !GetWorld()->AllowAudioPlayback()) return;
	Feed(WindWave, 0, .9f);
	Feed(HullWave, 1, .85f);
	Feed(SeaWave, 2, .8f);
	Feed(AlarmWave, 3, .7f);
}
