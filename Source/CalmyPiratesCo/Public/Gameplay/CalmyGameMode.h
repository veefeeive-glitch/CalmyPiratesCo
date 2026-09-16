#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/FlyCognition.h"
#include "CalmyGameMode.generated.h"

UCLASS()
class CALMYPIRATESCO_API ACalmyGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ACalmyGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void Logout(AController* Exiting) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
};

UCLASS()
class CALMYPIRATESCO_API ACalmyPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ACalmyPlayerController();
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void ClientRestart_Implementation(APawn* NewPawn) override;
    virtual void ClientSetViewTarget_Implementation(AActor* Target,FViewTargetTransitionParams Params) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
    void OpenSettings();
    void CloseSettings();
    bool IsSettingsOpen() const {return SettingsWidget.IsValid();}
    TSharedPtr<class SCalmySettingsWidget> GetSettingsWidget() const {return SettingsWidget;}
    void OpenJoinDialog();
    void CloseJoinDialog();
    bool IsJoinDialogOpen() const {return JoinWidget.IsValid();}
    bool HostLanSession(FString* OutError=nullptr);
    bool JoinLanSession(const FString& Address, FString* OutError=nullptr);
    void CopyLanEndpointToClipboard();
    void ApplyAudioPreferences();
    void TogglePauseMenu();
    void SetPauseMenuOpen(bool bOpen);
    bool IsPauseMenuOpen() const { return bPauseMenuOpen; }
    bool IsSoloPause() const { return GetNetMode() == NM_Standalone; }
    void ResumeFromMenu();
    /** Local listen host / solo only; clients see the row as HOST and nothing happens. */
    void RequestReplayTutorial();
    void MenuCamera();
    void MenuMotion();
    void MenuLookDown();
    void MenuLookUp();
    void MenuClick();
    void MenuUp();
    void MenuDown();
    void MenuConfirm();
    bool TryOpenFlyWheel();
    void CloseFlyWheel();
    void WheelInteractKey();
    void ConfirmFlyWheel();
    bool IsFlyWheelOpen() const { return WheelSwarm.IsValid(); }
    bool IsGameplayInputSuppressed() const;
    int32 GetFlyWheelSelection() const { return WheelSelection; }
    float GetFlyTalkCooldown() const;
    static int32 WheelSector(FVector2D Offset, float Radius);
    void RequestFlyTalk(EFlyUtterance Intent);
    // Owning client -> server, only its possessed living pawn within 4m and LOS.
    // Enum/range/lifecycle checked server-side. Four-second cooldown rejects
    // duplicate/spammed intent; a later press is a new utterance. Current speech
    // replicates to late joiners. Disconnect leaves memory, removes live attention.
    UFUNCTION(Server, Reliable) void ServerTalkToSwarm(class AFlySwarmMob* Swarm, EFlyUtterance Intent);
    // Owning client -> server. Leave or echo a closed-vocabulary mark the colony shares with all players. Same pawn, range,
    // LOS and enum checks as ServerTalkToSwarm and the same per-player four-second cooldown (duplicates rejected).
    // Marks replicate to late joiners via VisibleInscriptions; a disconnect keeps the mark and memory.
    UFUNCTION(Server, Reliable) void ServerInscribeSwarm(class AFlySwarmMob* Swarm, EFlyMessageKind Kind);
    // No server RPC for replay: a remote client must not travel the whole session.
    /** Host-local listen controller may skip ship briefing, restart a watch, or return to PortHub. */
    void RequestBeginVoyage();
    void RequestRestartVoyage();
    void RequestReturnToPort();
    void RequestMissionBoardStep(int32 Direction);
    void RequestMissionBoardAccept();
    // Owning client -> server. Accepted only for the local listen host/solo controller while the voyage is in Briefing;
    // a repeat after the watch started is ignored. Late joiners receive the replicated voyage phase; disconnects do not cancel.
    UFUNCTION(Server, Reliable) void ServerBeginVoyage();
    // Owning client -> server. Local host/solo only, after a settled outcome, and only when ExpectedRound matches the
    // server round, so a duplicate or stale restart cannot skip a watch. Late joiners see the new round's snapshot.
    UFUNCTION(Server, Reliable) void ServerRestartVoyage(int32 ExpectedRound);
    // Owning client -> server. Accepted only for the local listen host/solo pawn after a voyage outcome; one travel per
    // outcome (latched), duplicates ignored. Late joiners and disconnects are unaffected: the whole session travels.
    UFUNCTION(Server, Reliable) void ServerReturnToPort();
    // Owning client -> server. Local host/solo pawn within the board zone only; each call steps once. Selection replicates.
    UFUNCTION(Server, Reliable) void ServerMissionBoardStep(int32 Direction);
    // Owning client -> server. Local host/solo pawn within the board zone; saves the contract and travels once (latched).
    UFUNCTION(Server, Reliable) void ServerMissionBoardAccept();
    class ACalmyBridgeTerminal* GetBridgeTerminal() const { return BridgeTerminal; }
private:
    void TravelToTrainingMap();
    void MaybeLiveHold1Teleport();
    bool bLiveHold1Teleported = false;
    bool bLiveHold1Delivered = false;
    int8 LiveHold1Mode = -1;
    bool bEnsuredAmbience = false;
    friend class UCalmyMeleeProbe;
    TSharedPtr<class SCalmySettingsWidget> SettingsWidget;
    TSharedPtr<class SCalmyJoinWidget> JoinWidget;
    TMap<TWeakObjectPtr<class UAudioComponent>,float> OriginalAudioVolumes;
    double NextAudioUpdate=0;
    bool bAppliedLocalPreferences=false;
    UPROPERTY() TObjectPtr<class ACalmyBridgeTerminal> BridgeTerminal;
    UPROPERTY() TObjectPtr<class ACalmyBridgeDisplays> BridgeDisplays;
    TWeakObjectPtr<AActor> PreviousMenuView;
    bool bPauseMenuOpen = false;
    bool bOwnsWorldPause = false;
    bool bCheckedStartupPause = false;
    bool bPreviousCursor = false, bPreviousClicks = false;
    bool bPreviousAutoCamera=true;
    TWeakObjectPtr<APawn> MenuPawn;
    double LastFlyTalkRequest = -100.;
    TWeakObjectPtr<class AFlySwarmMob> WheelSwarm;
    TWeakObjectPtr<APawn> WheelPawn;
    bool bWheelInputLease = false, bWheelPreviousCursor = false, bWheelPreviousClicks = false;
    int32 WheelSelection = -1;
    int32 WheelHitsAtOpen = 0;
    double SuppressGameplayUntil = -100.;
};

UCLASS()
class CALMYPIRATESCO_API ACalmyHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    // Review hooks for non-shipping probes: inject a worst-case HUD, read back the last layout check.
    static bool bReviewCrowded;
    // Probe-only: draw the breakdown arrow at this world location.
    static bool bReviewBreakdown;
    static FVector ReviewBreakdownLocation;
    static bool bLastLayoutValid;
    static int32 LastVisibleTop;
    static int32 LastTopItems;
private:
    void DrawFlyWheel(ACalmyPlayerController* PC);
    void DrawGameplayHUD(ACalmyPlayerController* PC,bool bShipReview);
    void Toast(const FString& Text,const FLinearColor& Color,float Seconds);
    FString LocalizeKeys(const FString& Text) const;
    FString ToastText;
    FLinearColor ToastColor=FLinearColor::White;
    double ToastUntil=0,TipUntil=0;
    int32 SeenVoyagePhase=-1,SeenRound=-1,SeenShiftPhase=-1,SeenSea=-1;
    bool bShownControlsTip=false;
    void DrawScaledText(const FString& Text,FLinearColor Color,float X,float Y,UFont* Font=nullptr,float Scale=1.f,bool ScalePosition=false);
};
