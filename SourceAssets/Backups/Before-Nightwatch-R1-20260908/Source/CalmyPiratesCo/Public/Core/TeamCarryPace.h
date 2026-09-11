#pragma once
#include "CoreMinimal.h"

/** Server-side carrying effort; slowing down gives the partner time to catch up. */
struct FCalmyTeamCarryPace
{
    static float Evaluate(float Distance, float RelativeSpeed)
    {
        if(!FMath::IsFinite(Distance) || !FMath::IsFinite(RelativeSpeed)) return .55f;
        const float Separation=FMath::Clamp((Distance-145.f)/135.f,0.f,1.f);
        const float Disagreement=FMath::Clamp(RelativeSpeed/450.f,0.f,1.f);
        return FMath::Clamp(1.f-.3f*Separation-.15f*Disagreement,.55f,1.f);
    }
};
