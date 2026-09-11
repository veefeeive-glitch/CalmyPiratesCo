#include "Testing/CalmyFlyMindProbe.h"
#include "Gameplay/FlySwarmMob.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "InputKeyEventArgs.h"

bool UCalmyFlyMindProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    FString V; const auto* W = Cast<UWorld>(Outer);
    return W && W->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmyFlyMindProbe="), V);
#endif
}
void UCalmyFlyMindProbe::Finish(bool OK, const FString& Detail)
{
    UE_LOG(LogTemp, Display, TEXT("CALMY_FLY_MIND_TEST %s %s %s"), *Role, OK ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    GLog->Flush(); Age = -10000; FPlatformMisc::RequestExitWithStatus(false, OK ? 0 : 1);
}
void UCalmyFlyMindProbe::Tick(float Dt)
{
    if (Role.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("CalmyFlyMindProbe="), Role);
    if (Age < 0 || !GetWorld()->HasBegunPlay()) return;
    auto* PC = Cast<ACalmyPlayerController>(GetWorld()->GetFirstPlayerController());
    if (!PC || !PC->GetPawn()) return;
    PC->ResumeFromMenu();
    const bool Host = Role == TEXT("Host"), Visual = Role == TEXT("Visual"), Restore = Role == TEXT("Restore");
    if (!Visual && !Restore && GetWorld()->GetNetMode() != (Host ? NM_ListenServer : NM_Client)) return;
    Age += Dt;
    if (Age > 140.f) { Finish(false, TEXT("timeout")); return; }
    AFlySwarmMob* Swarm = nullptr;
    for (TActorIterator<AFlySwarmMob> It(GetWorld()); It; ++It) if (It->ActorHasTag(TEXT("Habitat_Hold1"))) { Swarm = *It; break; }
    if (!Swarm) return;
    // This fixture isolates conversation/save semantics; ship travel has its own probe.
    if (Host || Visual) Swarm->bRoamShip = false;
    if (!Swarm->bEnableCognition) { Finish(false, TEXT("saved_resident_missing_cognition")); return; }
    if (Restore)
    {
        bool Found = false;
        for (const auto& P : Swarm->GetCognition().People) Found |= P.bPersistent && P.Threats == 1 && P.Conversations == 3;
        Finish(Found, TEXT("new_process_persistent_player_threat_and_conversations_restored")); return;
    }
    if (Host || Visual)
    {
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            auto* C = Cast<ACalmyPlayerController>(It->Get());
            auto* P = C ? Cast<ACalmyCharacter>(C->GetPawn()) : nullptr;
            if (!P || PlacedPlayers.Contains(C->GetUniqueID())) continue;
            bFarRejected |= !Swarm->ReceiveUtterance(C, EFlyUtterance::Greet);
            const FVector Position = Swarm->GetActorLocation() + FVector(-230, (PlacedPlayers.Num()-1)*85, -82);
            P->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
            P->GetCharacterMovement()->StopMovementImmediately();
            P->GetCharacterMovement()->DisableMovement();
            C->SetControlRotation((Swarm->GetActorLocation() - P->GetPawnViewLocation()).Rotation());
            bInvalidRejected |= !Swarm->ReceiveUtterance(C, static_cast<EFlyUtterance>(255));
            PlacedPlayers.Add(C->GetUniqueID());
        }
        if (!bStarted)
        { bStarted = true; UE_LOG(LogTemp, Display, TEXT("CALMY_FLY_MIND_TEST %s START"), *Role); GLog->Flush(); }
        if (PlacedPlayers.Num() >= 3) PeersAge += Dt;
    }
    if (!Swarm->CanConverse(PC)) return;
    if (!Host && !Visual && !bStarted)
    { bStarted = true; UE_LOG(LogTemp, Display, TEXT("CALMY_FLY_MIND_TEST %s READY"), *Role); GLog->Flush(); }
    StageAge += Dt;
    auto Key = [&](FKey K)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, K, IE_Pressed, FPlatformTime::Cycles64()));
        PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, K, IE_Released, FPlatformTime::Cycles64()));
    };
    // Cognitive regression uses semantic commands. MeleeProbe covers the real E wheel.
    if (Stage == 0 && StageAge > 1.f) { PC->RequestFlyTalk(EFlyUtterance::Greet); Stage = 1; StageAge = 0; }
    else if (Stage == 1 && StageAge > 4.6f)
    {
        PC->RequestFlyTalk(EFlyUtterance::Threaten);
        // Bypass the local cooldown to test duplicate/invalid intent on server.
        PC->ServerTalkToSwarm(Swarm, EFlyUtterance::Threaten);
        PC->ServerTalkToSwarm(Swarm, static_cast<EFlyUtterance>(255));
        Stage = 2; StageAge = 0;
    }
    else if (Stage == 2 && StageAge > 4.6f) { PC->RequestFlyTalk(EFlyUtterance::AskMemory); Stage = 3; StageAge = 0; }
    if (Stage == 3) bSawRecall |= Swarm->Speech.Contains(TEXT("Threats remembered: 1"));
    if (Visual && Stage == 3 && StageAge > 1.f && StageAge < 1.f+Dt*2)
    {
        FString Dir; FParse::Value(FCommandLine::Get(), TEXT("CalmyReviewDir="), Dir);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Verification")/Dir/TEXT("01-memory-dialogue.png"), false, false);
    }
    if (Stage < 3 || StageAge < 3.f || (Host && PeersAge < 18.f)) return;
    if (Host || Visual)
    {
        int32 Participants = 0;
        for (const auto& P : Swarm->GetCognition().People) if (P.Conversations == 3 && P.Threats == 1) ++Participants;
        if (Participants != (Visual ? 1 : 3) || !bFarRejected || !bInvalidRejected || !bSawRecall)
        { Finish(false, FString::Printf(TEXT("participants=%d far_reject=%d invalid_reject=%d recall=%d"), Participants, bFarRejected, bInvalidRejected, bSawRecall)); return; }
        const bool Saved = !Visual && Swarm->SaveMemory();
        Finish(Visual || Saved, FString::Printf(TEXT("participants=%d learned_threat_once=1 memory_recall=1 invalid_and_far_rejected=1 disk_saved=%d brains=%d serial=%d"), Participants, Saved, Swarm->GetSimulatedFlyCount(), Swarm->SpeechSerial));
    }
    else Finish(bSawRecall && Swarm->GetCognition().People.IsEmpty() && Swarm->GetSimulatedFlyCount() == 0,
        FString::Printf(TEXT("replicated_recall=%d authority_only_memory=1 brains=%d serial=%d"), bSawRecall, Swarm->GetSimulatedFlyCount(), Swarm->SpeechSerial));
}
