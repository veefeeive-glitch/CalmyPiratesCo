#pragma once
#include "CoreMinimal.h"
#include "Sound/SoundAttenuation.h"

class UObject;
class UWorld;

enum class ECalmySoundCue : uint8
{
	UiSelect,
	UiConfirm,
	UiCancel,
	DoorOpen,
	DoorClose,
	CargoGrab,
	CargoDrop,
	CargoThrow,
	CargoDamage,
	WatchBegin,
	WatchOutcomeGood,
	WatchOutcomeBad,
	PortAccept,
	ReturnPort,
	Punch,
	FlySwat,
	FlyTalk,
	LeakAlarm,
	FuelPulse,
	RepairTick,
	StormPulse,
	DoorCreak,
	DoorLatch,
	DoorSlam,
	DoorThud,
	DoorGrab,
};

/** Original procedural SFX + ambient beds. No third-party recordings. */
struct CALMYPIRATESCO_API FCalmyAudio
{
	static bool CanPlay(const UObject* WorldContext);
	static void Play2D(UObject* WorldContext, ECalmySoundCue Cue, float VolumeScale = 1.f);
	/** World sound: spatialised, fades with distance and is muffled through bulkheads (occlusion traces). */
	static void PlayAt(UObject* WorldContext, ECalmySoundCue Cue, const FVector& Location, float VolumeScale = 1.f, float Pitch = 1.f);
	/** How far a world cue carries before it is silent. */
	static float AudibleRadiusCm(ECalmySoundCue Cue);
	static FSoundAttenuationSettings MakeAttenuation(ECalmySoundCue Cue);
	static void EnsureWorldAmbience(UWorld* World);
	static void SetStormLevel(UWorld* World, float Storm01);
	static void SetLeakAlarm(UWorld* World, bool bActive);
};
