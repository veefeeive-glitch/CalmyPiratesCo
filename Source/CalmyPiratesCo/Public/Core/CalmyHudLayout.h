#pragma once
#include "CoreMinimal.h"

/** Axis-aligned HUD rectangle in reference units (1080 units tall at HUD scale 1). */
struct CALMYPIRATESCO_API FCalmyHudRect
{
	float X = 0, Y = 0, W = 0, H = 0;
	float Right() const { return X + W; }
	float Bottom() const { return Y + H; }
	bool IsEmpty() const { return W <= 0 || H <= 0; }
	bool Overlaps(const FCalmyHudRect& Other) const;
	bool Inside(const FVector2D& Viewport) const;
};

/** Sizes of the minimal HUD blocks for one frame. Zero size means the block is absent. */
struct CALMYPIRATESCO_API FCalmyHudLayoutInput
{
	FVector2D Viewport = FVector2D(1920, 1080);
	/** Top-centre messages in priority order: alert, toast, objective, tip. */
	TArray<FVector2D> Top;
	/** Interaction prompt under the crosshair; PromptTop is its preferred top edge. */
	FVector2D Prompt = FVector2D::ZeroVector;
	float PromptTop = 0;
	/** Bottom-centre subtitles and bottom-left vitals. */
	FVector2D Subtitle = FVector2D::ZeroVector;
	FVector2D Vitals = FVector2D::ZeroVector;
	/** Top-right FPS readout. */
	FVector2D Fps = FVector2D::ZeroVector;
};

struct CALMYPIRATESCO_API FCalmyHudLayout
{
	TArray<FCalmyHudRect> Top;
	FCalmyHudRect Prompt;
	FCalmyHudRect Subtitle;
	FCalmyHudRect Vitals;
	FCalmyHudRect Fps;
};

/**
 * Minimal anchored HUD that never overlaps. Top messages stack from the top centre within the upper 42% of
 * the screen, the prompt stays below them, vitals sit bottom-left and subtitles bottom-centre (raised above the
 * vitals when they would collide). Anything that cannot fit is dropped rather than drawn on top of another block.
 */
class CALMYPIRATESCO_API FCalmyHudLayoutSolver
{
public:
	static constexpr float Margin = 24;
	static constexpr float Gap = 8;
	static FCalmyHudLayout Solve(const FCalmyHudLayoutInput& Input);
	/** Every visible block pair is disjoint and inside the viewport. */
	static bool IsValid(const FCalmyHudLayout& Layout, const FVector2D& Viewport);
};
