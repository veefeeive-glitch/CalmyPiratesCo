#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "CalmyGameMode.generated.h"

UCLASS()
class CALMYPIRATESCO_API ACalmyGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ACalmyGameMode();
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
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void TogglePauseMenu();
    void SetPauseMenuOpen(bool bOpen);
    bool IsPauseMenuOpen() const { return bPauseMenuOpen; }
    bool IsSoloPause() const { return GetNetMode() == NM_Standalone; }
    void ResumeFromMenu();
    void MenuCamera();
    void MenuMotion();
    void MenuLookDown();
    void MenuLookUp();
private:
    bool bPauseMenuOpen = false;
    bool bOwnsWorldPause = false;
    bool bCheckedStartupPause = false;
    bool bPreviousCursor = false, bPreviousClicks = false;
    TWeakObjectPtr<APawn> MenuPawn;
};

UCLASS()
class CALMYPIRATESCO_API ACalmyHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
};
