#include "Core/CalmyAudio.h"
#include "Gameplay/CalmyAmbientBeds.h"
#include "Gameplay/CalmyPreferences.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace CalmyAudioPrivate
{
	struct FTone
	{
		float Duration = .12f;
		float Freq0 = 440.f;
		float Freq1 = 440.f;
		float Amp = .35f;
		float Decay = 18.f;
		int32 Harmonics = 1;
		bool bNoise = false;
		float NoiseMix = 0.f;
	};

	static FTone ToneFor(ECalmySoundCue Cue)
	{
		FTone T;
		switch (Cue)
		{
		case ECalmySoundCue::UiSelect: T = {.08f, 620.f, 780.f, .22f, 28.f, 1, false, 0.f}; break;
		case ECalmySoundCue::UiConfirm: T = {.14f, 480.f, 920.f, .28f, 16.f, 2, false, 0.f}; break;
		case ECalmySoundCue::UiCancel: T = {.1f, 300.f, 180.f, .2f, 20.f, 1, false, 0.f}; break;
		case ECalmySoundCue::DoorOpen: T = {.28f, 90.f, 140.f, .4f, 8.f, 2, true, .35f}; break;
		case ECalmySoundCue::DoorClose: T = {.22f, 160.f, 70.f, .45f, 12.f, 2, true, .4f}; break;
		case ECalmySoundCue::CargoGrab: T = {.12f, 220.f, 360.f, .32f, 22.f, 2, true, .2f}; break;
		case ECalmySoundCue::CargoDrop: T = {.16f, 140.f, 90.f, .38f, 14.f, 2, true, .45f}; break;
		case ECalmySoundCue::CargoThrow: T = {.18f, 180.f, 420.f, .34f, 12.f, 2, true, .25f}; break;
		case ECalmySoundCue::CargoDamage: T = {.18f, 700.f, 280.f, .4f, 25.f, 1, false, 0.f}; break;
		case ECalmySoundCue::WatchBegin: T = {.35f, 260.f, 520.f, .3f, 7.f, 3, false, 0.f}; break;
		case ECalmySoundCue::WatchOutcomeGood: T = {.45f, 330.f, 660.f, .32f, 6.f, 3, false, 0.f}; break;
		case ECalmySoundCue::WatchOutcomeBad: T = {.4f, 240.f, 110.f, .34f, 7.f, 2, true, .15f}; break;
		case ECalmySoundCue::PortAccept: T = {.28f, 400.f, 800.f, .3f, 9.f, 2, false, 0.f}; break;
		case ECalmySoundCue::ReturnPort: T = {.32f, 500.f, 250.f, .28f, 8.f, 2, false, 0.f}; break;
		case ECalmySoundCue::Punch: T = {.1f, 120.f, 60.f, .5f, 30.f, 1, true, .7f}; break;
		case ECalmySoundCue::FlySwat: T = {.08f, 900.f, 1400.f, .22f, 40.f, 1, true, .5f}; break;
		case ECalmySoundCue::FlyTalk: T = {.16f, 1100.f, 1500.f, .18f, 18.f, 2, false, 0.f}; break;
		case ECalmySoundCue::LeakAlarm: T = {.2f, 880.f, 660.f, .25f, 10.f, 1, false, 0.f}; break;
		case ECalmySoundCue::FuelPulse: T = {.15f, 200.f, 320.f, .28f, 14.f, 2, true, .2f}; break;
		case ECalmySoundCue::RepairTick: T = {.07f, 540.f, 540.f, .2f, 35.f, 1, false, 0.f}; break;
		// Heavy watertight door: low creak grains while swinging, a metal latch click, a slam and a stop knock.
		case ECalmySoundCue::DoorCreak: T = {.16f, 150.f, 120.f, .2f, 5.f, 4, true, .3f}; break;
		case ECalmySoundCue::DoorLatch: T = {.1f, 1250.f, 820.f, .34f, 42.f, 2, true, .45f}; break;
		case ECalmySoundCue::DoorSlam: T = {.42f, 68.f, 42.f, .62f, 8.f, 3, true, .55f}; break;
		case ECalmySoundCue::DoorThud: T = {.24f, 92.f, 64.f, .46f, 13.f, 2, true, .45f}; break;
		case ECalmySoundCue::DoorGrab: T = {.08f, 560.f, 380.f, .26f, 38.f, 2, true, .4f}; break;
		case ECalmySoundCue::StormPulse: T = {.5f, 55.f, 90.f, .35f, 4.f, 2, true, .55f}; break;
		}
		return T;
	}

	static USoundWaveProcedural* MakeWave(UObject* Outer, const FTone& Tone)
	{
		constexpr int32 Rate = 22050;
		auto* Wave = NewObject<USoundWaveProcedural>(Outer);
		Wave->SetSampleRate(Rate);
		Wave->NumChannels = 1;
		Wave->bLooping = false;
		Wave->Duration = Tone.Duration;
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Tone.Duration * Rate));
		TArray<int16> Samples;
		Samples.SetNumUninitialized(Count);
		FRandomStream Rng(917 + Count);
		for (int32 I = 0; I < Count; ++I)
		{
			const float T = I / float(Rate);
			const float Env = FMath::Exp(-Tone.Decay * T) * FMath::Min(1.f, T * 80.f);
			const float Freq = FMath::Lerp(Tone.Freq0, Tone.Freq1, T / FMath::Max(.001f, Tone.Duration));
			float Sample = 0.f;
			for (int32 H = 1; H <= Tone.Harmonics; ++H)
			{
				Sample += FMath::Sin(2.f * PI * Freq * H * T) / float(H);
			}
			if (Tone.bNoise) Sample = FMath::Lerp(Sample, Rng.FRandRange(-1.f, 1.f), Tone.NoiseMix);
			Samples[I] = static_cast<int16>(FMath::Clamp(Sample * Env * Tone.Amp, -1.f, 1.f) * 30000.f);
		}
		Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
		return Wave;
	}

	static float EffectVolume()
	{
		if (const UCalmyPreferences* Pref = UCalmyPreferences::Get()) return Pref->EffectsVolume * Pref->MasterVolume;
		return 1.f;
	}
}

bool FCalmyAudio::CanPlay(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World && World->AllowAudioPlayback() && World->GetNetMode() != NM_DedicatedServer;
}

void FCalmyAudio::Play2D(UObject* WorldContext, ECalmySoundCue Cue, float VolumeScale)
{
	if (!CanPlay(WorldContext)) return;
	USoundWaveProcedural* Wave = CalmyAudioPrivate::MakeWave(WorldContext->GetWorld(), CalmyAudioPrivate::ToneFor(Cue));
	UGameplayStatics::PlaySound2D(WorldContext, Wave, CalmyAudioPrivate::EffectVolume() * VolumeScale);
}

float FCalmyAudio::AudibleRadiusCm(ECalmySoundCue Cue)
{
	switch (Cue)
	{
	case ECalmySoundCue::LeakAlarm: return 4200.f;
	case ECalmySoundCue::DoorSlam: return 3200.f;
	case ECalmySoundCue::DoorCreak: case ECalmySoundCue::DoorThud: case ECalmySoundCue::DoorOpen: case ECalmySoundCue::DoorClose: return 2000.f;
	case ECalmySoundCue::DoorLatch: return 1500.f;
	case ECalmySoundCue::DoorGrab: return 700.f;
	case ECalmySoundCue::CargoGrab: case ECalmySoundCue::CargoDrop: case ECalmySoundCue::CargoThrow: case ECalmySoundCue::CargoDamage: return 1800.f;
	case ECalmySoundCue::Punch: return 1400.f;
	case ECalmySoundCue::FuelPulse: return 1300.f;
	case ECalmySoundCue::FlySwat: case ECalmySoundCue::FlyTalk: return 1100.f;
	default: return 2000.f;
	}
}

FSoundAttenuationSettings FCalmyAudio::MakeAttenuation(ECalmySoundCue Cue)
{
	// Without attenuation a world cue played at full volume everywhere on the ship; 61 doors sounded random.
	const float Radius = AudibleRadiusCm(Cue);
	FSoundAttenuationSettings S;
	S.bAttenuate = true;
	S.bSpatialize = true;
	S.AttenuationShape = EAttenuationShape::Sphere;
	S.AttenuationShapeExtents = FVector(Radius * .08f, 0., 0.);
	S.FalloffDistance = Radius * .92f;
	S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	S.bAttenuateWithLPF = true;
	S.LPFRadiusMin = Radius * .3f;
	S.LPFRadiusMax = Radius;
	S.LPFFrequencyAtMin = 20000.f;
	S.LPFFrequencyAtMax = 2500.f;
	// Steel bulkheads between the listener and the source muffle it.
	S.bEnableOcclusion = true;
	S.OcclusionTraceChannel = ECC_Visibility;
	S.OcclusionLowPassFilterFrequency = 650.f;
	S.OcclusionVolumeAttenuation = .3f;
	S.OcclusionInterpolationTime = .12f;
	return S;
}

void FCalmyAudio::PlayAt(UObject* WorldContext, ECalmySoundCue Cue, const FVector& Location, float VolumeScale, float Pitch)
{
	if (!CanPlay(WorldContext)) return;
	USoundWaveProcedural* Wave = CalmyAudioPrivate::MakeWave(WorldContext->GetWorld(), CalmyAudioPrivate::ToneFor(Cue));
	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Wave);
	Attenuation->Attenuation = MakeAttenuation(Cue);
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Wave, Location, CalmyAudioPrivate::EffectVolume() * VolumeScale,
		FMath::Clamp(Pitch, .5f, 2.f), 0.f, Attenuation);
}

void FCalmyAudio::EnsureWorldAmbience(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	ACalmyAmbientBeds::Ensure(World);
}

void FCalmyAudio::SetStormLevel(UWorld* World, float Storm01)
{
	if (ACalmyAmbientBeds* Beds = ACalmyAmbientBeds::Find(World)) Beds->SetStormLevel(Storm01);
}

void FCalmyAudio::SetLeakAlarm(UWorld* World, bool bActive)
{
	if (ACalmyAmbientBeds* Beds = ACalmyAmbientBeds::Find(World)) Beds->SetLeakAlarm(bActive);
}
