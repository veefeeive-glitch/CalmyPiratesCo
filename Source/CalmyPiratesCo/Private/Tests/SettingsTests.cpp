#include "Gameplay/CalmyPreferences.h"
#include "Gameplay/CalmySettingsWidget.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyPreferencesTest,"CalmyPiratesCo.Settings.ValidationAndDiskRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyPreferencesTest::RunTest(const FString&)
{
    auto* Settings=GEngine->GetGameUserSettings();const float SavedScale=Settings->ScalabilityQuality.ResolutionQuality;
    Settings->ScalabilityQuality.ResolutionQuality=0;
    FCalmyVideoOptions Video;Video.Read();TestTrue(TEXT("UE default render scale remains automatic"),Video.AutoScale);
    Video.Write(false);TestEqual(TEXT("Applying unrelated settings preserves project render scale"),Settings->ScalabilityQuality.ResolutionQuality,0.f);
    Settings->ScalabilityQuality.ResolutionQuality=SavedScale;
    auto* P=NewObject<UCalmyPreferences>();P->Bindings.Reset();FString Error;
    TestFalse(TEXT("Duplicate action key rejected"),P->SetBinding(EKeys::E,EKeys::W,Error));
    TestFalse(TEXT("Escape cannot strand player outside menu"),P->SetBinding(EKeys::E,EKeys::Escape,Error));
    TestFalse(TEXT("Existing camera shortcut cannot swallow a remapped action"),P->SetBinding(EKeys::E,EKeys::LeftBracket,Error));
    TestFalse(TEXT("Mouse axis is not a button binding"),P->SetBinding(EKeys::E,EKeys::MouseX,Error));
    TestTrue(TEXT("Rebind interaction"),P->SetBinding(EKeys::E,EKeys::Z,Error));
    TestEqual(TEXT("Changed key resolves"),P->KeyFor(EKeys::E),EKeys::Z);
    P->MasterVolume=9;P->Sensitivity=-50;P->FieldOfView=200;P->HUDScale=10;P->AAMethod=99;P->Sanitize();
    TestEqual(TEXT("Volume bounded"),P->MasterVolume,1.f);TestEqual(TEXT("Sensitivity bounded"),P->Sensitivity,.25f);
    TestEqual(TEXT("FOV bounded"),P->FieldOfView,110.f);TestEqual(TEXT("Unknown AA uses supported mode"),P->AAMethod,4);
    const FString Path=FPaths::ProjectSavedDir()/TEXT("Verification/Preferences-automation.ini");
    P->MasterVolume=.35;P->InvertY=true;P->SaveConfig(CPF_Config,*Path);
    auto* Loaded=NewObject<UCalmyPreferences>();Loaded->LoadConfig(UCalmyPreferences::StaticClass(),*Path);
    TestEqual(TEXT("Disk audio round trip"),Loaded->MasterVolume,.35f);TestTrue(TEXT("Disk invert round trip"),Loaded->InvertY);
    TestEqual(TEXT("Disk binding round trip"),Loaded->KeyFor(EKeys::E),EKeys::Z);
    Loaded->Bindings.Add(EKeys::E.GetFName(),EKeys::RightBracket);Loaded->Sanitize();
    TestEqual(TEXT("Invalid saved menu shortcut restores usable controls"),Loaded->KeyFor(EKeys::E),EKeys::E);
    IFileManager::Get().Delete(*Path);return true;
}
#endif
