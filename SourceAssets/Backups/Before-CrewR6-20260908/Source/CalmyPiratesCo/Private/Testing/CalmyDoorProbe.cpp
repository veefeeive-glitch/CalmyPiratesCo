#include "Testing/CalmyDoorProbe.h"
#include "Gameplay/CalmyDoor.h"
#include "Gameplay/CalmyCharacter.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
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
    Age+=DT;StageAge+=DT;if(Age>140){Finish(false,TEXT("timeout"));return;}
    TArray<ACalmyDoor*> Doors;ACalmyDoor* RpcDoor=nullptr;for(TActorIterator<ACalmyDoor> It(GetWorld());It;++It){if(It->DoorId==1000)RpcDoor=*It;else Doors.Add(*It);}
    if(Age<4)return;
    if(Audit){int32 Clear=0;Doors.Sort([](const ACalmyDoor& A,const ACalmyDoor& B){return A.DoorId<B.DoorId;});for(auto* D:Doors){const bool OK=D->FitSwingToWorld();Clear+=OK?1:0;const FVector V=D->GetActorLocation();UE_LOG(LogTemp,Display,TEXT("DOOR_FIT id=%d clear=%d x=%.4f y=%.4f z=%.4f yaw=%.3f sign=%.0f angle=%.0f"),D->DoorId,OK,V.X,V.Y,V.Z,D->GetActorRotation().Yaw,D->SwingSign,D->OpenAngle);}Finish(Clear==61,FString::Printf(TEXT("runtime_swing_clear=%d/61"),Clear));return;}
    auto Photo=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Verification/Doors-R1")/(FString(Name)+TEXT(".png"))),true,false);};
    if(Stage==0){if(Doors.Num()!=61){Finish(false,FString::Printf(TEXT("count=%d"),Doors.Num()));return;}
        UE_LOG(LogTemp,Display,TEXT("CALMY_DOOR %s START count=61"),*Role);UE_LOG(LogTemp,Display,TEXT("CALMY_DOOR %s READY"),*Role);GLog->Flush();Stage=1;StageAge=0;}
    if(Visual&&!Fixture){auto* Cam=GetWorld()->SpawnActor<ACameraActor>();for(auto* D:Doors)if(D->DoorId==13){const FVector Center=D->GetActorLocation()+D->GetActorQuat().RotateVector(FVector(-D->WidthCm/2,0,125));const FVector Eye=Center+D->GetActorQuat().RotateVector(FVector(20,-240*D->SwingSign,20));Cam->SetActorLocation(Eye);Cam->SetActorRotation((Center-Eye).Rotation());}Cam->GetCameraComponent()->SetFieldOfView(78);Cam->GetCameraComponent()->bConstrainAspectRatio=false;auto* PC=GetWorld()->GetFirstPlayerController();PC->SetViewTarget(Cam);if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;Fixture=true;}
    if((Host||Solo)&&Stage==1&&StageAge>8){for(auto* D:Doors){D->TargetAngle=0;D->State=ECalmyDoorState::Closing;++D->Revision;}Stage=2;StageAge=0;}
    if((Host||Solo)&&Stage==2&&StageAge>4){int32 Closed=0;for(auto* D:Doors){if(D->State==ECalmyDoorState::Closed)++Closed;else UE_LOG(LogTemp,Warning,TEXT("DOOR_BLOCKED id=%d angle=%.1f pos=%s"),D->DoorId,D->Angle,*D->GetActorLocation().ToString());}
        if(Closed!=61){Finish(false,FString::Printf(TEXT("closed=%d/61"),Closed));return;}if(Visual)Photo(TEXT("door-closed"));for(auto* D:Doors){D->TargetAngle=D->OpenAngle;D->State=ECalmyDoorState::Opening;++D->Revision;}Stage=3;StageAge=0;}
    if((Host||Solo)&&Stage==3&&StageAge>4){for(auto* D:Doors)if(!D->IsOpen()){Finish(false,FString::Printf(TEXT("not_open=%d"),D->DoorId));return;}if(Visual)Photo(TEXT("door-open"));Stage=4;StageAge=0;}
    // Explicit isolated network interaction fixture; no saved-map changes or traversal claim.
    if(Host&&Stage==4&&!Fixture){FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        RpcDoor=GetWorld()->SpawnActor<ACalmyDoor>(FVector(0,0,10000),FRotator::ZeroRotator,S);RpcDoor->DoorId=1000;RpcDoor->ForceNetUpdate();Fixture=true;}
    if(Host&&Fixture)for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It){auto* PC=Cast<APlayerController>(It->GetController());
        if(PC&&!PC->IsLocalController()&&!It->ActorHasTag(TEXT("DoorFixture"))){It->Tags.Add(TEXT("DoorFixture"));It->SetActorLocation(FVector(56,-150,10090),false,nullptr,ETeleportType::TeleportPhysics);It->GetCharacterMovement()->StopMovementImmediately();It->GetCharacterMovement()->SetMovementMode(MOVE_None);It->ForceNetUpdate();}}
    if(!Host&&!Solo&&Stage==1&&Age>22){
        for(auto* D:Doors)if(!D->IsOpen()||D->Revision<2){Finish(false,TEXT("replication_angle_revision"));return;}
        if(!RpcDoor)return;
        if(Role==TEXT("LateJoin")){if(RpcDoor->IsOpen()&&RpcDoor->Revision==2)Finish(true,TEXT("61_snapshot_plus_client_operated_door_open revision=2"));return;}
        ACalmyCharacter* P=nullptr;for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It)if(It->IsLocallyControlled()&&Cast<APlayerController>(It->GetController()))P=*It;
        if(!P||P->GetActorLocation().Z<9000)return;
        auto* PC=Cast<APlayerController>(P->GetController());PC->SetControlRotation((RpcDoor->Collision->GetComponentLocation()-P->GetInteractionOrigin()).Rotation());RpcAge+=DT;
        if(RpcStage==0&&RpcAge>1){const uint32 Seq=++P->LastIssuedSequence;P->ServerDoor(RpcDoor,RpcDoor->Revision,Seq);P->ServerDoor(RpcDoor,RpcDoor->Revision,Seq);RpcStage=1;RpcAge=0;}
        else if(RpcStage==1&&RpcAge>4){if(RpcDoor->State!=ECalmyDoorState::Closed||RpcDoor->Revision!=1||P->RejectedCommands<1){Finish(false,TEXT("owning_client_close_or_duplicate"));return;}P->ServerDoor(RpcDoor,0,++P->LastIssuedSequence);RpcStage=2;RpcAge=0;}
        else if(RpcStage==2&&RpcAge>1){if(RpcDoor->Revision!=1||P->RejectedCommands<2){Finish(false,TEXT("stale_revision"));return;}P->RequestGrab();RpcStage=3;RpcAge=0;}
        else if(RpcStage==3&&RpcAge>4){Finish(RpcDoor->IsOpen()&&RpcDoor->Revision==2,TEXT("61_snapshot owning_client_E close_open duplicate_and_stale_denied"));}
    }
    if((Host||Solo)&&Stage==4&&StageAge>2&&(!Host||Age>100))Finish(Solo||(RpcDoor&&RpcDoor->IsOpen()&&RpcDoor->Revision==2),TEXT("61_closed_and_reopened collision_enabled client_disconnect_state no_player_traversal"));
}
