#include "Core/CalmyBreakdownCompass.h"

FCalmyBreakdownTarget FCalmyBreakdownCompass::Select(bool bLeakWarning, bool bLeaking, const FVector& Valve,
	bool bFuelWatchActive, int32 FuelPercent, const FVector& Pump)
{
	FCalmyBreakdownTarget Best;
	auto Offer = [&Best](ECalmyBreakdownKind Kind, const FVector& Location)
	{
		if (uint8(Kind) > uint8(Best.Kind)) { Best.Kind = Kind; Best.Location = Location; }
	};
	if (bLeaking) Offer(ECalmyBreakdownKind::Leak, Valve);
	else if (bLeakWarning) Offer(ECalmyBreakdownKind::LeakWarning, Valve);
	if (bFuelWatchActive && FuelPercent <= FuelCriticalPercent) Offer(ECalmyBreakdownKind::FuelCritical, Pump);
	else if (bFuelWatchActive && FuelPercent <= FuelLowPercent) Offer(ECalmyBreakdownKind::FuelLow, Pump);
	return Best;
}

FCalmyCompassArrow FCalmyBreakdownCompass::Arrow(const FRotator& View, const FVector& ViewLocation, const FVector& Target)
{
	FCalmyCompassArrow Out;
	const FVector Delta = Target - ViewLocation;
	const float Flat = float(Delta.Size2D());
	Out.YawDelta = Flat > KINDA_SMALL_NUMBER ? FMath::FindDeltaAngleDegrees(float(View.Yaw), float(Delta.Rotation().Yaw)) : 0.f;
	if (Out.YawDelta <= -180.f) Out.YawDelta += 360.f;
	Out.Elevation = FMath::RadiansToDegrees(FMath::Atan2(float(Delta.Z), FMath::Max(Flat, 1.f)));
	Out.DistanceMeters = float(Delta.Size()) / 100.f;
	Out.DeckOffset = FMath::Abs(Delta.Z) < StoreyCm * .8f ? 0 : int32(FMath::Sign(Delta.Z)) * FMath::Max(1, FMath::RoundToInt(float(FMath::Abs(Delta.Z)) / StoreyCm));
	Out.bArrived = Flat < ArrivedCm && Out.DeckOffset == 0;
	return Out;
}

const TCHAR* FCalmyBreakdownCompass::Label(ECalmyBreakdownKind Kind)
{
	switch (Kind)
	{
	case ECalmyBreakdownKind::Leak: return TEXT("LEAK");
	case ECalmyBreakdownKind::LeakWarning: return TEXT("LEAK WARNING");
	case ECalmyBreakdownKind::FuelCritical: return TEXT("FUEL CRITICAL");
	case ECalmyBreakdownKind::FuelLow: return TEXT("FUEL LOW");
	default: return TEXT("");
	}
}
