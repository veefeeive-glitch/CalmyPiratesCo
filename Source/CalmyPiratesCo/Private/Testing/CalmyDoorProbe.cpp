#include "Testing/CalmyDoorProbe.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
bool UCalmyDoorProbe::ShouldCreateSubsystem(UObject* Outer)const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString S;const auto* W=Cast<UWorld>(Outer);return W&&W->IsGameWorld()&&FParse::Value(FCommandLine::Get(),TEXT("CalmyDoorProbe="),S);
#endif
}
void UCalmyDoorProbe::Finish(bool OK,const FString& Detail){UE_LOG(LogTemp,Display,TEXT("CALMY_DOOR %s %s %s"),*Role,OK?TEXT("PASS"):TEXT("FAIL"),*Detail);GLog->Flush();Stage=999;FPlatformMisc::RequestExitWithStatus(false,OK?0:1);}
void UCalmyDoorProbe::Tick(float DT)
{
    if(Stage==999||!GetWorld()->HasBegunPlay())return;if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("CalmyDoorProbe="),Role);
    const bool Visual=Role==TEXT("Visual"),Audit=Role==TEXT("Audit"),Host=Role==TEXT("Host"),Solo=Role==TEXT("Solo")||Audit||Visual;if((Host&&GetWorld()->GetNetMode()!=NM_ListenServer)||(!Host&&!Solo&&GetWorld()->GetNetMode()!=NM_Client))return;
    Age+=DT;StageAge+=DT;if(Age>150){Finish(false,TEXT("timeout"));return;}
    TArray<ACalmyDoor*> Doors;ACalmyDoor* RpcDoor=nullptr;ACalmyDoor* DragDoor=nullptr;
    for(TActorIterator<ACalmyDoor> It(GetWorld());It;++It){if(It->DoorId==1000)RpcDoor=*It;else if(It->DoorId==1001)DragDoor=*It;else Doors.Add(*It);}
    if(Age<4)return;
    if(Audit){int32 Clear=0,MapClear=0;Doors.Sort([](const ACalmyDoor& A,const ACalmyDoor& B){return A.DoorId<B.DoorId;});for(auto* D:Doors){
        // The saved map pose is what players get at runtime: check its whole swing before trying alternative fits.
        float BlockedAt=-1;for(float A=0;A<=D->OpenAngle;A+=1)if(D->PoseBlocked(A)){BlockedAt=A;break;}
        if(BlockedAt<0)++MapClear;else UE_LOG(LogTemp,Display,TEXT("DOOR_SWING id=%d map_pose_blocked_at=%.0f open=%.0f sign=%.0f by=%s"),D->DoorId,BlockedAt,D->OpenAngle,D->SwingSign,*D->PoseBlocker(BlockedAt));
        const bool OK=D->FitSwingToWorld();Clear+=OK?1:0;const FVector V=D->GetActorLocation();UE_LOG(LogTemp,Display,TEXT("DOOR_FIT id=%d clear=%d x=%.4f y=%.4f z=%.4f yaw=%.3f sign=%.0f angle=%.0f"),D->DoorId,OK,V.X,V.Y,V.Z,D->GetActorRotation().Yaw,D->SwingSign,D->OpenAngle);}Finish(Clear==61&&MapClear==61,FString::Printf(TEXT("runtime_swing_clear=%d/61 map_pose_clear=%d/61"),Clear,MapClear));return;}
    auto Photo=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification/Doors-R1")/(FString(Name)+TEXT(".png"))),true,false);};
    if(Stage==0){if(Doors.Num()!=61){Finish(false,FString::Printf(TEXT("count=%d"),Doors.Num()));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_DOOR %s START count=61"),*Role);UE_LOG(LogTemp,Display,TEXT("CALMY_DOOR %s READY"),*Role);GLog->Flush();Stage=1;StageAge=0;}
    if(Visual&&!Fixture){auto* Cam=GetWorld()->SpawnActor<ACameraActor>();for(auto* D:Doors)if(D->DoorId==13){const FVector Center=D->GetActorLocation()+D->GetActorQuat().RotateVector(FVector(-D->WidthCm/2,0,125));const FVector Eye=Center+D->GetActorQuat().RotateVector(FVector(20,-240*D->SwingSign,20));Cam->SetActorLocation(Eye);Cam->SetActorRotation((Center-Eye).Rotation());}Cam->GetCameraComponent()->SetFieldOfView(78);Cam->GetCameraComponent()->bConstrainAspectRatio=false;auto* PC=GetWorld()->GetFirstPlayerController();PC->SetViewTarget(Cam);if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;Fixture=true;}
    // Heavy doors swing on their own after a tap: a full 180 degree swing takes about three seconds.
    if((Host||Solo)&&Stage==1&&StageAge>8){for(auto* D:Doors)D->SetAutoTarget(false);Stage=2;StageAge=0;}
    if((Host||Solo)&&Stage==2&&StageAge>7){int32 Closed=0;for(auto* D:Doors){if(D->IsClosed())++Closed;else UE_LOG(LogTemp,Warning,TEXT("DOOR_BLOCKED id=%d angle=%.1f pos=%s"),D->DoorId,D->Angle,*D->GetActorLocation().ToString());}
        if(Closed!=61){Finish(false,FString::Printf(TEXT("closed=%d/61"),Closed));return;}if(Visual)Photo(TEXT("door-closed"));for(auto* D:Doors)D->SetAutoTarget(true);Stage=3;StageAge=0;}
    if((Host||Solo)&&Stage==3&&StageAge>7){for(auto* D:Doors)if(!D->IsOpen()){Finish(false,FString::Printf(TEXT("not_open=%d angle=%.1f"),D->DoorId,D->Angle));return;}if(Visual)Photo(TEXT("door-open"));Stage=4;StageAge=0;}
    // Explicit isolated network interaction fixtures; no saved-map changes or traversal claim.
    if(Host&&Stage==4&&!Fixture){FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        // Toggle fixture starts open: owning-client test closes then reopens it.
        RpcDoor=GetWorld()->SpawnActor<ACalmyDoor>(FVector(0,0,10000),FRotator::ZeroRotator,S);
        RpcDoor->DoorId=1000;RpcDoor->bStartOpen=true;RpcDoor->SnapTo(RpcDoor->OpenAngle);
        // Drag fixture starts half open beside the client so a held pull can swing it either way.
        DragDoor=GetWorld()->SpawnActor<ACalmyDoor>(FVector(0,-260,10000),FRotator::ZeroRotator,S);
        DragDoor->DoorId=1001;DragDoor->SnapTo(90);Fixture=true;}
    if(Host&&Fixture)for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It){auto* PC=Cast<APlayerController>(It->GetController());
        if(PC&&!PC->IsLocalController()&&!It->ActorHasTag(TEXT("DoorFixture"))){It->Tags.Add(TEXT("DoorFixture"));It->SetActorLocation(FVector(56,-150,10090),false,nullptr,ETeleportType::TeleportPhysics);It->GetCharacterMovement()->StopMovementImmediately();It->GetCharacterMovement()->SetMovementMode(MOVE_None);It->ForceNetUpdate();}}
    if(!Host&&!Solo&&Stage==1&&Age>30){
        for(auto* D:Doors)if(!D->IsOpen()||D->Revision<2){Finish(false,FString::Printf(TEXT("replication_angle_revision id=%d angle=%.1f rev=%u"),D->DoorId,D->Angle,D->Revision));return;}
        if(!RpcDoor)return;
        if(Role==TEXT("LateJoin")){if(RpcDoor->IsOpen()&&RpcDoor->Revision==2)Finish(true,TEXT("61_snapshot_plus_client_operated_door_open revision=2"));return;}
        ACalmyCharacter* P=nullptr;for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)if(It->IsLocallyControlled()&&Cast<APlayerController>(It->GetController()))P=*It;
        if(!P||P->GetActorLocation().Z<9000)return;
        auto* PC=Cast<APlayerController>(P->GetController());RpcAge+=DT;
        const ACalmyDoor* Aimed=RpcStage<4?RpcDoor:DragDoor;
        if(Aimed)PC->SetControlRotation((Aimed->Collision->GetComponentLocation()-P->GetInteractionOrigin()).Rotation());
        if(RpcStage==0&&RpcAge>1){const uint32 Seq=++P->LastIssuedSequence;P->ServerDoor(RpcDoor,RpcDoor->Revision,Seq);P->ServerDoor(RpcDoor,RpcDoor->Revision,Seq);RpcStage=1;RpcAge=0;}
        else if(RpcStage==1&&RpcAge>6){if(!RpcDoor->IsClosed()||RpcDoor->Revision!=1||P->RejectedCommands<1){Finish(false,FString::Printf(TEXT("owning_client_close_or_duplicate state=%d rev=%u angle=%.1f"),int32(RpcDoor->State),RpcDoor->Revision,RpcDoor->Angle));return;}P->ServerDoor(RpcDoor,0,++P->LastIssuedSequence);RpcStage=2;RpcAge=0;}
        else if(RpcStage==2&&RpcAge>1){if(RpcDoor->Revision!=1||P->RejectedCommands<2){Finish(false,TEXT("stale_revision"));return;}const uint32 Seq=++P->LastIssuedSequence;P->ServerDoor(RpcDoor,RpcDoor->Revision,Seq);RpcStage=3;RpcAge=0;}
        else if(RpcStage==3&&RpcAge>6){if(!RpcDoor->IsOpen()||RpcDoor->Revision!=2){Finish(false,TEXT("owning_client_reopen"));return;}RpcStage=4;RpcAge=0;}
        // Held drag: a full pull for 1.5 s must move the heavy leaf, but no faster than the drag speed limit allows.
        else if(RpcStage==4&&RpcAge>1&&DragDoor){P->ServerDoorGrab(DragDoor,true,++P->LastDoorGrabIssued);DragStartAngle=DragDoor->Angle;RpcStage=5;RpcAge=0;}
        else if(RpcStage==5)
        {
            if(DragDoor)P->ServerDoorDrag(DragDoor,127);
            if(RpcAge>1.5f)
            {
                const float Moved=FMath::Abs(DragDoor->Angle-DragStartAngle);
                if(DragDoor->Holder!=P||Moved<20.f||Moved>FCalmyDoorMotion::MaxDragSpeed*1.5f+10.f)
                {Finish(false,FString::Printf(TEXT("held_drag holder=%d moved=%.1f"),DragDoor->Holder==P,Moved));return;}
                DragMoved=Moved;P->ServerDoorGrab(DragDoor,false,++P->LastDoorGrabIssued);RpcStage=6;RpcAge=0;
            }
        }
        else if(RpcStage==6&&RpcAge>2)
        {
            Finish(DragDoor&&DragDoor->Holder==nullptr,FString::Printf(TEXT("61_snapshot owning_client_E close_open duplicate_and_stale_denied held_drag_moved=%.0f released=1"),DragMoved));
        }
    }
    if((Host||Solo)&&Stage==4&&StageAge>2&&(!Host||Age>100))Finish(Solo||(RpcDoor&&RpcDoor->IsOpen()&&RpcDoor->Revision==2),TEXT("61_closed_and_reopened collision_enabled client_disconnect_state no_player_traversal"));
}
