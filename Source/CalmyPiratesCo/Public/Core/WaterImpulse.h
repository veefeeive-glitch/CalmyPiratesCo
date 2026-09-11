#pragma once
#include "CoreMinimal.h"

/** Bounded, damped radial height-field response in centimetres/seconds.
 * An interactive wave approximation, NOT a volume-conserving fluid solver.
 * Shared by the server, client physics and render mesh; no frame integration drift.
 */
struct FCalmyWaterImpulse
{
    static constexpr double Lifetime = 5.;
    static constexpr float Speed = 180.f;
    static constexpr int32 Capacity = 48;
    static float Height(float Distance, double Age, float Amplitude, float Width)
    {
        if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Age) || !FMath::IsFinite(Amplitude)
            || !FMath::IsFinite(Width) || Age <= 0 || Age >= Lifetime || Distance < 0 || Width <= 0) return 0;
        Width = FMath::Clamp(Width, 55.f, 150.f);
        const double S = (Distance - Speed * Age) / Width;
        // Smooth compact support avoids evaluating exp on the entire ocean grid.
        if (FMath::Abs(S) >= 4.) return 0;
        const double Support = FMath::Square(1. - FMath::Pow(S / 4., 4.));
        const double In = FMath::Clamp(Age / .18, 0., 1.);
        const double Out = FMath::Clamp((Lifetime - Age) / 1.2, 0., 1.);
        const double Envelope = In * In * (3 - 2 * In) * Out * Out * (3 - 2 * Out) * FMath::Exp(-.45 * Age);
        return FMath::Clamp(Amplitude, 0.f, 16.f) * Envelope * (1. - S * S) * FMath::Exp(-.5 * S * S) * Support;
    }
    // Smooth aggregate limit: unlike hard clipping, its derivative stays continuous.
    static float Limit(float Height) { return Height / FMath::Sqrt(1.f + FMath::Square(Height / 24.f)); }
    static float ImpactAmplitude(float MassKg, float RelativeSpeed)
    {
        if (!FMath::IsFinite(MassKg) || !FMath::IsFinite(RelativeSpeed) || MassKg <= 0) return 0;
        return FMath::Clamp(FMath::Sqrt(MassKg / 70.f) * FMath::Max(0.f, RelativeSpeed) * .028f, 0.f, 16.f);
    }
    static bool CrossedSurface(float PreviousBottom, float Bottom, float Surface, float PreviousSurface)
    {
        return PreviousBottom > PreviousSurface && Bottom <= Surface;
    }
    static bool ContinuousMotion(double Dt, float Distance)
    {
        return FMath::IsFinite(Dt) && FMath::IsFinite(Distance) && Dt>0 && Dt<.3
            && Distance>=0 && Distance<FMath::Max(180.,Dt*2000.);
    }
};
