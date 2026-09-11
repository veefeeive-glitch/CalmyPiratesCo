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
    void ApplyAudioPreferences();
    void TogglePauseMenu();
    void SetPauseMenuOpen(bool bOpen);
    bool IsPauseMenuOpen() const { return bPauseMenuOpen; }
    bool IsSoloPause() const { return GetNetMode() == NM_Standalone; }
    void ResumeFromMenu();
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
    class ACalmyBridgeTerminal* GetBridgeTerminal() const { return BridgeTerminal; }
private:
    friend class UCalmyMeleeProbe;
    TSharedPtr<class SCalmySettingsWidget> SettingsWidget;
    TMap<TWeakObjectPtr<class UAudioComponent>,float> OriginalAudioVolumes;
    double NextAudioUpdate=0;
    bool bAppliedLocalPreferences=false;
    UPROPERTY() TObjectPtr<class ACalmyBridgeTerminal> BridgeTerminal;
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
private:
    void DrawFlyWheel(ACalmyPlayerController* PC);
    void DrawScaledText(const FString& Text,FLinearColor Color,float X,float Y,UFont* Font=nullptr,float Scale=1.f,bool ScalePosition=false);
};
