#include "Gameplay/CalmyPreferences.h"
#include "Gameplay/CalmyGameMode.h"
#include "Gameplay/CalmyCharacter.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"

static bool IsBindableKey(const FKey& Key)
{
    return Key.IsValid()&&!Key.IsAxis1D()&&!Key.IsAxis2D()&&!Key.IsGamepadKey()
        &&Key!=EKeys::Escape&&Key!=EKeys::Enter&&Key!=EKeys::Up&&Key!=EKeys::Down&&Key!=EKeys::Left&&Key!=EKeys::Right
        &&Key!=EKeys::Tab&&Key!=EKeys::Tilde&&Key!=EKeys::LeftBracket&&Key!=EKeys::RightBracket;
}

UCalmyPreferences* UCalmyPreferences::Get()
{
    auto* P=GetMutableDefault<UCalmyPreferences>();
    static bool Migrated=false;
    if(!Migrated)
    {
        Migrated=true;
        // Preserve the user's previous camera choices on first upgrade.
        if(!GConfig->DoesSectionExist(TEXT("/Script/CalmyPiratesCo.CalmyPreferences"),GGameUserSettingsIni))
        {
            GConfig->GetBool(TEXT("Calmy.Accessibility"),TEXT("SteadyCamera"),P->SteadyCamera,GGameUserSettingsIni);
            GConfig->GetFloat(TEXT("Calmy.Accessibility"),TEXT("CameraMotion"),P->CameraMotion,GGameUserSettingsIni);
            GConfig->GetFloat(TEXT("Calmy.Accessibility"),TEXT("LookSensitivity"),P->Sensitivity,GGameUserSettingsIni);
        }
        P->Sanitize();
        GConfig->GetBool(TEXT("Calmy.Progress"),TEXT("TutorialCompleted"),P->bTutorialCompleted,GGameUserSettingsIni);
    }
    return P;
}
const TArray<FKey>& UCalmyPreferences::DefaultKeys()
{
    static const TArray<FKey> Keys={EKeys::W,EKeys::S,EKeys::A,EKeys::D,EKeys::SpaceBar,EKeys::E,EKeys::G,EKeys::LeftMouseButton,EKeys::Q,EKeys::H,EKeys::B,EKeys::F,EKeys::R,EKeys::V,EKeys::M};
    return Keys;
}
FString UCalmyPreferences::ActionLabel(const FKey& K)
{
    static const TCHAR* Names[]={TEXT("Вперёд"),TEXT("Назад"),TEXT("Влево"),TEXT("Вправо"),TEXT("Прыжок"),TEXT("Взаимодействие"),TEXT("Поставить груз"),TEXT("Удар / бросок"),TEXT("Передать груз"),TEXT("Помощь Миши"),TEXT("Споткнуться"),TEXT("Ремонт (удерживать)"),TEXT("Перезапуск смены"),TEXT("Режим камеры"),TEXT("Движение камеры")};
    const int32 I=DefaultKeys().IndexOfByKey(K);return I==INDEX_NONE?K.GetDisplayName().ToString():Names[I];
}
FKey UCalmyPreferences::KeyFor(const FKey& Default) const
{ const auto* Key=Bindings.Find(Default.GetFName());return Key&&Key->IsValid()?*Key:Default; }
bool UCalmyPreferences::SetBinding(const FKey& Default,const FKey& NewKey,FString& Error)
{
    if(!DefaultKeys().Contains(Default)||!IsBindableKey(NewKey))
    {Error=TEXT("Эта клавиша зарезервирована для меню. Выберите другую.");return false;}
    for(const FKey& K:DefaultKeys())if(K!=Default&&KeyFor(K)==NewKey)
    {Error=TEXT("Клавиша уже назначена: ")+ActionLabel(K);return false;}
    Bindings.Add(Default.GetFName(),NewKey);Error.Reset();return true;
}
void UCalmyPreferences::Sanitize()
{
    auto Clamp=[](float& V,float Min,float Max,float Fallback){V=FMath::IsFinite(V)?FMath::Clamp(V,Min,Max):Fallback;};
    Clamp(MasterVolume,0,1,1);Clamp(AmbienceVolume,0,1,1);Clamp(EffectsVolume,0,1,1);
    Clamp(Sensitivity,.25f,3,1);Clamp(FieldOfView,70,110,92);Clamp(CameraMotion,0,1,1);Clamp(HUDScale,.75f,1.5f,1);Clamp(Gamma,1.6f,2.8f,2.2f);
    if(AAMethod!=0&&AAMethod!=1&&AAMethod!=2&&AAMethod!=4)AAMethod=4;
    TSet<FKey> Used;bool Valid=true;
    for(const FKey& K:DefaultKeys()) {const FKey V=KeyFor(K);if(!IsBindableKey(V)||Used.Contains(V))Valid=false;Used.Add(V);}
    if(!Valid)Bindings.Reset();
}
void UCalmyPreferences::Persist()
{
    Sanitize();SaveConfig(CPF_Config,*GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Calmy.Accessibility"),TEXT("SteadyCamera"),SteadyCamera,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Calmy.Accessibility"),TEXT("CameraMotion"),CameraMotion,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Calmy.Accessibility"),TEXT("LookSensitivity"),Sensitivity,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Calmy.Progress"),TEXT("TutorialCompleted"),bTutorialCompleted,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void UCalmyPreferences::Apply(ACalmyPlayerController* PC) const
{
    auto Set=[](const TCHAR* Name,int32 Value){if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))C->Set(Value,ECVF_SetByGameSetting);};
    Set(TEXT("r.MotionBlurQuality"),MotionBlur?3:0);Set(TEXT("r.BloomQuality"),Bloom?5:0);Set(TEXT("r.AntiAliasingMethod"),AAMethod);
    if(GEngine)GEngine->DisplayGamma=Gamma;
    if(PC) {if(auto* P=Cast<ACalmyCharacter>(PC->GetPawn()))P->ApplyLocalPreferences();PC->ApplyAudioPreferences();}
}
