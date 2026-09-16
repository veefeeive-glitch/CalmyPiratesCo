#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmyGameMode.h"
#include "Core/CalmySession.h"
#include "Core/CalmyAudio.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Camera/PlayerCameraManager.h"
#include "ProceduralMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"

namespace CalmyTerminal
{
    // Ship-local coordinates: ShipMotion_R7 is placed at the map origin and carries every R7 mesh.
    // BridgeConsole1 ScreenFace in OceanFreighterR7: 94 x 40 cm, front plane X -3371.7,
    // DisplayTitle label ~0.7 cm in front of it. The skin sits just ahead of both.
    constexpr float ScreenCmW=94,ScreenCmH=40;
    const FVector ScreenCenter(-3373,-210,1988);
    // Helm chair spans X -3535..-3487; the camera stays between chair and console.
    constexpr float CameraDistanceCm=84,CameraFov=64;
    // Logical layout units match the physical 2.35:1 face; the target is rendered at 2x.
    constexpr float LogicalW=1024,LogicalH=436,TargetScale=2;
    constexpr float PadX=40,RowTop=112,RowPitch=50,RowH=42;
    struct FRow{FString Label;FString Value;bool bEnabled=true;};
}

ACalmyBridgeTerminal::ACalmyBridgeTerminal()
{
    PrimaryActorTick.bCanEverTick=false;
    bReplicates=false;
    Screen=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerminalScreen"));SetRootComponent(Screen);
    Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetCastShadow(false);
}
void ACalmyBridgeTerminal::Initialize(ACalmyPlayerController* Controller)
{
    using namespace CalmyTerminal;
    PC=Controller;SetOwner(PC);
    // Thin screen skin exactly over the physical display face; no new obstacle, no bezel overlap.
    // The console heaves, heels and drifts with ShipMotion_R7: place skin and camera in the ship frame
    // and attach them, so they stay on the display after the hull has moved (host and client alike).
    auto* Ship=ACalmyShipMotion::Find(GetWorld());
    const FTransform ShipFrame=Ship?Ship->GetActorTransform():FTransform::Identity;
    SetActorTransform(FTransform(ScreenCenter)*ShipFrame);
    if(Ship)AttachToActor(Ship,FAttachmentTransformRules::KeepWorldTransform);
    const float HW=ScreenCmW*.5f,HH=ScreenCmH*.5f;
    TArray<FVector> V={{0,-HW,HH},{0,HW,HH},{0,HW,-HH},{0,-HW,-HH}};
    Screen->CreateMeshSection(0,V,{0,1,2,0,2,3},{},{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)},{},{},false);
    // This actor exists only in the local player's world, never replicates.
    // OnlyOwnerSee tested the view actor (the separate camera), hiding the screen.
    Screen->SetOnlyOwnerSee(false);
    MenuCamera=GetWorld()->SpawnActor<ACameraActor>();
    // Square-on view: no keystone, text rows stay horizontal and the bezel frames the menu.
    MenuCamera->SetOwner(PC);MenuCamera->SetActorTransform(FTransform(ScreenCenter-FVector(CameraDistanceCm,0,0))*ShipFrame);
    if(Ship)MenuCamera->AttachToActor(Ship,FAttachmentTransformRules::KeepWorldTransform);
    MenuCamera->GetCameraComponent()->SetFieldOfView(CameraFov);
    MenuCamera->GetCameraComponent()->bConstrainAspectRatio=false;
    // The project keeps vertical FOV (BaseEngine MaintainYFOV), which cropped 4:3. Fix the horizontal
    // axis here and size it per viewport in FitCamera().
    MenuCamera->GetCameraComponent()->bOverrideAspectRatioAxisConstraint=true;
    MenuCamera->GetCameraComponent()->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
    // The dark wheelhouse drove eye adaptation up and washed the emissive screen out.
    // Fixed exposure and no lens effects keep the canvas colours as authored.
    auto& PP=MenuCamera->GetCameraComponent()->PostProcessSettings;
    PP.bOverride_AutoExposureMethod=true;PP.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
    PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=0;
    PP.bOverride_LocalExposureHighlightContrastScale=true;PP.LocalExposureHighlightContrastScale=1;
    PP.bOverride_LocalExposureShadowContrastScale=true;PP.LocalExposureShadowContrastScale=1;
    PP.bOverride_BloomIntensity=true;PP.BloomIntensity=0;
    PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=0;
    PP.bOverride_FilmGrainIntensity=true;PP.FilmGrainIntensity=0;
    PP.bOverride_LensFlareIntensity=true;PP.LensFlareIntensity=0;
    PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
    PP.bOverride_DepthOfFieldFocalDistance=true;PP.DepthOfFieldFocalDistance=0; // 0 disables depth of field.
    PP.bOverride_SceneFringeIntensity=true;PP.SceneFringeIntensity=0;
    GetWorld()->bIsCameraMoveableWhenPaused=true;
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/BridgeMenu/M_BridgeTerminal.M_BridgeTerminal"));
    Display=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),LogicalW*TargetScale,LogicalH*TargetScale);
    if(Display && Material)
    {
        Display->ClearColor=FLinearColor(.010f,.022f,.026f,1);
        Display->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::Paint);
        auto* MID=UMaterialInstanceDynamic::Create(Material,this);MID->SetTextureParameterValue(TEXT("Screen"),Display);
        Screen->SetMaterial(0,MID);Refresh();
    }
    UE_LOG(LogTemp,Display,TEXT("BRIDGE_TERMINAL created owner_local=1 display=%d material=%d"),Display!=nullptr,Material!=nullptr);
}
void ACalmyBridgeTerminal::Open()
{
    Page=0;Selected=0;
    bHoverArmed=false;
    bMouseBaseline=FSlateApplication::IsInitialized();
    if(bMouseBaseline)LastMouse=FVector2D(FSlateApplication::Get().GetCursorPos());
    FitCamera();
    Refresh();
}
void ACalmyBridgeTerminal::Refresh()
{
    if(!Display)return;
    Display->UpdateResource();
    // Canvas content has no motion vectors: reset temporal AA history so the previous page never ghosts.
    if(PC&&PC->PlayerCameraManager&&MenuCamera&&PC->GetViewTarget()==MenuCamera)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
}
void ACalmyBridgeTerminal::FitCamera()
{
    using namespace CalmyTerminal;
    if(!PC||!MenuCamera)return;
    int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);if(VX<=0||VY<=0)return;
    // Whole face plus a thin bezel margin on any aspect: width-limited up to ~2.3:1, height-limited beyond.
    const float VisibleW=FMath::Max(ScreenCmW*1.12f,ScreenCmH*1.16f*float(VX)/float(VY));
    const float Fov=FMath::RadiansToDegrees(2*FMath::Atan(VisibleW*.5f/CameraDistanceCm));
    auto* Camera=MenuCamera->GetCameraComponent();
    if(!FMath::IsNearlyEqual(Camera->FieldOfView,Fov,.01f))Camera->SetFieldOfView(Fov);
}
int32 ACalmyBridgeTerminal::RowAt(const FVector2D& P,int32 N)
{
    using namespace CalmyTerminal;
    if(!FMath::IsFinite(P.X)||!FMath::IsFinite(P.Y)||P.X<PadX||P.X>LogicalW-PadX||P.Y<RowTop)return INDEX_NONE;
    const int32 R=FMath::FloorToInt((P.Y-RowTop)/RowPitch);
    return R>=0&&R<N&&P.Y-(RowTop+R*RowPitch)<=RowH?R:INDEX_NONE;
}
FVector ACalmyBridgeTerminal::RowWorld(int32 Row) const
{
    using namespace CalmyTerminal;
    return GetActorTransform().TransformPosition(FVector(0,0,ScreenCmH*.5f-(RowTop+RowH*.5f+Row*RowPitch)*ScreenCmH/LogicalH));
}
int32 ACalmyBridgeTerminal::MouseRow() const
{
    using namespace CalmyTerminal;
    float X,Y;FVector O,D;if(!PC||!PC->GetMousePosition(X,Y)||!PC->DeprojectScreenPositionToWorld(X,Y,O,D))return INDEX_NONE;
    O=GetActorTransform().InverseTransformPosition(O);D=GetActorTransform().InverseTransformVectorNoScale(D);
    if(FMath::Abs(D.X)<.0001f)return INDEX_NONE;
    const float T=-O.X/D.X;if(T<=0)return INDEX_NONE;
    const FVector P=O+D*T;
    return RowAt(FVector2D((P.Y+ScreenCmW*.5f)*LogicalW/ScreenCmW,(ScreenCmH*.5f-P.Z)*LogicalH/ScreenCmH),Count());
}
void ACalmyBridgeTerminal::Hover()
{
    FitCamera();
    // Hover follows a deliberate cursor move only. Opening the menu under a parked cursor, tiny
    // jitter, or arrow-key navigation must not steal Begin Watch into Replay/Leave.
    if(!FSlateApplication::IsInitialized())return;
    const FVector2D Cursor(FSlateApplication::Get().GetCursorPos());
    if(!bMouseBaseline){bMouseBaseline=true;LastMouse=Cursor;return;}
    constexpr float ArmPixels=10.f;
    if(!bHoverArmed)
    {
        if(FVector2D::Distance(Cursor,LastMouse)<ArmPixels)return;
        bHoverArmed=true;
    }
    if(Cursor.Equals(LastMouse,1.5f))return;
    LastMouse=Cursor;
    const int32 R=MouseRow();if(R!=INDEX_NONE&&Selected!=R){Selected=R;Refresh();}
}
void ACalmyBridgeTerminal::Click(){const int32 R=MouseRow();if(R!=INDEX_NONE){Selected=R;FCalmyAudio::Play2D(this,ECalmySoundCue::UiConfirm);Activate();}}
void ACalmyBridgeTerminal::Step(int32 Direction)
{
    // Keyboard owns the highlight: disarm hover until the mouse moves again.
    bHoverArmed=false;
    if(FSlateApplication::IsInitialized())LastMouse=FVector2D(FSlateApplication::Get().GetCursorPos());
    Selected=(Selected+Direction+Count())%Count();
    FCalmyAudio::Play2D(this,ECalmySoundCue::UiSelect);
    Refresh();
}
void ACalmyBridgeTerminal::Activate()
{
    if(!PC||!PC->IsPauseMenuOpen()||PC->IsJoinDialogOpen())return;
    FCalmyAudio::Play2D(this,ECalmySoundCue::UiConfirm);
    if(Page==0)
    {
        // Enter always uses the keyboard/selected row, never a silent hover steal.
        if(Selected==0)
        {
            if(const auto* Voyage=ACalmyVoyageDirector::Find(GetWorld()))
            {
                if(Voyage->State.Phase==ECalmyVoyagePhase::Briefing)
                {
                    PC->RequestBeginVoyage();
                    PC->ResumeFromMenu();
                    return;
                }
                if(Voyage->State.Phase==ECalmyVoyagePhase::Outcome)
                {
                    PC->RequestReturnToPort();
                    PC->ResumeFromMenu();
                    return;
                }
            }
            PC->ResumeFromMenu();
            return;
        }
        if(Selected==1){PC->OpenSettings();return;}
        if(Selected==2){PC->RequestReplayTutorial();return;}
        if(Selected==3){Page=3;Selected=0;bHoverArmed=false;Refresh();return;}
        Page=2;Selected=0;bHoverArmed=false;
    }
    else if(Page==1)
    {
        if(Selected==0)PC->MenuCamera();else if(Selected==1)PC->MenuMotion();
        else if(Selected==2)PC->MenuLookDown();else if(Selected==3)PC->MenuLookUp();else {Page=0;Selected=0;}
    }
    else if(Page==3)
    {
        if(Selected==0)
        {
            FString Error;
            if(!PC->HostLanSession(&Error) && !Error.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("CALMY_SESSION host_ui %s"), *Error);
            }
            Refresh();
            return;
        }
        if(Selected==1){PC->OpenJoinDialog();return;}
        if(Selected==2){PC->CopyLanEndpointToClipboard();Refresh();return;}
        Page=0;Selected=3;
    }
    else if(Selected==0){Page=0;Selected=0;}
    else UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false);
    Refresh();
}
void ACalmyBridgeTerminal::Paint(UCanvas* C,int32 W,int32 H)
{
    using namespace CalmyTerminal;
    if(!C||!PC||!GEngine)return;
    const float S=W/LogicalW;
    const UFont* Face=GEngine->GetMediumFont();
    auto Font=[&](float Size,const TCHAR* Typeface,int32 Spacing){FSlateFontInfo F(Face,Size*S,FName(Typeface));F.LetterSpacing=Spacing;return F;};
    auto Rect=[&](float X,float Y,float SX,float SY,const FLinearColor& Color)
    {FCanvasTileItem T(FVector2D(X*S,Y*S),FVector2D(SX*S,SY*S),Color);T.BlendMode=SE_BLEND_Opaque;C->DrawItem(T);};
    auto Text=[&](const FString& Str,float X,float Y,const FSlateFontInfo& F,const FLinearColor& Color,bool bRightAligned=false)
    {
        if(Str.IsEmpty())return;
        FVector2D Pos(X*S,Y*S);
        if(bRightAligned&&FSlateApplication::IsInitialized()&&FSlateApplication::Get().GetRenderer())
            Pos.X-=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str,F).X;
        FCanvasTextItem T(Pos,FText::FromString(Str),F,Color);T.BlendMode=SE_BLEND_Translucent;C->DrawItem(T);
    };
    const FLinearColor Background(.010f,.022f,.026f),Rule(.075f,.16f,.17f),Bright(.82f,.93f,.90f),Body(.46f,.62f,.60f),
        Muted(.20f,.32f,.32f),Accent(.28f,.88f,.70f),SelectedFill(.035f,.13f,.13f);

    Rect(0,0,LogicalW,LogicalH,Background);
    Text(TEXT("CALMY  /  NIGHT WATCH"),PadX,26,Font(22,TEXT("Bold"),140),Bright);
    const FString Subtitle =
        Page==1?TEXT("BODYCAM CONFIGURATION"):
        Page==2?TEXT("LEAVE THIS SESSION?"):
        Page==3?TEXT("CREW SESSION  /  LAN"):
        TEXT("WHEELHOUSE  /  CREW TERMINAL 01");
    Text(Subtitle,PadX,66,Font(11,TEXT("Regular"),80),Body);
    Text(PC->IsSoloPause()?TEXT("SOLO  ·  WORLD PAUSED"):TEXT("ONLINE  ·  WORLD LIVE"),LogicalW-PadX,32,Font(10,TEXT("Bold"),80),Accent,true);
    Rect(PadX,96,LogicalW-2*PadX,2,Rule);

    const bool bClient=PC->GetNetMode()==NM_Client;
    TArray<FRow> Rows;
    auto* P=Cast<ACalmyCharacter>(PC->GetPawn());
    const auto* Voyage=ACalmyVoyageDirector::Find(GetWorld());
    if(Page==0)
    {
        FRow Primary{TEXT("CONTINUE WATCH"),TEXT("ПРОДОЛЖИТЬ")};
        if(Voyage && Voyage->State.Phase==ECalmyVoyagePhase::Briefing) Primary={TEXT("BEGIN WATCH NOW"),bClient?TEXT("HOST ONLY"):TEXT("НАЧАТЬ ВАХТУ"),!bClient};
        else if(Voyage && Voyage->State.Phase==ECalmyVoyagePhase::Outcome) Primary={TEXT("RETURN TO PORT"),bClient?TEXT("HOST ONLY"):TEXT("В ПОРТ"),!bClient};
        Rows={Primary,{TEXT("SETTINGS"),TEXT("НАСТРОЙКИ")},
            {TEXT("REPLAY TRAINING"),bClient?TEXT("HOST ONLY"):TEXT("ОБУЧЕНИЕ СНОВА"),!bClient},
            {TEXT("CREW SESSION"),TEXT("ЭКИПАЖ")},{TEXT("LEAVE SESSION"),TEXT("ВЫЙТИ")}};
    }
    else if(Page==2)Rows={{TEXT("STAY ON BOARD"),TEXT("ОСТАТЬСЯ")},{TEXT("EXIT TO DESKTOP"),TEXT("ПОДТВЕРДИТЬ ВЫХОД")}};
    else if(Page==3)
    {
        const bool Listening=PC->GetNetMode()==NM_ListenServer;
        Rows={
            {Listening?TEXT("LAN LISTEN ACTIVE"):TEXT("HOST LAN WATCH"),Listening?TEXT("ONLINE"):TEXT("")},
            {TEXT("JOIN LAN"),FCalmySession::LoadSavedJoinAddress()},
            {TEXT("COPY ADDRESS"),Listening?FCalmySession::DescribeListenEndpoint(PC->GetWorld()):TEXT("HOST FIRST"),Listening},
            {TEXT("BACK"),TEXT("")}};
    }
    else Rows={
        {TEXT("CAMERA"),FString::Printf(TEXT("%s    V"),P&&P->IsSteadyCamera()?TEXT("STEADY"):TEXT("BODYCAM"))},
        {TEXT("MOTION"),FString::Printf(TEXT("%.0f%%    M"),P?P->GetCameraMotionScale()*100:0)},
        {TEXT("LOOK SENSITIVITY  −"),FString::Printf(TEXT("%.2fx    ["),P?P->GetLookSensitivity():1)},
        {TEXT("LOOK SENSITIVITY  +"),TEXT("]")},
        {TEXT("BACK"),TEXT("")}};

    for(int32 I=0;I<Rows.Num();++I)
    {
        const float Y=RowTop+I*RowPitch;const bool bSelected=I==Selected;
        if(bSelected){Rect(PadX,Y,LogicalW-2*PadX,RowH,SelectedFill);Rect(PadX,Y,5,RowH,Accent);}
        const FLinearColor LabelColor=!Rows[I].bEnabled?Muted:bSelected?Bright:Body;
        Text(Rows[I].Label,PadX+26,Y+9,Font(16,bSelected?TEXT("Bold"):TEXT("Regular"),60),LabelColor);
        Text(Rows[I].Value,LogicalW-PadX-22,Y+12,Font(12,TEXT("Regular"),40),!Rows[I].bEnabled?Muted:bSelected?Accent:Muted,true);
    }

    Rect(PadX,372,LogicalW-2*PadX,2,Rule);
    const FSlateFontInfo Footer=Font(10,TEXT("Regular"),60);
    Text(TEXT("ARROWS / MOUSE  SELECT     ENTER  CONFIRM"),PadX,384,Footer,Body);
    Text(Page==3?FCalmySession::StatusLine(PC->GetWorld()):FString(TEXT("ESC  BACK TO DECK")),LogicalW-PadX,384,Footer,Body,true);
}
