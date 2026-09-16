#include "Gameplay/CalmyBridgeDisplays.h"
#include "Gameplay/CalmyScreenPainter.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyLeak.h"
#include "Gameplay/CalmyOcean.h"
#include "Gameplay/CalmyShift.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Gameplay/FlySwarmMob.h"
#include "Core/CalmySession.h"
#include "ProceduralMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "EngineUtils.h"

namespace CalmyBridgeScreens
{
    // Ship-local ScreenFace centres of the free R7 bridge consoles; the menu terminal owns Y -210.
    // Order: ALARMS, CREW (behind the helm wheel, least urgent), WATCH (clear view), SEA.
    constexpr float FaceX=-3373,FaceZ=1988,FaceW=94,FaceH=40;
    constexpr float ConsoleY[]={-425,0,210,425};
    // Seen through the gameplay camera, eye adaptation in the dark wheelhouse brightens emissive screens.
    constexpr float ScreenGain=.45f;
    constexpr float LogicalW=1024,LogicalH=436,TargetScale=1.5f;
    const FLinearColor Background(.010f,.022f,.026f),Rule(.075f,.16f,.17f),Bright(.82f,.93f,.90f),Body(.46f,.62f,.60f),
        Muted(.26f,.38f,.38f),Accent(.28f,.88f,.70f),Amber(1.f,.74f,.28f),Red(1.f,.36f,.28f);

    void Header(const FCalmyScreenPainter& P,const TCHAR* Title,const FString& Right)
    {
        P.Rect(0,0,LogicalW,LogicalH,Background);
        P.Text(Title,40,26,P.Font(22,true,140),Bright);
        P.Text(Right,LogicalW-40,34,P.Font(11,true,80),Accent,true);
        P.Rect(40,78,LogicalW-80,2,Rule);
    }
    void Hero(const FCalmyScreenPainter& P,const FString& Title,const FString& Sub,const FLinearColor& Color)
    {
        P.Text(Title,40,92,P.Font(34,true,40),Color);
        P.Text(Sub,40,150,P.Font(13,false,20),Body);
    }
    void Row(const FCalmyScreenPainter& P,int32 Index,const FString& Label,const FString& Value,const FLinearColor& ValueColor)
    {
        const float Y=196+Index*52;
        P.Rect(40,Y-10,LogicalW-80,1,Rule);
        P.Text(Label,40,Y+6,P.Font(12,false,60),Body);
        P.Text(Value,LogicalW-40,Y,P.Font(20,true,20),ValueColor,true);
    }
}

ACalmyBridgeDisplays::ACalmyBridgeDisplays()
{
    PrimaryActorTick.bCanEverTick=false;
    bReplicates=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    for(int32 I=0;I<UE_ARRAY_COUNT(CalmyBridgeScreens::ConsoleY);++I)
    {
        auto* Screen=CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("StatusScreen%d"),I));
        Screen->SetupAttachment(GetRootComponent());
        Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetCastShadow(false);
        Screens.Add(Screen);
    }
}
void ACalmyBridgeDisplays::Initialize(ACalmyPlayerController* Controller)
{
    using namespace CalmyBridgeScreens;
    PC=Controller;SetOwner(PC);
    // Same frame as the menu terminal: ship-local faces, attached so heave, heel and drift carry them.
    auto* Ship=ACalmyShipMotion::Find(GetWorld());
    SetActorTransform(Ship?Ship->GetActorTransform():FTransform::Identity);
    if(Ship)AttachToActor(Ship,FAttachmentTransformRules::KeepWorldTransform);
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/BridgeMenu/M_BridgeTerminal.M_BridgeTerminal"));
    const float HW=FaceW*.5f,HH=FaceH*.5f;
    const TArray<FVector> Vertices={{0,-HW,HH},{0,HW,HH},{0,HW,-HH},{0,-HW,-HH}};
    const TArray<int32> Triangles={0,1,2,0,2,3};
    const TArray<FVector2D> UVs={FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)};
    for(int32 I=0;I<Screens.Num();++I)
    {
        UProceduralMeshComponent* Screen=Screens[I];
        Screen->SetRelativeLocation(FVector(FaceX,ConsoleY[I],FaceZ));
        Screen->CreateMeshSection(0,Vertices,Triangles,{},UVs,{},{},false);
        auto* Target=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),LogicalW*TargetScale,LogicalH*TargetScale);
        if(!Target||!Material)continue;
        Target->ClearColor=FLinearColor(Background.R*ScreenGain,Background.G*ScreenGain,Background.B*ScreenGain,1);
        switch(I)
        {
        case 0:Target->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::PaintAlarms);break;
        case 1:Target->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::PaintCrew);break;
        case 2:Target->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::PaintWatch);break;
        default:Target->OnCanvasRenderTargetUpdate.AddDynamic(this,&ThisClass::PaintSea);break;
        }
        auto* MID=UMaterialInstanceDynamic::Create(Material,this);MID->SetTextureParameterValue(TEXT("Screen"),Target);
        Screen->SetMaterial(0,MID);
        Targets.Add(Target);
    }
    Refresh();
    GetWorldTimerManager().SetTimer(RefreshTimer,this,&ThisClass::Refresh,.5f,true);
    UE_LOG(LogTemp,Display,TEXT("BRIDGE_DISPLAYS created screens=%d material=%d"),Targets.Num(),Material!=nullptr);
}
void ACalmyBridgeDisplays::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(RefreshTimer);
    Super::EndPlay(EndPlayReason);
}
void ACalmyBridgeDisplays::Refresh()
{
    for(UCanvasRenderTarget2D* Target:Targets)if(Target)Target->UpdateResource();
}
void ACalmyBridgeDisplays::PaintAlarms(UCanvas* C,int32 W,int32 H)
{
    using namespace CalmyBridgeScreens;
    if(!C||!GEngine)return;
    const FCalmyScreenPainter P(C,LogicalW,ScreenGain);
    const auto* Leak=ACalmyLeak::Find(GetWorld());
    Header(P,TEXT("ALARMS"),Leak?FString::Printf(TEXT("EVENT %d"),Leak->State.EventSerial):FString(TEXT("NO SENSORS")));
    if(!Leak){Hero(P,TEXT("ALL CLEAR"),TEXT("No leak sensors on this deck."),Accent);return;}
    const auto& L=Leak->State;
    switch(L.Phase)
    {
    // The valve moved with the R7 rework: name the bilge valve itself, not a place it no longer stands in.
    case ECalmyLeakPhase::Warning:Hero(P,TEXT("LEAK WARNING"),FString::Printf(TEXT("Hull breach in %d s. Get to the bilge valve."),L.WarningSeconds),Amber);break;
    case ECalmyLeakPhase::Leaking:Hero(P,FString::Printf(TEXT("LEAK  %d%%"),L.Water),L.Repairer?TEXT("Valve under repair. Empty hands, stay still."):TEXT("Hold F at the red valve with empty hands."),Red);break;
    case ECalmyLeakPhase::Draining:Hero(P,TEXT("SEALED"),FString::Printf(TEXT("Pumps draining. Water %d%%."),L.Water),Accent);break;
    case ECalmyLeakPhase::Resolved:Hero(P,TEXT("REPAIRED"),L.CooldownSeconds>0?FString::Printf(TEXT("Deck dry. Next check in %d s."),L.CooldownSeconds):FString(TEXT("Deck dry. Drill limit reached.")),Accent);break;
    case ECalmyLeakPhase::Suspended:Hero(P,TEXT("STANDBY"),TEXT("Leak drills stop with the contract."),Muted);break;
    default:Hero(P,TEXT("ALL CLEAR"),FString::Printf(TEXT("Next drill in about %d s."),L.NextEventSeconds),Accent);break;
    }
    Row(P,0,TEXT("WATER"),FString::Printf(TEXT("%d%%"),L.Water),L.Water>=50?Red:L.Water>0?Amber:Bright);
    Row(P,1,TEXT("REPAIR"),FString::Printf(TEXT("%d%%"),L.Repair),Bright);
    Row(P,2,TEXT("POWER"),L.bPowerDegraded?TEXT("EMERGENCY"):TEXT("MAIN"),L.bPowerDegraded?Amber:Bright);
    Row(P,3,TEXT("DECK"),Leak->GetDeckSpeedScale()<1?TEXT("FLOODED  -30% SPEED"):TEXT("DRY"),Leak->GetDeckSpeedScale()<1?Red:Bright);
}
void ACalmyBridgeDisplays::PaintWatch(UCanvas* C,int32 W,int32 H)
{
    using namespace CalmyBridgeScreens;
    if(!C||!GEngine)return;
    const FCalmyScreenPainter P(C,LogicalW,ScreenGain);
    const auto* Shift=ACalmyShift::Find(GetWorld());
    const auto* Voyage=ACalmyVoyageDirector::Find(GetWorld());
    Header(P,TEXT("WATCH"),Voyage?FString::Printf(TEXT("ROUND %d"),Voyage->State.Round):FString(TEXT("NO CONTRACT")));
    if(!Shift){Hero(P,TEXT("STANDBY"),TEXT("No active contract on this ship."),Muted);return;}
    const auto& S=Shift->State;
    if(Voyage&&Voyage->State.Phase==ECalmyVoyagePhase::Briefing)
    {
        FString Head,Tail;const FString Briefing=Voyage->BriefingText();
        Hero(P,Briefing.Split(TEXT(" | "),&Head,&Tail)?Head.Replace(TEXT("WATCH "),TEXT("")):FString(TEXT("BRIEFING")),TEXT("Host starts the watch from the bridge menu, or it starts by itself."),Amber);
    }
    else if(Voyage&&Voyage->State.Phase==ECalmyVoyagePhase::Outcome)
        Hero(P,Voyage->State.bShiftFailed?TEXT("WATCH FAILED"):TEXT("WATCH COMPLETE"),TEXT("Host: R or NEW WATCH in the bridge menu."),Voyage->State.bShiftFailed?Red:Accent);
    else if(S.Phase==ECalmyShiftPhase::Complete)Hero(P,TEXT("CONTRACT PAID"),TEXT("Host: R for a new contract."),Accent);
    else if(S.Phase==ECalmyShiftPhase::Failed)Hero(P,TEXT("CONTRACT FAILED"),TEXT("Time expired. Host: R to retry."),Red);
    else if(S.Phase==ECalmyShiftPhase::InvalidManifest)Hero(P,TEXT("MANIFEST INVALID"),TEXT("A manifest crate is missing."),Red);
    else Hero(P,FString::Printf(TEXT("%02d:%02d"),S.SecondsRemaining/60,S.SecondsRemaining%60),TEXT("Deliver the manifest to the green zone. Repair leaks with F."),S.SecondsRemaining<60?Amber:Bright);
    Row(P,0,TEXT("DELIVERED"),FString::Printf(TEXT("%d / %d"),S.Delivered,S.Required),S.Delivered>=S.Required?Accent:Bright);
    Row(P,1,TEXT("MANIFEST"),Voyage?FString::Printf(TEXT("S%d  B%d  F%d  U%d   ·   HOLD 1"),Voyage->State.RequiredStandard,Voyage->State.RequiredBulky,Voyage->State.RequiredFragile,Voyage->State.RequiredUnstable):FString(TEXT("ALL CRATES")),Bright);
    Row(P,2,TEXT("FUEL  ·  STORM"),Voyage?FString::Printf(TEXT("%d%%   ·   %d%%"),Voyage->State.FuelPercent,Voyage->State.StormPercent):FString(TEXT("--")),
        Voyage&&Voyage->State.FuelPercent<25?Amber:Bright);
    Row(P,3,TEXT("PAY  ·  DAMAGED"),S.Phase==ECalmyShiftPhase::Complete?FString::Printf(TEXT("$%d · %d/3 · dmg %d"),S.FinalPayout,S.PerformanceStars,S.DamagedDelivered):FString::Printf(TEXT("$1200 + SPEED · dmg %d"),S.DamagedDelivered),S.DamagedDelivered>0?Amber:Bright);
}
void ACalmyBridgeDisplays::PaintCrew(UCanvas* C,int32 W,int32 H)
{
    using namespace CalmyBridgeScreens;
    if(!C||!GEngine)return;
    const FCalmyScreenPainter P(C,LogicalW,ScreenGain);
    const UWorld* World=GetWorld();
    Header(P,TEXT("CREW"),FCalmySession::RoleLabel(World));
    Hero(P,FString::Printf(TEXT("%d / %d ABOARD"),FCalmySession::CountCrew(World),FCalmySession::MaxCrew),FCalmySession::StatusLine(World),Bright);
    int32 Index=0;
    if(const auto* GS=World->GetGameState())
        for(const APlayerState* PS:GS->PlayerArray)
        {
            const auto* Pawn=PS?Cast<ACalmyCharacter>(PS->GetPawn()):nullptr;
            if(!Pawn||Index>=3)continue;
            const FString State=StaticEnum<ECalmyPlayerLifecycleState>()->GetNameStringByValue(static_cast<int64>(Pawn->Lifecycle)).ToUpper();
            Row(P,Index++,PS->GetPlayerName().Left(20).ToUpper(),FString::Printf(TEXT("HP %.0f   ·   %s%s"),Pawn->Health,*State,Pawn->HeldCargo?TEXT("   ·   CARGO"):TEXT("")),
                Pawn->Lifecycle==ECalmyPlayerLifecycleState::Controlled?Bright:Amber);
        }
    for(TActorIterator<ACalmyCrewHelper> It(World);It&&Index<4;++It)
    {
        Row(P,Index++,TEXT("MISHA  ·  CREW MATE"),StaticEnum<ECalmyHelperState>()->GetNameStringByValue(static_cast<int64>(It->Assistance)).ToUpper(),Accent);
        break;
    }
}
void ACalmyBridgeDisplays::PaintSea(UCanvas* C,int32 W,int32 H)
{
    using namespace CalmyBridgeScreens;
    if(!C||!GEngine)return;
    const FCalmyScreenPainter P(C,LogicalW,ScreenGain);
    const auto* Ocean=ACalmyOcean::Find(GetWorld());
    const auto* Ship=ACalmyShipMotion::Find(GetWorld());
    Header(P,TEXT("SEA  ·  RADAR"),Ship&&Ship->StormBoost>0?FString::Printf(TEXT("STORM +%.0f%%"),Ship->StormBoost*100):FString(TEXT("HARBOUR SWELL")));
    const float Strength=Ocean?ACalmyOcean::SeaStrength(Ocean->WaveTime()):0;
    Hero(P,Strength>1.8f?TEXT("STRONG SWELL"):Strength>1.f?TEXT("CHANGING SWELL"):TEXT("CALM INTERVAL"),
        Strength>1.8f?TEXT("Current pushes swimmers and loose cargo."):Strength>1.f?TEXT("Watch the waves before entering the water."):TEXT("Stronger waves arrive in sets."),
        Strength>1.8f?Red:Strength>1.f?Amber:Accent);
    const auto* SeaVoyage=ACalmyVoyageDirector::Find(GetWorld());
    if(SeaVoyage&&SeaVoyage->State.bFuelStationArmed)
        Row(P,0,TEXT("FUEL   ·   STORM"),FString::Printf(TEXT("%d%%   ·   %d%%"),SeaVoyage->State.FuelPercent,SeaVoyage->State.StormPercent),SeaVoyage->State.FuelPercent<=25?Amber:Bright);
    else Row(P,0,TEXT("SEA STRENGTH"),FString::Printf(TEXT("%.2f"),Strength),Bright);
    if(Ship)
    {
        float Heave=0,Heel=0,Drift=0;Ship->GetEffectiveMotionLimits(Heave,Heel,Drift);
        const FRotator Tilt=(Ship->GetActorRotation()-Ship->GetRestTransform().Rotator()).GetNormalized();
        Row(P,1,TEXT("HEEL"),FString::Printf(TEXT("%+.1f°   /   ±%.1f°"),Tilt.Roll,Heel),FMath::Abs(Tilt.Roll)>Heel*.8f?Amber:Bright);
        Row(P,2,TEXT("HEAVE"),FString::Printf(TEXT("%+.0f cm   /   ±%.0f cm"),Ship->GetActorLocation().Z-Ship->GetRestTransform().GetLocation().Z,Heave),Bright);
    }
    for(TActorIterator<AFlySwarmMob> It(GetWorld());It;++It)
    {
        Row(P,3,TEXT("SHIP RESIDENT"),FFlyCognition::ActivityLabel(It->Activity),Accent);
        break;
    }
}
