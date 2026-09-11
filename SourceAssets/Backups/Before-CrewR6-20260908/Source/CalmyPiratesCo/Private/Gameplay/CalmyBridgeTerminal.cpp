#include "Gameplay/CalmyBridgeTerminal.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"

ACalmyBridgeTerminal::ACalmyBridgeTerminal()
{
    PrimaryActorTick.bCanEverTick=false;
    bReplicates=false;
    Screen=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerminalScreen"));SetRootComponent(Screen);
    Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetCastShadow(false);
}
void ACalmyBridgeTerminal::Initialize(ACalmyPlayerController* Controller)
{
    PC=Controller;SetOwner(PC);
    // Thin screen skin over the existing physical bridge console; no new obstacle.
    SetActorLocation(FVector(-3374,-210,1988));
    TArray<FVector> V={{0,-50,23},{0,50,23},{0,50,-23},{0,-50,-23}};
    Screen->CreateMeshSection(0,V,{0,1,2,0,2,3},{},{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)},{},{},false);
    Screen->SetOnlyOwnerSee(true);
    MenuCamera=GetWorld()->SpawnActor<ACameraActor>();
    MenuCamera->SetOwner(PC);MenuCamera->SetActorLocation(FVector(-3500,-226,2020));
    MenuCamera->SetActorRotation((GetActorLocation()-MenuCamera->GetActorLocation()).Rotation());
    MenuCamera->GetCameraComponent()->SetFieldOfView(56);
    MenuCamera->GetCameraComponent()->bConstrainAspectRatio=false;
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/BridgeMenu/M_BridgeTerminal.M_BridgeTerminal"));
    Display=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),1024,512);
    if(Display && Material)
    {
        Display->ClearColor=FLinearColor(.006,.012,.016,1);
        Display->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::Paint);
        auto* MID=UMaterialInstanceDynamic::Create(Material,this);MID->SetTextureParameterValue(TEXT("Screen"),Display);
        Screen->SetMaterial(0,MID);Refresh();
    }
    UE_LOG(LogTemp,Display,TEXT("BRIDGE_TERMINAL created owner_local=1 display=%d material=%d"),Display!=nullptr,Material!=nullptr);
}
void ACalmyBridgeTerminal::Open(){Page=0;Selected=0;LastMouse=FVector2D(-1,-1);Refresh();}
void ACalmyBridgeTerminal::Refresh(){if(Display)Display->UpdateResource();}
int32 ACalmyBridgeTerminal::RowAt(const FVector2D& P,int32 N)
{
    if(!FMath::IsFinite(P.X)||!FMath::IsFinite(P.Y)||P.X<46||P.X>978||P.Y<176)return INDEX_NONE;
    const int32 R=FMath::FloorToInt((P.Y-176)/52);
    return R>=0&&R<N&&P.Y-(176+R*52)<=44?R:INDEX_NONE;
}
FVector ACalmyBridgeTerminal::RowWorld(int32 Row) const
{return GetActorTransform().TransformPosition(FVector(0,0,23-(198+Row*52)*46.f/512));}
int32 ACalmyBridgeTerminal::MouseRow() const
{
    float X,Y;FVector O,D;if(!PC||!PC->GetMousePosition(X,Y)||!PC->DeprojectScreenPositionToWorld(X,Y,O,D))return INDEX_NONE;
    O=GetActorTransform().InverseTransformPosition(O);D=GetActorTransform().InverseTransformVectorNoScale(D);
    if(FMath::Abs(D.X)<.0001f)return INDEX_NONE;
    const float T=-O.X/D.X;if(T<=0)return INDEX_NONE;
    const FVector P=O+D*T;return RowAt(FVector2D((P.Y+50)*1024/100,(23-P.Z)*512/46),Count());
}
void ACalmyBridgeTerminal::Hover()
{
    float X,Y;if(!PC->GetMousePosition(X,Y))return;
    const FVector2D Pos(X,Y);if(Pos.Equals(LastMouse,.1f))return;LastMouse=Pos;
    const int32 R=MouseRow();if(R!=INDEX_NONE&&Selected!=R){Selected=R;Refresh();}
}
void ACalmyBridgeTerminal::Click(){const int32 R=MouseRow();if(R!=INDEX_NONE){Selected=R;Activate();}}
void ACalmyBridgeTerminal::Step(int32 Direction){Selected=(Selected+Direction+Count())%Count();Refresh();}
void ACalmyBridgeTerminal::Activate()
{
    if(!PC||!PC->IsPauseMenuOpen())return;
    if(Page==0)
    {
        if(Selected==0){PC->ResumeFromMenu();return;}
        Page=Selected==1?1:2;Selected=0;
    }
    else if(Page==1)
    {
        if(Selected==0)PC->MenuCamera();else if(Selected==1)PC->MenuMotion();
        else if(Selected==2)PC->MenuLookDown();else if(Selected==3)PC->MenuLookUp();else {Page=0;Selected=0;}
    }
    else if(Selected==0){Page=0;Selected=0;}
    else UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false);
    Refresh();
}
void ACalmyBridgeTerminal::Paint(UCanvas* C,int32 W,int32 H)
{
    if(!C||!PC)return;
    auto Rect=[&](float X,float Y,float SX,float SY,FLinearColor Color){FCanvasTileItem T(FVector2D(X,Y),FVector2D(SX,SY),Color);T.BlendMode=SE_BLEND_Opaque;C->DrawItem(T);};
    auto Text=[&](const FString& S,float X,float Y,float Scale,FLinearColor Color){FCanvasTextItem T(FVector2D(X,Y),FText::FromString(S),GEngine->GetMediumFont(),Color);T.Scale=FVector2D(Scale);C->DrawItem(T);};
    const FLinearColor White(.72,.82,.78),Dim(.27,.48,.48),Accent(.38,.82,.7);
    Rect(0,0,W,H,FLinearColor(.009,.02,.024));Rect(46,128,932,1,Dim);
    Text(TEXT("C A L M Y  /  NIGHT WATCH"),48,38,1.65,White);
    Text(Page==1?TEXT("BODYCAM CONFIGURATION"):Page==2?TEXT("LEAVE THIS SESSION?"):TEXT("WHEELHOUSE  /  CREW TERMINAL  01"),48,94,.95,Dim);
    TArray<FString> Labels;
    auto* P=Cast<ACalmyCharacter>(PC->GetPawn());
    if(Page==0)Labels={TEXT("CONTINUE WATCH"),TEXT("CAMERA & CONTROLS"),TEXT("LEAVE SESSION")};
    else if(Page==2)Labels={TEXT("STAY ON BOARD"),TEXT("CONFIRM / EXIT TO DESKTOP")};
    else Labels={FString::Printf(TEXT("CAMERA       %s   [V]"),P&&P->IsSteadyCamera()?TEXT("STEADY"):TEXT("BODYCAM")),FString::Printf(TEXT("MOTION       %.0f%%   [M]"),P?P->GetCameraMotionScale()*100:0),FString::Printf(TEXT("LOOK -       %.2fx   [[]"),P?P->GetLookSensitivity():1),TEXT("LOOK +       []]"),TEXT("BACK")};
    for(int32 I=0;I<Labels.Num();++I)
    {
        const float Y=176+I*52;
        if(I==Selected){Rect(46,Y,932,44,FLinearColor(.04,.11,.115));Rect(46,Y,3,44,Accent);}
        Text(Labels[I],68,Y+10,1.18,I==Selected?White:Dim);
    }
    Rect(46,457,932,1,Dim);
    Text(PC->IsSoloPause()?TEXT("SOLO / WORLD PAUSED"):TEXT("ONLINE / WORLD REMAINS LIVE"),48,476,.78,Accent);
    Text(TEXT("MOUSE / ARROWS / ENTER     ESC BACK TO DECK"),475,476,.69,Dim);
}
