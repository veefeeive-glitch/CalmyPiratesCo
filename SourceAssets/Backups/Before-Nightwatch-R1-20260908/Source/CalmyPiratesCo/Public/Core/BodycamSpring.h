#pragma once
#include "CoreMinimal.h"
// Local camera dynamics only. Substepped and bounded so a hitch or landing cannot
// inject a permanent roll or NaN into the player's view.
struct FCalmyBodycamSpring
{
    FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector;
    void Step(FVector Target,float DT)
    {
        if(Target.ContainsNaN() || !FMath::IsFinite(DT) || DT<=0) return;
        if(Position.ContainsNaN() || Velocity.ContainsNaN()) { Position=Velocity=FVector::ZeroVector; }
        Target.X=FMath::Clamp(Target.X,-4.,4.);Target.Y=FMath::Clamp(Target.Y,-4.,4.);Target.Z=FMath::Clamp(Target.Z,-3.,3.);
        float Left=FMath::Min(DT,.1f);
        while(Left>SMALL_NUMBER)
        {
            const float H=FMath::Min(Left,1.f/120.f);Left-=H;
            Velocity+=((Target-Position)*95.f-Velocity*14.f)*H;
            Position+=Velocity*H;
        }
        Position.X=FMath::Clamp(Position.X,-5.,5.);Position.Y=FMath::Clamp(Position.Y,-5.,5.);Position.Z=FMath::Clamp(Position.Z,-6.,6.);
        Velocity=Velocity.GetClampedToMaxSize(100.f);
    }
};
