#pragma once
#include "CoreMinimal.h"

// Gameplay tuning, not a medical injury model. cm/s -> bounded energy-like loss.
struct FCalmyCrewImpact
{
    static float Damage(float Speed,bool bLanding)
    {
        if(!FMath::IsFinite(Speed) || Speed<=0) return 0;
        const float Safe=bLanding?700.f:600.f;
        const float V=FMath::Min(Speed,2500.f);
        return FMath::Clamp((V*V-Safe*Safe)*(bLanding?.00006f:.00003f),0.f,100.f);
    }
};
