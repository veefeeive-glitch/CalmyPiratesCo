#pragma once
#include "CoreMinimal.h"

/**
 * Heavy hinged watertight door. Engine-free and deterministic so the feel is testable; angles in degrees, time in
 * seconds. A held door follows the player's push with limited acceleration (it takes a moment to get going and to
 * stop), a tapped door swings on its own and eases into the stop, and a free door coasts against hinge friction.
 */
struct CALMYPIRATESCO_API FCalmyDoorMotion
{
	/** Fastest a strong pull can swing the leaf. */
	static constexpr float MaxDragSpeed = 110.f;
	/** Angular acceleration limit: the leaf needs ~0.45 s to reach full drag speed. */
	static constexpr float HeavyAccel = 240.f;
	/** Cruise speed of a tapped (auto) open/close. */
	static constexpr float AutoSpeed = 75.f;
	/** Slowest auto approach so a tapped door always arrives. */
	static constexpr float AutoMinSpeed = 4.f;
	/** Hinge friction (1/s) when nobody drives the leaf. */
	static constexpr float FreeDamping = 1.6f;
	/** Static friction: a free leaf slower than this stops. */
	static constexpr float StopSpeed = 6.f;
	/** Closing faster than this slams instead of clicking shut. */
	static constexpr float SlamSpeed = 35.f;
	/** Hitting the open stop faster than this knocks. */
	static constexpr float StopKnockSpeed = 20.f;

	enum class EEvent : uint8 { None, Latched, Slammed, HitOpenStop };

	float Angle = 0.f;
	float Velocity = 0.f;

	/**
	 * Advances one step. Drive priority: drag (Push in [-1, 1], positive opens) > auto seek to AutoTarget > free coast.
	 * Returns the limit event of this step and its impact speed (deg/s).
	 */
	EEvent Step(float Dt, float OpenAngle, bool bDrag, float Push, bool bAuto, float AutoTarget, float& OutImpactSpeed);

	bool IsSettled() const { return Velocity == 0.f; }
};
