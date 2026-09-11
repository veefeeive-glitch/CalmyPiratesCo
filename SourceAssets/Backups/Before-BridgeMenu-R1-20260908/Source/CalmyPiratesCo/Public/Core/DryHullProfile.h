#pragma once
#include "CoreMinimal.h"
/** Static sealed-hull exclusion, NOT flooding or buoyancy simulation. Empty profile = ordinary ocean. */
struct FCalmyDryHullProfile
{
    static double Sample(const TArray<FVector2D>& Curve,double X)
    {
        if(Curve.Num()<2 || !FMath::IsFinite(X) || X<Curve[0].X || X>Curve.Last().X)return -1;
        for(int32 I=1;I<Curve.Num();++I)
        {
            if(Curve[I].ContainsNaN() || Curve[I].X<=Curve[I-1].X)return -1;
            if(X<=Curve[I].X)return FMath::Lerp(Curve[I-1].Y,Curve[I].Y,(X-Curve[I-1].X)/(Curve[I].X-Curve[I-1].X));
        }
        return -1;
    }
    static bool Contains(const FVector& P,const TArray<FVector2D>& Stations,const TArray<FVector2D>& Vertical)
    {
        if(P.ContainsNaN())return false;
        const double Beam=Sample(Stations,P.X),Factor=Sample(Vertical,P.Z);
        return Beam>0 && Factor>0 && FMath::Abs(P.Y)<Beam*Factor-2.;
    }
};
