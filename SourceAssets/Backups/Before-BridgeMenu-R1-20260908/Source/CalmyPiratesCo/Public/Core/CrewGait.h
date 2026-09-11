#pragma once
#include "CoreMinimal.h"
// Two-link sagittal leg target in centimetres/degrees; cosmetic motor target only.
struct FCalmyLegPose
{
    float Hip=0,Knee=0;
    static FCalmyLegPose Solve(float Forward,float Down)
    {
        if(!FMath::IsFinite(Forward)||!FMath::IsFinite(Down)) return {};
        FVector2D Goal(Forward,FMath::Max(20.f,Down));
        Goal=Goal.GetClampedToMaxSize(81.f);
        const float K=-FMath::Acos(FMath::Clamp((Goal.SizeSquared()-38.f*38.f-44.f*44.f)/(2.f*38.f*44.f),-1.,1.));
        const float H=FMath::Atan2(Goal.X,Goal.Y)-FMath::Atan2(44.f*FMath::Sin(K),38.f+44.f*FMath::Cos(K));
        return {FMath::Clamp(FMath::RadiansToDegrees(H),-60.f,80.f),FMath::Clamp(FMath::RadiansToDegrees(K),-100.f,0.f)};
    }
};
