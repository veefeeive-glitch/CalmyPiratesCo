#pragma once
#include "CoreMinimal.h"

/**
 * Kind-specific grip locals in cargo actor centimetres (collision-root space).
 * Biased toward R3 handle fittings from create_crew_cargo_r3.py while staying
 * inside the two-bone arm reach that Grip/Health R1 already verified.
 * True mesh sockets / finger skin remain a later slice.
 */
struct FCalmyCargoGrip
{
	// Mirrors ECalmyCargoKind order: Standard, Bulky, Unstable, Fragile.
	enum class EKind : uint8 { Standard = 0, Bulky = 1, Unstable = 2, Fragile = 3 };

	static FVector HandleLocal(EKind Kind, bool bRightHand, bool bFarSide)
	{
		const float Hand = bRightHand ? 1.f : -1.f;
		const float Facing = bFarSide ? -1.f : 1.f;
		const float Y = Hand * Facing;
		switch (Kind)
		{
		case EKind::Standard:
			// Near/far faces + lateral bias toward side grab rails (Y±47, Z30).
			return FVector(bFarSide ? 48.f : -48.f, Y * 42.f, 18.f);
		case EKind::Bulky:
			// Two-person top rails (Y±44, Z43): keep X deep, raise Z modestly.
			return FVector(bFarSide ? 52.f : -52.f, Y * 40.f, 26.f);
		case EKind::Unstable:
			// Cage/drum side grasp (no dedicated handle).
			return FVector(bFarSide ? 44.f : -44.f, Y * 36.f, 14.f);
		case EKind::Fragile:
			// Carry handles sit lower (Y±47, Z8).
			return FVector(bFarSide ? 46.f : -46.f, Y * 40.f, 10.f);
		default:
		{
			checkNoEntry();
			return FVector(bFarSide ? 50.f : -50.f, Hand * (bFarSide ? -30.f : 30.f), 12.f);
		}
		}
	}

	static FVector HandleLocal(uint8 Kind, bool bRightHand, bool bFarSide)
	{
		return HandleLocal(static_cast<EKind>(Kind), bRightHand, bFarSide);
	}

	/** Horizontal span between left and right near-side handles; used by unit tests. */
	static float NearHandSpan(EKind Kind)
	{
		const FVector L = HandleLocal(Kind, false, false);
		const FVector R = HandleLocal(Kind, true, false);
		return static_cast<float>(FVector::Dist2D(L, R));
	}
};
