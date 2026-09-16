#include "Core/CalmyDoorMotion.h"

FCalmyDoorMotion::EEvent FCalmyDoorMotion::Step(float Dt, float OpenAngle, bool bDrag, float Push, bool bAuto, float AutoTarget, float& OutImpactSpeed)
{
	OutImpactSpeed = 0.f;
	Dt = FMath::Clamp(Dt, 0.f, .1f);
	OpenAngle = FMath::Max(1.f, OpenAngle);
	if (Dt <= 0.f) return EEvent::None;

	if (bDrag)
	{
		const float Target = FMath::Clamp(Push, -1.f, 1.f) * MaxDragSpeed;
		Velocity = FMath::FInterpConstantTo(Velocity, Target, Dt, HeavyAccel);
	}
	else if (bAuto)
	{
		const float ToGo = FMath::Clamp(AutoTarget, 0.f, OpenAngle) - Angle;
		// Brake early enough to arrive gently (v^2 = 2ad), but never so slowly that the leaf stalls short.
		const float Arrive = FMath::Max(AutoMinSpeed, FMath::Sqrt(2.f * HeavyAccel * FMath::Abs(ToGo)));
		const float Target = FMath::Abs(ToGo) < KINDA_SMALL_NUMBER ? 0.f : FMath::Sign(ToGo) * FMath::Min(AutoSpeed, Arrive);
		Velocity = FMath::FInterpConstantTo(Velocity, Target, Dt, HeavyAccel);
		// Land exactly on an interior target instead of oscillating around it.
		if (FMath::Abs(ToGo) <= FMath::Abs(Velocity) * Dt && AutoTarget > 0.f && AutoTarget < OpenAngle)
		{
			Angle = AutoTarget;
			Velocity = 0.f;
			return EEvent::None;
		}
	}
	else
	{
		Velocity *= FMath::Exp(-FreeDamping * Dt);
		if (FMath::Abs(Velocity) < StopSpeed) Velocity = 0.f;
	}

	Angle += Velocity * Dt;
	if (Angle <= 0.f)
	{
		const float Speed = -Velocity;
		Angle = 0.f;
		Velocity = 0.f;
		if (Speed > 0.f)
		{
			OutImpactSpeed = Speed;
			return Speed >= SlamSpeed ? EEvent::Slammed : EEvent::Latched;
		}
		return EEvent::None;
	}
	if (Angle >= OpenAngle)
	{
		const float Speed = Velocity;
		Angle = OpenAngle;
		// A heavy leaf barely rebounds off its stop.
		Velocity = Speed > StopKnockSpeed ? -Speed * .12f : 0.f;
		if (Speed > StopKnockSpeed)
		{
			OutImpactSpeed = Speed;
			return EEvent::HitOpenStop;
		}
	}
	return EEvent::None;
}
