#include "Testing/CalmyHudReviewProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyPreferences.h"
#include "Gameplay/CalmySettingsWidget.h"
#include "Gameplay/CalmyShipMotion.h"
#include "Gameplay/CalmyTutorial.h"
#include "Gameplay/CalmyVoyageDirector.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

bool UCalmyHudReviewProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString Value;const auto* W=Cast<UWorld>(Outer);
    return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyHudReview="),Value);
#endif
}
void UCalmyHudReviewProbe::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Completed training normally auto-boards the ship; the review needs the lesson HUD on WaterLab.
    if(GetWorld()&&GetWorld()->GetMapName().Contains(TEXT("WaterLab")))ACalmyTutorial::BeginForcedReplay();
}
void UCalmyHudReviewProbe::Deinitialize()
{
    ACalmyHUD::bReviewCrowded=false;
    ACalmyHUD::bReviewBreakdown=false;
    if(SavedScale>0)UCalmyPreferences::Get()->HUDScale=SavedScale;
    Super::Deinitialize();
}
void UCalmyHudReviewProbe::Finish(bool OK,const FString& Detail)
{
    ACalmyHUD::bReviewCrowded=false;
    ACalmyHUD::bReviewBreakdown=false;
    if(SavedScale>0){UCalmyPreferences::Get()->HUDScale=SavedScale;SavedScale=-1;}
    UE_LOG(LogTemp,Display,TEXT("CALMY_HUD_REVIEW %s %s map=%s"),OK?TEXT("PASS"):TEXT("FAIL"),*Detail,*GetWorld()->GetMapName());GLog->Flush();
    Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyHudReviewProbe::Photo(const FString& Name)
{
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));
    IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),true,false);++Shots;
}
void UCalmyHudReviewProbe::Tick(float DT)
{
    if(Stage==999||!GetWorld()->HasBegunPlay())return;
    if(Label.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyHudReview="),Label);
    const double Now=FPlatformTime::Seconds();if(!Start)Start=Now;
    if(Now-Start>205){Finish(false,FString::Printf(TEXT("timeout stage=%d"),Stage));return;}
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    if(!PC||!P||Now-Start<5)return;
    const bool bShip=GetWorld()->GetMapName().Contains(TEXT("OceanFreighterR7"));
    auto LayoutOk=[&](const TCHAR* What)
    {
        if(ACalmyHUD::bLastLayoutValid)return true;
        Finish(false,FString::Printf(TEXT("layout_invalid at=%s top=%d/%d"),What,ACalmyHUD::LastVisibleTop,ACalmyHUD::LastTopItems));return false;
    };
    // Bridge screen views in the ship frame (consoles at ship-local X -3373; helm chairs end at X -3487).
    auto Aim=[&](float Y,float X,float Fov)
    {
        auto* Ship=ACalmyShipMotion::Find(GetWorld());
        if(!ReviewCamera.IsValid())
        {
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>();
            Camera->GetCameraComponent()->bConstrainAspectRatio=false;
            ReviewCamera=Camera;
        }
        ACameraActor* Camera=ReviewCamera.Get();
        Camera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Camera->SetActorTransform(FTransform(FRotator::ZeroRotator,FVector(X,Y,1992))*(Ship?Ship->GetActorTransform():FTransform::Identity));
        if(Ship)Camera->AttachToActor(Ship,FAttachmentTransformRules::KeepWorldTransform);
        Camera->GetCameraComponent()->SetFieldOfView(Fov);
        PC->SetViewTarget(Camera);
    };
    // Screenshots are captured at the end of the frame: every state change waits for the next stage.
    if(Stage==0){if(PC->IsPauseMenuOpen())PC->ResumeFromMenu();Since=Now;Stage=1;}
    else if(Stage==1&&Now-Since>1.5){if(!LayoutOk(TEXT("base")))return;Photo(bShip?TEXT("01-ship-start"):TEXT("01-tutorial"));Since=Now;Stage=2;}
    else if(Stage==2&&Now-Since>.6)
    {
        if(bShip)if(auto* Voyage=ACalmyVoyageDirector::Find(GetWorld()))Voyage->TryBeginWatch(P);
        Since=Now;Stage=3;
    }
    else if(Stage==3&&Now-Since>2)
    {
        if(!LayoutOk(TEXT("watch")))return;
        if(bShip)Photo(TEXT("02-ship-watch"));
        Since=Now;Stage=bShip?20:30;
    }
    else if(Stage==20&&Now-Since>.6){Aim(-317,-3480,95);Since=Now;Stage=21;}
    else if(Stage==21&&Now-Since>1.2){Photo(TEXT("05-bridge-alarms-menu"));Since=Now;Stage=22;}
    else if(Stage==22&&Now-Since>.6){Aim(0,-3470,62);Since=Now;Stage=23;}
    else if(Stage==23&&Now-Since>1.2){Photo(TEXT("06-bridge-crew"));Since=Now;Stage=24;}
    else if(Stage==24&&Now-Since>.6){Aim(317,-3480,95);Since=Now;Stage=25;}
    else if(Stage==25&&Now-Since>1.2){Photo(TEXT("07-bridge-watch-sea"));Since=Now;Stage=70;}
    // Service spaces S1: camera position and look-at target in ship-local centimetres.
    // Stages 70..97 (not 50..61: 60/61 are the Settings stages and a collision looped the review).
    else if(Stage>=70&&Stage<98)
    {
        struct FShot{FVector Eye,Target;float Fov;const TCHAR* Name;};
        static const FShot SpaceShots[]={
            {FVector(5290,0,900),FVector(5760,-120,780),90,TEXT("09-chain-locker")},
            {FVector(-2240,-420,1240),FVector(-2800,0,1150),80,TEXT("10-service-house")},
            // Blender Y is reflected on import: the office built at Blender Y -6.3..-3.4 m sits at UE Y +340..+630 cm.
            {FVector(-4520,250,2090),FVector(-4870,520,1990),85,TEXT("11-ship-office")},
            {FVector(5600,-3400,1500),FVector(3400,-1000,1100),62,TEXT("12-bow-mast-name")},
            // Guardrails S2: hold landing beside the bulkhead door (lower tween) and the bridge-roof stair landing.
            {FVector(800,150,560),FVector(800,715,500),80,TEXT("14-hold-landing-rail")},
            {FVector(-2650,-950,2480),FVector(-3185,-950,2250),75,TEXT("15-bridge-roof-rail")},
            // Details S3 (Blender Y reflected): SB bunker station on the hold 1 coaming, hold vents between the hatches,
            // bridge-roof kit seen from the forward stair landing.
            {FVector(-1330,-900,1260),FVector(-1760,-770,1110),80,TEXT("16-bunker-station")},
            {FVector(1200,880,1330),FVector(1200,0,1060),85,TEXT("17-hold-vents")},
            {FVector(-3400,-560,2600),FVector(-4650,150,2250),80,TEXT("18-bridge-roof-kit")},
            // SB hatch coaming of hold 2 (stays, cleats) and the SB accommodation ladder stowed under the lifeboat.
            {FVector(2000,-880,1250),FVector(2700,-760,1130),80,TEXT("19-coaming-stays")},
            {FVector(-4200,-900,1200),FVector(-5000,-790,1370),85,TEXT("20-accommodation-ladder")},
            // Service rooms: CO2 room door on the engine-room tween flat, paint store (bow tween) and deck store (bow lower).
            {FVector(-2680,100,890),FVector(-3090,-250,830),85,TEXT("21-co2-room")},
            {FVector(5160,90,890),FVector(4900,0,830),90,TEXT("22-paint-store")},
            {FVector(5160,90,615),FVector(4900,0,555),90,TEXT("23-deck-store")}};
        const int32 I=(Stage-70)/2;
        if((Stage%2)==0&&Now-Since>.6)
        {
            auto* Ship=ACalmyShipMotion::Find(GetWorld());
            if(!ReviewCamera.IsValid()){auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->GetCameraComponent()->bConstrainAspectRatio=false;ReviewCamera=Cam;}
            ACameraActor* Cam=ReviewCamera.Get();Cam->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            const FTransform Frame=Ship?Ship->GetActorTransform():FTransform::Identity;
            Cam->SetActorTransform(FTransform((SpaceShots[I].Target-SpaceShots[I].Eye).Rotation(),SpaceShots[I].Eye)*Frame);
            if(Ship)Cam->AttachToActor(Ship,FAttachmentTransformRules::KeepWorldTransform);
            Cam->GetCameraComponent()->SetFieldOfView(SpaceShots[I].Fov);PC->SetViewTarget(Cam);Since=Now;++Stage;
        }
        else if((Stage%2)==1&&Now-Since>1.2){Photo(SpaceShots[I].Name);Since=Now;Stage=(Stage==97)?26:Stage+1;}
    }
    else if(Stage==26&&Now-Since>.6)
    {
        PC->SetViewTarget(P);
        if(ReviewCamera.IsValid())ReviewCamera->Destroy();
        Since=Now;Stage=27;
    }
    // Breakdown arrow: a leak one deck below, ahead and to the right.
    else if(Stage==27&&Now-Since>.6)
    {
        ACalmyHUD::ReviewBreakdownLocation=P->GetActorLocation()+FRotator(0,PC->GetControlRotation().Yaw,0).RotateVector(FVector(700,800,-275));
        ACalmyHUD::bReviewBreakdown=true;Since=Now;Stage=28;
    }
    else if(Stage==28&&Now-Since>.8){if(!LayoutOk(TEXT("breakdown_arrow")))return;Photo(TEXT("13-breakdown-arrow"));Since=Now;Stage=29;}
    // The screenshot is captured on a later frame: keep the arrow on until it has been taken.
    else if(Stage==29&&Now-Since>.6){ACalmyHUD::bReviewBreakdown=false;Since=Now;Stage=30;}
    else if(Stage==30&&Now-Since>.6){ACalmyHUD::bReviewCrowded=true;Since=Now;Stage=4;}
    else if(Stage==4&&Now-Since>.6)
    {
        if(!LayoutOk(TEXT("crowded")))return;
        if(ACalmyHUD::LastVisibleTop<3){Finish(false,FString::Printf(TEXT("crowded_top_dropped top=%d/%d"),ACalmyHUD::LastVisibleTop,ACalmyHUD::LastTopItems));return;}
        Photo(TEXT("03-crowded"));Since=Now;Stage=40;
    }
    else if(Stage==40&&Now-Since>.6){SavedScale=UCalmyPreferences::Get()->HUDScale;UCalmyPreferences::Get()->HUDScale=1.5f;Since=Now;Stage=5;}
    else if(Stage==5&&Now-Since>.6)
    {
        if(!LayoutOk(TEXT("crowded_hud150")))return;
        Photo(TEXT("04-crowded-hud150"));Since=Now;Stage=6;
    }
    // Controls and tips now live in Settings: photograph that tab too.
    else if(Stage==6&&Now-Since>.6){PC->OpenSettings();Since=Now;Stage=60;}
    else if(Stage==60&&Now-Since>.6){if(auto Widget=PC->GetSettingsWidget();Widget.IsValid())Widget->SetPage(5);Since=Now;Stage=61;}
    else if(Stage==61&&Now-Since>.8){Photo(TEXT("08-settings-tips"));Since=Now;Stage=62;}
    else if(Stage==62&&Now-Since>.6){PC->CloseSettings();Since=Now;Stage=7;}
    else if(Stage==7&&Now-Since>.6)
    {
        Finish(true,FString::Printf(TEXT("shots=%d hud150_top=%d/%d"),Shots,ACalmyHUD::LastVisibleTop,ACalmyHUD::LastTopItems));
    }
}
