#pragma once
#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Fonts/SlateFontInfo.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

/** Canvas helper for diegetic console screens: layout in logical units, Roboto rasterised at the target size. */
struct FCalmyScreenPainter
{
    UCanvas* Canvas=nullptr;
    float Scale=1;
    /** Emissive gain. Screens seen through the gameplay camera's eye adaptation need less than 1 to keep contrast. */
    float Gain=1;
    FCalmyScreenPainter(UCanvas* InCanvas,float LogicalWidth,float InGain=1):Canvas(InCanvas),Scale(InCanvas?InCanvas->ClipX/LogicalWidth:1),Gain(InGain){}
    FLinearColor Lit(const FLinearColor& Color) const { return FLinearColor(Color.R*Gain,Color.G*Gain,Color.B*Gain,Color.A); }
    FSlateFontInfo Font(float Size,bool bBold,int32 Spacing=0) const
    {
        FSlateFontInfo F(GEngine->GetMediumFont(),Size*Scale,bBold?FName(TEXT("Bold")):FName(TEXT("Regular")));
        F.LetterSpacing=Spacing;return F;
    }
    float Width(const FString& Str,const FSlateFontInfo& F) const
    {
        if(!FSlateApplication::IsInitialized()||!FSlateApplication::Get().GetRenderer())return Str.Len()*F.Size*.55f/Scale;
        return float(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str,F).X)/Scale;
    }
    void Rect(float X,float Y,float W,float H,const FLinearColor& Color) const
    {
        FCanvasTileItem Item(FVector2D(X*Scale,Y*Scale),FVector2D(W*Scale,H*Scale),Lit(Color));
        Item.BlendMode=SE_BLEND_Opaque;Canvas->DrawItem(Item);
    }
    void Text(const FString& Str,float X,float Y,const FSlateFontInfo& F,const FLinearColor& Color,bool bRightAligned=false) const
    {
        if(Str.IsEmpty())return;
        FCanvasTextItem Item(FVector2D((bRightAligned?X-Width(Str,F):X)*Scale,Y*Scale),FText::FromString(Str),F,Lit(Color));
        Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
    }
};
