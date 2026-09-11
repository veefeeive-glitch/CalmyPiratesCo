#include "Testing/CalmyMeleeProbe.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCrewHelper.h"
#include "Gameplay/FlySwarmMob.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceRedirector.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"

bool UCalmyMeleeProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V; const auto* W=Cast<UWorld>(Outer);
    return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyMeleeProbe="),V);
#endif
}
void UCalmyMeleeProbe::Finish(bool OK,const FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("CALMY_MELEE_TEST %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);
    GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);
}
void UCalmyMeleeProbe::Photo(const FString& Name)
{
    if(Role!=TEXT("Visual"))return;
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("CalmyReviewDir="),Dir);
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/(Name+TEXT(".png")),false,false);
}
void UCalmyMeleeProbe::Tick(float Dt)
{
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyMeleeProbe="),Role);
    if(Stage==999||!GetWorld()->HasBegunPlay())return;
    const bool Host=Role==TEXT("Host"),Visual=Role==TEXT("Visual"),Late=Role==TEXT("LateJoin"),Server=Host||Visual;
    if(!Visual&&GetWorld()->GetNetMode()!=(Host?NM_ListenServer:NM_Client))return;
    auto* PC=Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* P=PC?Cast<ACalmyCharacter>(PC->GetPawn()):nullptr;if(!P)return;
    PC->ResumeFromMenu();Age+=Dt;
    if(Age>130){Finish(false,FString::Printf(TEXT("timeout stage=%d position=%s wheel=%d"),Stage,*P->GetActorLocation().ToString(),PC->IsFlyWheelOpen()));return;}
    ACalmyCrewHelper* M=nullptr;AFlySwarmMob* F=nullptr;
    for(TActorIterator<ACalmyCrewHelper> It(GetWorld());It;++It){M=*It;break;}
    for(TActorIterator<AFlySwarmMob> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("Habitat_Hold1"))){F=*It;break;}
    if(!M||!F)return;
    if(Server)F->bRoamShip=false; // Stationary melee fixture; travelling colony tested separately.
    auto Place=[](ACalmyCharacter* C,FVector Pos)
    {C->SetActorLocation(Pos,false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->DisableMovement();};
    if(Server)
    {
        if(!PlacedHelper){M->ReleaseLeader();Place(M,FVector(-400,0,310));PlacedHelper=true;}
        if(!Started){Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_MELEE_TEST %s START"),*Role);GLog->Flush();}
        if(!PlacedAttacker)
        {
            for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)
            {
                auto* C=It->Get();auto* Candidate=C?Cast<ACalmyCharacter>(C->GetPawn()):nullptr;
                if(Candidate&&(Visual||!C->IsLocalController())){Active=Candidate;Place(Candidate,FVector(-540,0,310));PlacedAttacker=true;break;}
            }
        }
        if(Active.IsValid())
        {
            SawCounter|=Active->MeleeHitsReceived>0&&M->Aggressor==Active.Get();
            ActiveRejections=Active->RejectedCommands;
            if(SawCounter&&!MovedToFlies)
            {
                CounterAge+=Dt;
                if(CounterAge>1.5f){Place(Active.Get(),F->GetActorLocation()+FVector(-110,0,-82));MovedToFlies=true;}
            }
        }
        if(M->MeleeHitsReceived<3&&M->IsDefending()){Finish(false,TEXT("retaliated_before_third_hit"));return;}
        if(M->MeleeHitsReceived>3){Finish(false,TEXT("duplicate_or_wheel_click_hit_misha"));return;}
        if(Host&&PlacedAttacker&&!Active.IsValid()&&F->SwatCount==1)
        {Finish(SawCounter&&ActiveRejections>0&&P->Health==100&&!M->IsDefending(),TEXT("three_hits_then_counter=1 bystander_safe=1 replay_rejected=1 swat_replicated=1 disconnect_clears_aggression=1"));return;}
    }
    if(Host)return;
    if(Late)
    {
        if(M->MeleeHitsReceived==3&&IsValid(M->Aggressor)&&M->Aggressor!=P)
        {Finish(P->Health==100&&M->Assistance==ECalmyHelperState::Defending,TEXT("late_join_aggressor_and_third_hit_snapshot=1 innocent_health=100"));}
        return;
    }
    auto Key=[&](FKey K)
    {PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,IE_Pressed,FPlatformTime::Cycles64()));PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,IE_Released,FPlatformTime::Cycles64()));};
    auto Advance=[&](int32 Next){Stage=Next;Since=Age;UE_LOG(LogTemp,Display,TEXT("CALMY_MELEE_TEST %s STAGE %d"),*Role,Stage);};
    if(Stage<4)PC->SetControlRotation((M->GetActorLocation()+FVector(0,0,30)-P->GetInteractionOrigin()).Rotation());
    else if(!PC->IsFlyWheelOpen())
    {
        // Aim at a real current instance, keeping the same player origin used by gameplay.
        FTransform Pose;F->FlyInstances->GetInstanceTransform(0,Pose,true);
        PC->SetControlRotation((Pose.GetLocation()-P->GetInteractionOrigin()).Rotation());
    }
    if(Stage==0&&Age>2&&P->FindMeleeTarget()==M){Key(EKeys::LeftMouseButton);Advance(1);}
    else if(Stage==1&&Age-Since>1.f)
    {
        if(M->MeleeHitsReceived!=1||P->Health!=100||M->IsDefending()){Finish(false,TEXT("first_hit_or_warning_failed"));return;}
        Photo(TEXT("01-first-punch"));Key(EKeys::LeftMouseButton);Advance(2);
    }
    else if(Stage==2&&Age-Since>1.f)
    {
        if(M->MeleeHitsReceived!=2||P->Health!=100||M->IsDefending()){Finish(false,TEXT("second_hit_or_warning_failed"));return;}
        if(!Started){Started=true;UE_LOG(LogTemp,Display,TEXT("CALMY_MELEE_TEST %s READY"),*Role);GLog->Flush();}
        if(!Visual&&GetWorld()->GetGameState()->PlayerArray.Num()<3)return;
        Key(EKeys::LeftMouseButton);Advance(3);
    }
    else if(Stage==3)
    {
        if(!ReplaySent&&P->PunchCount==3){P->ServerPunch(P->LastIssuedSequence);ReplaySent=true;}
        if(P->MeleeHitsReceived>0)
        {
            SawCounter=true;
            if(Visual&&!CounterPhotoTaken&&CounterAge>.15f){Photo(TEXT("02-misha-counter"));CounterPhotoTaken=true;}
        }
        if(P->MeleeHitsReceived>0&&FVector::Dist(P->GetActorLocation(),F->GetActorLocation())<160){Advance(4);}
    }
    else if(Stage==4&&Age-Since>1.f&&AFlySwarmMob::FindAimedSwarm(PC)==F){Key(EKeys::E);Advance(5);}
    else if(Stage==5&&Age-Since>.5f)
    {
        if(!PC->IsFlyWheelOpen()||!PC->IsMoveInputIgnored()||!PC->IsLookInputIgnored()||GetWorld()->IsPaused()){Finish(false,TEXT("wheel_input_lease_failed"));return;}
        int32 W,H;PC->GetViewportSize(W,H);PC->SetMouseLocation(W/2+FMath::Min(W,H)*.17f,H/2);Advance(6);
    }
    else if(Stage==6&&Age-Since>.6f)
    {
        // NullRHI has no OS cursor; the GPU probe exercises real mouse hover.
        if(!Visual)PC->WheelSelection=ACalmyPlayerController::WheelSector(FVector2D(130,0),200);
        if(PC->GetFlyWheelSelection()!=1){Finish(false,TEXT("wheel_hover_failed"));return;}
        Photo(TEXT("03-interaction-wheel"));Advance(7);
    }
    else if(Stage==7&&Age-Since>.4f)
    {
        if(Visual)Key(EKeys::LeftMouseButton);
        else {PC->WheelSelection=1;PC->ConfirmFlyWheel();}
        Advance(8);
    }
    else if(Stage==8&&Age-Since>.6f)
    {
        if(PC->IsFlyWheelOpen()||PC->IsMoveInputIgnored()||PC->IsLookInputIgnored()||P->PunchCount!=3||F->SpeechSerial<1)
        {Finish(false,TEXT("wheel_confirm_click_leaked_or_no_reply"));return;}
        Key(EKeys::E);Advance(9);
    }
    else if(Stage==9&&Age-Since>.5f)
    {
        if(!PC->IsFlyWheelOpen()){Finish(false,TEXT("wheel_reopen_failed"));return;}
        Key(EKeys::Escape);Advance(10);
    }
    else if(Stage==10&&Age-Since>.5f)
    {
        if(PC->IsFlyWheelOpen()||PC->IsPauseMenuOpen()||PC->IsMoveInputIgnored()||PC->IsLookInputIgnored())
        {Finish(false,TEXT("wheel_cancel_leaked_to_pause"));return;}
        if(P->FindMeleeTarget()!=F)return;
        Key(EKeys::LeftMouseButton);Advance(11);
    }
    else if(Stage==11)
    {
        SawScatter|=F->bScattering&&F->SwatCount==1;
        if(Age-Since>.25f&&Stage<12&&SawScatter){Photo(TEXT("04-swat-scatter"));Advance(12);}
        if(Age-Since>3.f){Finish(false,TEXT("swat_missed_or_not_replicated"));return;}
    }
    else if(Stage==12&&Age-Since>1.f)
    {
        Finish(SawCounter&&SawScatter&&M->MeleeHitsReceived==3&&P->PunchCount==4&&P->Health>0&&P->Health<100,
            FString::Printf(TEXT("hits_misha=%d punches=%d response_damage=%.0f wheel_hover_confirm_cancel=1 no_click_leak=1 swat=%d"),M->MeleeHitsReceived,P->PunchCount,100-P->Health,F->SwatCount));
    }
}
