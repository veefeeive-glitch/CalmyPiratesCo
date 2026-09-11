#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"

struct FCalmyCrewRecovery
{
    static bool FindStandingCapsule(UWorld* World,const AActor* Ignored,const FVector& Root,float Radius,float HalfHeight,FVector& Position)
    {
        if(!World || Root.ContainsNaN() || !FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius<=0 || HalfHeight<Radius) return false;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(CrewStandClearance),false,Ignored);
        FHitResult Floor;
        if(!World->LineTraceSingleByChannel(Floor,Root+FVector(0,0,30),Root-FVector(0,0,110),ECC_WorldStatic,Q) || Floor.ImpactNormal.Z<.75f) return false;
        Position=FVector(Root.X,Root.Y,Floor.ImpactPoint.Z+HalfHeight+3.f);
        return !World->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,HalfHeight),Q);
    }
};
