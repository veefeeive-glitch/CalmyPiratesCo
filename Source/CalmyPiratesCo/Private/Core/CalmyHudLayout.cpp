#include "Core/CalmyHudLayout.h"

bool FCalmyHudRect::Overlaps(const FCalmyHudRect& Other) const
{
	if (IsEmpty() || Other.IsEmpty()) return false;
	return X < Other.Right() && Other.X < Right() && Y < Other.Bottom() && Other.Y < Bottom();
}

bool FCalmyHudRect::Inside(const FVector2D& Viewport) const
{
	return IsEmpty() || (X >= 0 && Y >= 0 && Right() <= Viewport.X + .01f && Bottom() <= Viewport.Y + .01f);
}

FCalmyHudLayout FCalmyHudLayoutSolver::Solve(const FCalmyHudLayoutInput& In)
{
	FCalmyHudLayout Out;
	const float VX = float(In.Viewport.X), VY = float(In.Viewport.Y);
	const float MaxW = FMath::Max(0.f, VX - 2 * Margin);
	auto Present = [](const FVector2D& Size) { return Size.X > 0 && Size.Y > 0; };
	auto Centered = [&](const FVector2D& Size, float Y)
	{
		const float W = FMath::Min(float(Size.X), MaxW);
		return FCalmyHudRect{(VX - W) * .5f, Y, W, float(Size.Y)};
	};

	if (Present(In.Fps)) Out.Fps = {VX - Margin - float(In.Fps.X), Margin, float(In.Fps.X), float(In.Fps.Y)};

	float Y = Margin;
	for (const FVector2D& Item : In.Top)
	{
		if (!Present(Item)) continue;
		FCalmyHudRect Rect = Centered(Item, Y);
		if (Rect.Overlaps(Out.Fps)) Rect.Y = Out.Fps.Bottom() + Gap;
		if (Rect.Bottom() > VY * .42f) break;
		Out.Top.Add(Rect);
		Y = Rect.Bottom() + Gap;
	}
	const float TopBottom = Out.Top.Num() ? Out.Top.Last().Bottom() + Gap : 0;

	if (Present(In.Prompt))
	{
		const FCalmyHudRect Rect = Centered(In.Prompt, FMath::Max(In.PromptTop, TopBottom));
		if (Rect.Inside(In.Viewport) && !Rect.Overlaps(Out.Fps)) Out.Prompt = Rect;
	}
	if (Present(In.Vitals))
	{
		const FCalmyHudRect Rect{Margin, VY - Margin - float(In.Vitals.Y), FMath::Min(float(In.Vitals.X), MaxW), float(In.Vitals.Y)};
		if (Rect.Inside(In.Viewport) && Rect.Y >= TopBottom && !Rect.Overlaps(Out.Prompt)) Out.Vitals = Rect;
	}
	if (Present(In.Subtitle))
	{
		FCalmyHudRect Rect = Centered(In.Subtitle, VY - Margin - float(In.Subtitle.Y));
		if (Rect.Overlaps(Out.Vitals)) Rect.Y = Out.Vitals.Y - Gap - Rect.H;
		if (Rect.Inside(In.Viewport) && Rect.Y >= TopBottom && !Rect.Overlaps(Out.Prompt) && !Rect.Overlaps(Out.Vitals) && !Rect.Overlaps(Out.Fps))
			Out.Subtitle = Rect;
	}
	return Out;
}

bool FCalmyHudLayoutSolver::IsValid(const FCalmyHudLayout& Layout, const FVector2D& Viewport)
{
	TArray<FCalmyHudRect> All = Layout.Top;
	All.Add(Layout.Prompt);
	All.Add(Layout.Subtitle);
	All.Add(Layout.Vitals);
	All.Add(Layout.Fps);
	for (int32 A = 0; A < All.Num(); ++A)
	{
		if (!All[A].Inside(Viewport)) return false;
		for (int32 B = A + 1; B < All.Num(); ++B)
			if (All[A].Overlaps(All[B])) return false;
	}
	return true;
}
