#pragma once
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "GameFramework/PlayerController.h"

/** Test input only: approach physically and aim, never bypass the gameplay grab validation. */
struct FCalmyPickupFixture
{
    static void Aim(ACalmyCharacter* P,const ACalmyCargo* C)
    {
        if(!P || !C) return;
        if(auto* PC=Cast<APlayerController>(P->GetController()))
            for(int32 I=0;I<2;++I) PC->SetControlRotation((C->GetActorLocation()-P->GetInteractionOrigin()).Rotation());
    }
    static bool Approach(ACalmyCharacter* P,const ACalmyCargo* C)
    {
        Aim(P,C);
        if(!P || !C) return false;
        const FVector2D Delta=FVector2D(C->GetActorLocation()-P->GetActorLocation());
        if(Delta.Size()>175) {P->MoveIntent(Delta.GetSafeNormal()*FMath::Min(1.,(Delta.Size()-145)/100.));return false;}
        return P->FindGrabTarget()==C;
    }
    static void Forward(ACalmyCharacter* P)
    {if(auto* PC=Cast<APlayerController>(P->GetController())) PC->SetControlRotation(FRotator::ZeroRotator);}
};
