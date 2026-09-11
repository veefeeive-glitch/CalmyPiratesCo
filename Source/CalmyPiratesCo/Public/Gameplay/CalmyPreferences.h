#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "CalmyPreferences.generated.h"

// Local preferences only. Gameplay authority, physics and neural frequency are
// deliberately not graphics settings and never depend on these values.
UCLASS(Config=GameUserSettings)
class CALMYPIRATESCO_API UCalmyPreferences : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config) float MasterVolume=1.f;
    UPROPERTY(Config) float AmbienceVolume=1.f;
    UPROPERTY(Config) float EffectsVolume=1.f;
    UPROPERTY(Config) bool MuteUnfocused=false;
    UPROPERTY(Config) float Sensitivity=1.f;
    UPROPERTY(Config) bool InvertY=false;
    UPROPERTY(Config) float FieldOfView=92.f;
    UPROPERTY(Config) bool SteadyCamera=false;
    UPROPERTY(Config) float CameraMotion=1.f;
    UPROPERTY(Config) bool ShowFPS=false;
    UPROPERTY(Config) float HUDScale=1.f;
    UPROPERTY(Config) bool ShowCrosshair=true;
    UPROPERTY(Config) bool MotionBlur=true;
    UPROPERTY(Config) bool Bloom=true;
    UPROPERTY(Config) int32 AAMethod=4; // TSR; 0 off, 1 FXAA, 2 TAA, 4 TSR.
    UPROPERTY(Config) float Gamma=2.2f;
    UPROPERTY(Config) TMap<FName,FKey> Bindings;
    static UCalmyPreferences* Get();
    static const TArray<FKey>& DefaultKeys();
    static FString ActionLabel(const FKey& Key);
    FKey KeyFor(const FKey& Default) const;
    bool SetBinding(const FKey& Default, const FKey& NewKey, FString& Error);
    void Sanitize();
    void Persist();
    void Apply(class ACalmyPlayerController* PC) const;
};
