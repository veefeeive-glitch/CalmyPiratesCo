#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

/**
 * R8 forearm-local hand contract (glove baked into SM_*_Forearm*).
 * Continuous skinned fingers / separate hand meshes are not in Content yet;
 * these offsets and sockets scaffold contact until articulated art lands.
 *
 * Authoring reference: Scripts/create_crew_r8.py glove hand along -Z,
 * finger grooves near Z=-30.8..-35.6, tip near Z=-36.6.
 */
struct FCalmyCrewHand
{
	static FName WristSocket() { return FName(TEXT("Wrist")); }
	static FName PalmSocket() { return FName(TEXT("Palm")); }
	static FName TipSocket() { return FName(TEXT("Tip")); }

	/** Palm centre in forearm mesh local centimetres. */
	static FVector PalmLocal() { return FVector(0.f, 0.f, -33.f); }

	/** Finger-tip contact in forearm mesh local centimetres. */
	static FVector TipLocal() { return FVector(2.f, 0.f, -36.f); }

	/**
	 * Cosmetic carry wrap on the lower-arm motor target (degrees).
	 * Not skinned finger bones — only a curl cue on the existing forearm part.
	 */
	static FRotator CarryFingerCurl(bool bRightHand)
	{
		const float Side = bRightHand ? 1.f : -1.f;
		return FRotator(-12.f, Side * 8.f, Side * -15.f);
	}

	/**
	 * World contact point: Palm/Tip socket on the forearm mesh if stamped,
	 * else TipLocal through the forearm component transform.
	 */
	static FVector ContactWorld(const UStaticMeshComponent* Forearm, bool bPreferTip = true)
	{
		if (!IsValid(Forearm)) return FVector::ZeroVector;
		const FName Primary = bPreferTip ? TipSocket() : PalmSocket();
		const FName Secondary = bPreferTip ? PalmSocket() : TipSocket();
		if (Forearm->DoesSocketExist(Primary)) return Forearm->GetSocketLocation(Primary);
		if (Forearm->DoesSocketExist(Secondary)) return Forearm->GetSocketLocation(Secondary);
		return Forearm->GetComponentTransform().TransformPosition(bPreferTip ? TipLocal() : PalmLocal());
	}
};
