#pragma once
#include "CoreMinimal.h"
struct FCalmyArmIK
{
    static FVector Elbow(const FVector& Shoulder,const FVector& Target,const FVector& Pole,float Upper=30,float Lower=34)
    {
        const FVector D=(Target-Shoulder).GetSafeNormal(SMALL_NUMBER,FVector::ForwardVector);
        const float L=FMath::Clamp(static_cast<float>(FVector::Distance(Shoulder,Target)),FMath::Abs(Upper-Lower)+.1f,Upper+Lower-.1f);
        const float Along=(Upper*Upper-Lower*Lower+L*L)/(2*L);
        FVector Side=(Pole-D*FVector::DotProduct(Pole,D)).GetSafeNormal();
        if(Side.IsNearlyZero()) Side=FVector::CrossProduct(D,FVector::RightVector).GetSafeNormal();
        return Shoulder+D*Along+Side*FMath::Sqrt(FMath::Max(0.f,Upper*Upper-Along*Along));
    }
};
