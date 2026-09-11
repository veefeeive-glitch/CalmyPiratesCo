#pragma once
#include "CoreMinimal.h"

/** Ray versus scaled/rotated cargo proxy. Input direction is world-normalized. */
struct FCalmyAimInteraction
{
    static bool HitBox(const FVector& Origin, const FVector& Direction, const FTransform& Transform,
        const FBox& Bounds, float MaxDistance, float& Distance)
    {
        if (Origin.ContainsNaN() || Direction.ContainsNaN() || Transform.ContainsNaN() ||
            !FMath::IsFinite(MaxDistance) || MaxDistance<=0 || !Direction.IsNormalized() || !Bounds.IsValid) return false;
        const FVector O=Transform.InverseTransformPosition(Origin);
        const FVector D=Transform.InverseTransformVector(Direction);
        double Near=0,Far=MaxDistance;
        for(int32 Axis=0;Axis<3;++Axis)
        {
            if(FMath::Abs(D[Axis])<1.e-8)
            { if(O[Axis]<Bounds.Min[Axis] || O[Axis]>Bounds.Max[Axis]) return false;continue; }
            double A=(Bounds.Min[Axis]-O[Axis])/D[Axis], B=(Bounds.Max[Axis]-O[Axis])/D[Axis];
            if(A>B) Swap(A,B);
            Near=FMath::Max(Near,A);Far=FMath::Min(Far,B);
            if(Near>Far) return false;
        }
        Distance=static_cast<float>(Near);return true;
    }
    static float Sensitivity(float Value)
    { return FMath::IsFinite(Value)?FMath::Clamp(Value,.25f,2.5f):1.f; }
};
