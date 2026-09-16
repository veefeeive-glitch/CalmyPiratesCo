#pragma once
#include "CoreMinimal.h"

/** What the top-of-screen arrow points at, most urgent first. */
enum class ECalmyBreakdownKind : uint8
{
	None,
	FuelLow,
	LeakWarning,
	FuelCritical,
	Leak,
};

struct CALMYPIRATESCO_API FCalmyBreakdownTarget
{
	ECalmyBreakdownKind Kind = ECalmyBreakdownKind::None;
	FVector Location = FVector::ZeroVector;
	bool IsValid() const { return Kind != ECalmyBreakdownKind::None; }
};

/** Screen arrow for a world target, relative to the local view. */
struct CALMYPIRATESCO_API FCalmyCompassArrow
{
	/** Heading to the target relative to the view yaw, (-180, 180]; positive means turn right. */
	float YawDelta = 0.f;
	/** Degrees above (+) or below (-) the viewer's horizontal plane. */
	float Elevation = 0.f;
	float DistanceMeters = 0.f;
	/** Close enough that the arrow gives way to "HERE". */
	bool bArrived = false;
	/** The target is on another deck (more than one storey up or down). */
	int32 DeckOffset = 0;
};

/**
 * NFS-style breakdown compass: selects the most urgent breakdown from replicated state and computes the arrow.
 * Pure presentation math; all inputs are already replicated to every client.
 */
class CALMYPIRATESCO_API FCalmyBreakdownCompass
{
public:
	static constexpr int32 FuelLowPercent = 25;
	static constexpr int32 FuelCriticalPercent = 10;
	static constexpr float ArrivedCm = 250.f;
	static constexpr float StoreyCm = 250.f;

	static FCalmyBreakdownTarget Select(bool bLeakWarning, bool bLeaking, const FVector& Valve,
		bool bFuelWatchActive, int32 FuelPercent, const FVector& Pump);
	static FCalmyCompassArrow Arrow(const FRotator& View, const FVector& ViewLocation, const FVector& Target);
	static const TCHAR* Label(ECalmyBreakdownKind Kind);
	static bool IsCritical(ECalmyBreakdownKind Kind) { return Kind == ECalmyBreakdownKind::Leak || Kind == ECalmyBreakdownKind::FuelCritical; }
};
