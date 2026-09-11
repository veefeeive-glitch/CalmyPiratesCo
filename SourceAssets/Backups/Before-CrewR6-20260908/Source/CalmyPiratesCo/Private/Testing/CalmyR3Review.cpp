#include "Testing/CalmyR3Review.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Gameplay/CalmyCargo.h"
#include "Gameplay/CalmyOcean.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
bool UCalmyR3Review::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    const auto* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld()&&FParse::Param(FCommandLine::Get(),TEXT("CalmyR3Review"));
#endif
}
void UCalmyR3Review::Photo(const FString& Name)
{
    FString Label=TEXT("CrewCargoR3-Review");FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Label);
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification")/FPaths::GetCleanFilename(Label));
    IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(Name+TEXT(".png")),true,false);
    UE_LOG(LogTemp,Display,TEXT("R3_PHOTO %s"),*Name);
}
void UCalmyR3Review::Tick(float DT)
{
    if(Stage==999 || !GetWorld()->HasBegunPlay())return;
    Age+=DT;auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;
    const bool G1HUD=FParse::Param(FCommandLine::Get(),TEXT("CalmyG1HUD"));
    const bool LP4=FParse::Param(FCommandLine::Get(),TEXT("CalmyLP4Views"));
    auto* Ocean=ACalmyOcean::Find(GetWorld());if(!P||!Ocean||Age<3)return;
    ACalmyCrewHelper* NPC=nullptr;ACalmyCargo* Crates[4]={};
    for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It)NPC=*It;
    for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It)Crates[static_cast<int32>(It->Kind)]=*It;
    auto Fail=[&](const TCHAR* Why){UE_LOG(LogTemp,Error,TEXT("R3_REVIEW FAIL %s"),Why);Stage=999;FPlatformMisc::RequestExitWithStatus(false,1);};
    if(!NPC || !Crates[0]||!Crates[1]||!Crates[2]||!Crates[3]) {Fail(TEXT("missing_scene_cast"));return;}
    auto Next=[&](){++Stage;StageTime=Age;};
    auto ShotReady=[&](const FString& Name){if(PhotoAt<StageTime){Photo(Name);PhotoAt=Age;return false;}return Age-PhotoAt>.5f;};
    auto Aim=[&](const FVector& At,const FVector& From){Camera->SetActorLocation(From);Camera->SetActorRotation((At-From).Rotation());PC->SetViewTarget(Camera);};
    auto AimCargo=[&](){for(int32 I=0;I<2;++I)PC->SetControlRotation((Crates[0]->GetActorLocation()-P->GetInteractionOrigin()).Rotation());};
    if(Stage==0)
    {
        Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(62);
        Camera->GetCameraComponent()->bConstrainAspectRatio=false;
        Camera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount=true;
        Camera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount=0;
        PC->GetHUD()->bShowHUD=false;
        Aim(NPC->GetActorLocation()+FVector(0,0,20),NPC->GetActorLocation()+FVector(-240,-200,70));Next();
    }
    else if(Stage==1 && Age-StageTime>3 && ShotReady(TEXT("01-misha-waiting")))
    {
        if(LP4)
        {
            P->SetActorLocation(FVector(-800,0,90));P->SetActorRotation(FRotator::ZeroRotator);
            Aim(P->GetActorLocation()+FVector(0,0,10),P->GetActorLocation()+FVector(260,-210,85));Stage=21;StageTime=Age;
        }
        else {Aim(FVector(560,-550,40),FVector(-80,-1020,350));Next();}
    }
    else if(Stage==21 && Age-StageTime>3 && ShotReady(TEXT("01b-player-front")))
    {Aim(P->GetActorLocation()+FVector(0,0,10),P->GetActorLocation()+FVector(-260,210,85));Stage=22;StageTime=Age;}
    else if(Stage==22 && Age-StageTime>3 && ShotReady(TEXT("01c-player-back")))
    {Aim(FVector(560,-550,40),FVector(-80,-1020,350));Stage=2;StageTime=Age;}
    else if(Stage==2 && Age-StageTime>3 && ShotReady(TEXT("02-four-cargo-silhouettes")))
    {
        P->SetActorLocation(G1HUD?Crates[0]->GetActorLocation()+FVector(-360,0,60):FVector(-600,-300,90));
        PC->SetViewTarget(P);PC->SetControlRotation(FRotator(-8,0,0));PC->GetHUD()->bShowHUD=G1HUD;Next();
    }
    else if(Stage==3)
    {
        if(G1HUD) AimCargo();
        if(Age-StageTime>3 && ShotReady(G1HUD?TEXT("03-aim-too-far"):TEXT("03-idle-bodycam-hands")))
        {
            if(G1HUD && P->GetGrabFeedback(Crates[0])!=ECalmyGrabFeedback::TooFar) {Fail(TEXT("far_hint_missing"));return;}
            P->SetActorLocation(Crates[0]->GetActorLocation()+FVector(-150,0,60));P->SetActorRotation(FRotator::ZeroRotator);
            if(G1HUD){Stage=31;StageTime=Age;}else{Crates[0]->TryGrab(P);Next();}
        }
    }
    else if(Stage==31)
    {
        AimCargo();
        if(Age-StageTime>3 && ShotReady(TEXT("04-aim-ready")))
        {
            if(P->GetGrabFeedback(Crates[0])!=ECalmyGrabFeedback::Ready){Fail(TEXT("ready_hint_missing"));return;}
            P->RequestGrab();PC->SetControlRotation(FRotator(-8,0,0));Stage=4;StageTime=Age;
        }
    }
    else if(Stage==4)
    {
        if(!P->HeldCargo){Fail(TEXT("review_grab_failed"));return;}
        if(Age-StageTime>1)MaxHandError=FMath::Max(MaxHandError,P->GetCrewPresentation()->GetHandContactError());
        if(Age-StageTime>3 && ShotReady(G1HUD?TEXT("05-held-bodycam"):TEXT("04-grip-contact"))){P->ReleaseCargo();PC->GetHUD()->bShowHUD=false;P->SetActorLocation(FVector(3100,0,Ocean->HeightAt(FVector(3100,0,0))-20));P->GetCharacterMovement()->SetMovementMode(MOVE_Custom,1);PC->SetControlRotation(FRotator(-35,0,0));Next();}
    }
    else if(Stage==5)
    {
        P->MoveIntent(FVector2D(.6,0));
        if(Age-StageTime>3 && ShotReady(TEXT("05-swimming-ripples"))){Aim(P->GetActorLocation(),P->GetActorLocation()+FVector(-320,-380,180));Next();}
    }
    else if(Stage==6)
    {
        P->MoveIntent(FVector2D(.3,0));Aim(P->GetActorLocation(),P->GetActorLocation()+FVector(-320,-380,180));
        if(Age-StageTime>3 && ShotReady(TEXT("06-water-wake")))
        {
            if(Ocean->ActiveDisturbances()<1 || MaxHandError>2) {Fail(TEXT("missing_wake_or_unreachable_hands"));return;}
            UE_LOG(LogTemp,Display,TEXT("R3_REVIEW READY hand_error_cm=%.3f ripples=%d"),MaxHandError,Ocean->ActiveDisturbances());Next();
        }
    }
    else if(Stage==7 && Age-StageTime>2){UE_LOG(LogTemp,Display,TEXT("R3_REVIEW PASS views=%d interaction_hud=%d"),(G1HUD?7:6)+(LP4?2:0),G1HUD);Stage=999;FPlatformMisc::RequestExitWithStatus(false,0);}
}
