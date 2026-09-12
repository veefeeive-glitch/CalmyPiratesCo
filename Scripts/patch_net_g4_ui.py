# -*- coding: utf-8 -*-
"""Surgical Net G4 patches for GameMode / BridgeTerminal / tests / smoke."""
from pathlib import Path
import re

ROOT = Path(r"E:\Game\CalmyPiratesCoUE")


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    path = ROOT / rel
    path.write_text(text.replace("\r\n", "\n"), encoding="utf-8", newline="\n")
    print(f"PATCHED {rel} {path.stat().st_size}")


# --- GameMode.h ---
gmh = read("Source/CalmyPiratesCo/Public/Gameplay/CalmyGameMode.h")
if "InitGame" not in gmh:
    gmh = gmh.replace(
        "\tACalmyGameMode();\n    virtual void BeginPlay() override;",
        "\tACalmyGameMode();\n\tvirtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;\n    virtual void BeginPlay() override;",
    )
if "OpenJoinDialog" not in gmh:
    gmh = gmh.replace(
        "    TSharedPtr<class SCalmySettingsWidget> GetSettingsWidget() const {return SettingsWidget;}\n    void ApplyAudioPreferences();",
        "    TSharedPtr<class SCalmySettingsWidget> GetSettingsWidget() const {return SettingsWidget;}\n"
        "    void OpenJoinDialog();\n"
        "    void CloseJoinDialog();\n"
        "    bool IsJoinDialogOpen() const {return JoinWidget.IsValid();}\n"
        "    bool HostLanSession(FString* OutError=nullptr);\n"
        "    bool JoinLanSession(const FString& Address, FString* OutError=nullptr);\n"
        "    void CopyLanEndpointToClipboard();\n"
        "    void ApplyAudioPreferences();",
    )
if "JoinWidget" not in gmh:
    gmh = gmh.replace(
        "    TSharedPtr<class SCalmySettingsWidget> SettingsWidget;\n",
        "    TSharedPtr<class SCalmySettingsWidget> SettingsWidget;\n"
        "    TSharedPtr<class SCalmyJoinWidget> JoinWidget;\n",
    )
write("Source/CalmyPiratesCo/Public/Gameplay/CalmyGameMode.h", gmh)

# --- GameMode.cpp ---
gmc = read("Source/CalmyPiratesCo/Private/Gameplay/CalmyGameMode.cpp")
if '#include "Core/CalmySession.h"' not in gmc:
    gmc = gmc.replace(
        '#include "Gameplay/CalmySettingsWidget.h"\n',
        '#include "Gameplay/CalmySettingsWidget.h"\n'
        '#include "Gameplay/CalmyJoinWidget.h"\n'
        '#include "Core/CalmySession.h"\n'
        '#include "GameFramework/GameSession.h"\n'
        '#include "HAL/PlatformApplicationMisc.h"\n',
    )

# InputKey join dialog
if "IsJoinDialogOpen()" not in gmc:
    gmc = gmc.replace(
        "        if(IsSettingsOpen()) {if(Args.Key==EKeys::Escape)CloseSettings();return true;}",
        "        if(IsJoinDialogOpen()) {if(Args.Key==EKeys::Escape)CloseJoinDialog();return true;}\n"
        "        if(IsSettingsOpen()) {if(Args.Key==EKeys::Escape)CloseSettings();return true;}",
    )

# MenuClick
gmc = gmc.replace(
    "void ACalmyPlayerController::MenuClick()\n{ if(IsSettingsOpen())return;",
    "void ACalmyPlayerController::MenuClick()\n{ if(IsSettingsOpen()||IsJoinDialogOpen())return;",
)

# Insert session methods after CloseSettings
if "OpenJoinDialog" not in gmc:
    close_settings = """void ACalmyPlayerController::CloseSettings()
{
    if(!SettingsWidget.IsValid())return;
    SettingsWidget->RevertVideo();
    if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(SettingsWidget.ToSharedRef());
    SettingsWidget.Reset();FlushPressedKeys();
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
"""
    insert = close_settings + """void ACalmyPlayerController::OpenJoinDialog()
{
    if(!IsLocalController()||JoinWidget.IsValid()||!GEngine||!GEngine->GameViewport)return;
    if(!bPauseMenuOpen)SetPauseMenuOpen(true);
    CloseSettings();
    JoinWidget=SNew(SCalmyJoinWidget).Controller(this);
    GEngine->GameViewport->AddViewportWidgetContent(JoinWidget.ToSharedRef(),110);
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(JoinWidget);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    FSlateApplication::Get().SetKeyboardFocus(JoinWidget);
}
void ACalmyPlayerController::CloseJoinDialog()
{
    if(!JoinWidget.IsValid())return;
    if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(JoinWidget.ToSharedRef());
    JoinWidget.Reset();FlushPressedKeys();
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();
    if(BridgeTerminal)BridgeTerminal->Refresh();
}
bool ACalmyPlayerController::HostLanSession(FString* OutError)
{
    if(!IsLocalController()) { if(OutError)*OutError=TEXT("Local controller required"); return false; }
    CloseJoinDialog();
    const bool Ok=FCalmySession::HostLanListen(GetWorld(), FCalmySession::DefaultPort, OutError);
    if(BridgeTerminal)BridgeTerminal->Refresh();
    if(Ok && bOwnsWorldPause && GetWorld() && GetWorld()->IsPaused()) { SetPause(false); bOwnsWorldPause=false; }
    return Ok;
}
bool ACalmyPlayerController::JoinLanSession(const FString& Address, FString* OutError)
{
    if(!IsLocalController()) { if(OutError)*OutError=TEXT("Local controller required"); return false; }
    CloseSettings();
    if(bOwnsWorldPause && GetWorld() && GetWorld()->IsPaused()) { SetPause(false); bOwnsWorldPause=false; }
    SetPauseMenuOpen(false);
    return FCalmySession::JoinLan(this, Address, OutError);
}
void ACalmyPlayerController::CopyLanEndpointToClipboard()
{
    if(!IsLocalController()||!GetWorld())return;
    if(GetNetMode()!=NM_ListenServer)
    {
        UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION copy_skipped role=%s"), *FCalmySession::RoleLabel(GetWorld()));
        return;
    }
    const FString Endpoint=FCalmySession::DescribeListenEndpoint(GetWorld());
    FPlatformApplicationMisc::ClipboardCopy(*Endpoint);
    UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION COPY_LAN %s"), *Endpoint);
}
"""
    if close_settings not in gmc:
        raise SystemExit("CloseSettings block not found exactly")
    gmc = gmc.replace(close_settings, insert, 1)

# Toggle / menus
gmc = gmc.replace(
    "void ACalmyPlayerController::MenuUp(){if(!IsSettingsOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(-1);}\n"
    "void ACalmyPlayerController::MenuDown(){if(!IsSettingsOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(1);}\n"
    "void ACalmyPlayerController::MenuConfirm(){if(IsSettingsOpen())return;if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Activate();else ResumeFromMenu();}\n\n"
    "void ACalmyPlayerController::TogglePauseMenu() {if(IsSettingsOpen()){CloseSettings();return;} if (bWheelInputLease) CloseFlyWheel(); else SetPauseMenuOpen(!bPauseMenuOpen); }",
    "void ACalmyPlayerController::MenuUp(){if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(-1);}\n"
    "void ACalmyPlayerController::MenuDown(){if(!IsSettingsOpen()&&!IsJoinDialogOpen()&&bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Step(1);}\n"
    "void ACalmyPlayerController::MenuConfirm(){if(IsSettingsOpen()||IsJoinDialogOpen())return;if(bPauseMenuOpen&&BridgeTerminal)BridgeTerminal->Activate();else ResumeFromMenu();}\n\n"
    "void ACalmyPlayerController::TogglePauseMenu() {if(IsJoinDialogOpen()){CloseJoinDialog();return;} if(IsSettingsOpen()){CloseSettings();return;} if (bWheelInputLease) CloseFlyWheel(); else SetPauseMenuOpen(!bPauseMenuOpen); }",
)

gmc = gmc.replace(
    "    if (bOpen) CloseFlyWheel();else CloseSettings();",
    "    if (bOpen) CloseFlyWheel();else {CloseSettings();CloseJoinDialog();}",
)

if "CloseJoinDialog();" not in gmc.split("EndPlay")[1][:400]:
    gmc = gmc.replace(
        "void ACalmyPlayerController::EndPlay(const EEndPlayReason::Type Reason)\n{\n    CloseFlyWheel();\n    SetPauseMenuOpen(false);",
        "void ACalmyPlayerController::EndPlay(const EEndPlayReason::Type Reason)\n{\n    CloseFlyWheel();\n    CloseJoinDialog();\n    CloseSettings();\n    SetPauseMenuOpen(false);",
    )

if "ACalmyGameMode::InitGame" not in gmc:
    gmc = gmc.replace(
        "ACalmyGameMode::ACalmyGameMode()\n{\n\tDefaultPawnClass = ACalmyCharacter::StaticClass();\n\tHUDClass = ACalmyHUD::StaticClass();\n    PlayerControllerClass = ACalmyPlayerController::StaticClass();\n}\n",
        "ACalmyGameMode::ACalmyGameMode()\n{\n\tDefaultPawnClass = ACalmyCharacter::StaticClass();\n\tHUDClass = ACalmyHUD::StaticClass();\n    PlayerControllerClass = ACalmyPlayerController::StaticClass();\n}\n\n"
        "void ACalmyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)\n"
        "{\n\tSuper::InitGame(MapName, Options, ErrorMessage);\n\tif (GameSession)\n\t{\n\t\tGameSession->MaxPlayers = FCalmySession::MaxCrew;\n\t}\n}\n",
    )

gmc = re.sub(
    r"void ACalmyGameMode::PreLogin\(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage\)\n\{\n\tSuper::PreLogin\(Options, Address, UniqueId, ErrorMessage\);\n\tif \(GetNumPlayers\(\) >= 4\) ErrorMessage = TEXT\(\"This build supports at most four crew members\.\"\);\n\}",
    "void ACalmyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)\n"
    "{\n\tSuper::PreLogin(Options, Address, UniqueId, ErrorMessage);\n\tif (!ErrorMessage.IsEmpty()) return;\n"
    "\tif (!FCalmySession::CanAdmit(GetNumPlayers()))\n\t{\n\t\tErrorMessage = TEXT(\"This build supports at most four crew members.\");\n\t}\n}",
    gmc,
    count=1,
)

gmc = gmc.replace(
    'GetNetMode() == NM_Client ? TEXT("CLIENT") : (GetNetMode() == NM_ListenServer ? TEXT("HOST") : TEXT("SOLO"))), FLinearColor::White, 30, 82);',
    '*FCalmySession::StatusLine(GetWorld())), FLinearColor::White, 30, 82);',
)

write("Source/CalmyPiratesCo/Private/Gameplay/CalmyGameMode.cpp", gmc)

# --- BridgeTerminal.h Count ---
bth = read("Source/CalmyPiratesCo/Public/Gameplay/CalmyBridgeTerminal.h")
bth = bth.replace(
    "int32 Count() const { return Page==1?5:Page==2?2:4; }",
    "int32 Count() const { return Page==1?5:Page==2?2:Page==3?4:5; }",
)
write("Source/CalmyPiratesCo/Public/Gameplay/CalmyBridgeTerminal.h", bth)

# --- BridgeTerminal.cpp --- overwrite Activate+Paint sections via full file from known good content is safer
btc = read("Source/CalmyPiratesCo/Private/Gameplay/CalmyBridgeTerminal.cpp")
if '#include "Core/CalmySession.h"' not in btc:
    btc = btc.replace(
        '#include "Gameplay/CalmyGameMode.h"\n',
        '#include "Gameplay/CalmyGameMode.h"\n#include "Core/CalmySession.h"\n',
    )

# Replace Activate function
activate_new = r'''void ACalmyBridgeTerminal::Activate()
{
    if(!PC||!PC->IsPauseMenuOpen()||PC->IsJoinDialogOpen())return;
    if(Page==0)
    {
        if(Selected==0){PC->ResumeFromMenu();return;}
        if(Selected==1){PC->OpenSettings();return;}
        if(Selected==2){PC->RequestReplayTutorial();return;}
        if(Selected==3){Page=3;Selected=0;Refresh();return;}
        Page=2;Selected=0;
    }
    else if(Page==1)
    {
        if(Selected==0)PC->MenuCamera();else if(Selected==1)PC->MenuMotion();
        else if(Selected==2)PC->MenuLookDown();else if(Selected==3)PC->MenuLookUp();else {Page=0;Selected=0;}
    }
    else if(Page==3)
    {
        if(Selected==0)
        {
            FString Error;
            if(!PC->HostLanSession(&Error) && !Error.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("CALMY_SESSION host_ui %s"), *Error);
            }
            Refresh();
            return;
        }
        if(Selected==1){PC->OpenJoinDialog();return;}
        if(Selected==2){PC->CopyLanEndpointToClipboard();Refresh();return;}
        Page=0;Selected=3;
    }
    else if(Selected==0){Page=0;Selected=0;}
    else UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false);
    Refresh();
}
'''
btc = re.sub(
    r"void ACalmyBridgeTerminal::Activate\(\)\n\{.*?\n    Refresh\(\);\n\}\nvoid ACalmyBridgeTerminal::Paint",
    activate_new + "void ACalmyBridgeTerminal::Paint",
    btc,
    count=1,
    flags=re.S,
)

paint_new = r'''void ACalmyBridgeTerminal::Paint(UCanvas* C,int32 W,int32 H)
{
    if(!C||!PC)return;
    auto Rect=[&](float X,float Y,float SX,float SY,FLinearColor Color){FCanvasTileItem T(FVector2D(X,Y),FVector2D(SX,SY),Color);T.BlendMode=SE_BLEND_Opaque;C->DrawItem(T);};
    auto Text=[&](const FString& S,float X,float Y,float Scale,FLinearColor Color){FCanvasTextItem T(FVector2D(X,Y),FText::FromString(S),GEngine->GetMediumFont(),Color);T.Scale=FVector2D(Scale);C->DrawItem(T);};
    const FLinearColor White(.72,.82,.78),Dim(.27,.48,.48),Accent(.38,.82,.7);
    Rect(0,0,W,H,FLinearColor(.009,.02,.024));Rect(46,128,932,1,Dim);
    Text(TEXT("C A L M Y  /  NIGHT WATCH"),48,38,1.65,White);
    const FString Subtitle =
        Page==1?TEXT("BODYCAM CONFIGURATION"):
        Page==2?TEXT("LEAVE THIS SESSION?"):
        Page==3?TEXT("CREW SESSION / LAN"):
        TEXT("WHEELHOUSE  /  CREW TERMINAL  01");
    Text(Subtitle,48,94,.95,Dim);
    TArray<FString> Labels;
    auto* P=Cast<ACalmyCharacter>(PC->GetPawn());
    if(Page==0)Labels={TEXT("CONTINUE WATCH"),TEXT("SETTINGS / НАСТРОЙКИ"),TEXT("REPLAY TRAINING / ОБУЧЕНИЕ СНОВА"),TEXT("CREW SESSION / ЭКИПАЖ"),TEXT("LEAVE SESSION")};
    else if(Page==2)Labels={TEXT("STAY ON BOARD"),TEXT("CONFIRM / EXIT TO DESKTOP")};
    else if(Page==3)
    {
        const bool Listening = PC->GetNetMode()==NM_ListenServer;
        Labels={
            Listening?TEXT("LAN LISTEN ACTIVE"):TEXT("HOST LAN WATCH"),
            FString::Printf(TEXT("JOIN LAN  [%s]"), *FCalmySession::LoadSavedJoinAddress()),
            Listening?FString::Printf(TEXT("COPY ADDRESS  %s"), *FCalmySession::DescribeListenEndpoint(PC->GetWorld())):TEXT("COPY ADDRESS  (host first)"),
            TEXT("BACK")
        };
    }
    else Labels={FString::Printf(TEXT("CAMERA       %s   [V]"),P&&P->IsSteadyCamera()?TEXT("STEADY"):TEXT("BODYCAM")),FString::Printf(TEXT("MOTION       %.0f%%   [M]"),P?P->GetCameraMotionScale()*100:0),FString::Printf(TEXT("LOOK -       %.2fx   [[]"),P?P->GetLookSensitivity():1),TEXT("LOOK +       []]"),TEXT("BACK")};
    for(int32 I=0;I<Labels.Num();++I)
    {
        const float Y=176+I*52;
        if(I==Selected){Rect(46,Y,932,44,FLinearColor(.04,.11,.115));Rect(46,Y,3,44,Accent);}
        Text(Labels[I],68,Y+10,1.05,I==Selected?White:Dim);
    }
    Rect(46,457,932,1,Dim);
    Text(PC->IsSoloPause()?TEXT("SOLO / WORLD PAUSED"):TEXT("ONLINE / WORLD REMAINS LIVE"),48,476,.78,Accent);
    Text(Page==3?*FCalmySession::StatusLine(PC->GetWorld()):TEXT("MOUSE / ARROWS / ENTER     ESC BACK TO DECK"),Page==3?320.f:475.f,476,.62,Dim);
}
'''
btc = re.sub(
    r"void ACalmyBridgeTerminal::Paint\(UCanvas\* C,int32 W,int32 H\)\n\{.*\}\s*\Z",
    paint_new,
    btc,
    count=1,
    flags=re.S,
)
write("Source/CalmyPiratesCo/Private/Gameplay/CalmyBridgeTerminal.cpp", btc)

# BridgeMenuTests
write(
    "Source/CalmyPiratesCo/Private/Tests/BridgeMenuTests.cpp",
    """#include "Gameplay/CalmyBridgeTerminal.h"
#include "Core/CalmySession.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMenuRows,"CalmyPiratesCo.UI.BridgeTerminalHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBridgeMenuRows::RunTest(const FString&)
{
    TestEqual(TEXT("First action"),ACalmyBridgeTerminal::RowAt(FVector2D(500,198),5),0);
    TestEqual(TEXT("Settings"),ACalmyBridgeTerminal::RowAt(FVector2D(500,250),5),1);
    TestEqual(TEXT("Gap not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,224),5),INDEX_NONE);
    TestEqual(TEXT("Outside screen"),ACalmyBridgeTerminal::RowAt(FVector2D(-1,198),5),INDEX_NONE);
    TestEqual(TEXT("Replay training"),ACalmyBridgeTerminal::RowAt(FVector2D(500,302),5),2);
    TestEqual(TEXT("Crew session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,354),5),3);
    TestEqual(TEXT("Leave session"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    TestEqual(TEXT("Below leave not clickable"),ACalmyBridgeTerminal::RowAt(FVector2D(500,458),5),INDEX_NONE);
    TestEqual(TEXT("Settings last row on bodycam page"),ACalmyBridgeTerminal::RowAt(FVector2D(500,406),5),4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmySessionHelpers,"CalmyPiratesCo.Core.SessionLanHelpers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmySessionHelpers::RunTest(const FString&)
{
    TestTrue(TEXT("admit empty"), FCalmySession::CanAdmit(0));
    TestTrue(TEXT("admit three"), FCalmySession::CanAdmit(3));
    TestFalse(TEXT("reject fourth already connected"), FCalmySession::CanAdmit(4));
    FString Clean, Error;
    TestTrue(TEXT("sanitize ip port"), FCalmySession::SanitizeJoinAddress(TEXT(" 192.168.0.12:17777 "), Clean, Error));
    TestEqual(TEXT("clean address"), Clean, FString(TEXT("192.168.0.12:17777")));
    TestFalse(TEXT("reject path"), FCalmySession::SanitizeJoinAddress(TEXT("/Game/Maps/WaterLab"), Clean, Error));
    TestFalse(TEXT("reject query"), FCalmySession::SanitizeJoinAddress(TEXT("127.0.0.1:17777?listen"), Clean, Error));
    TestEqual(TEXT("max crew"), FCalmySession::MaxCrew, 4);
    return true;
}
#endif
""",
)

print("surgical patches complete")
print("OpenJoinDialog" in read("Source/CalmyPiratesCo/Private/Gameplay/CalmyGameMode.cpp"))
print("CREW SESSION" in read("Source/CalmyPiratesCo/Private/Gameplay/CalmyBridgeTerminal.cpp"))
print("InitGame" in read("Source/CalmyPiratesCo/Public/Gameplay/CalmyGameMode.h"))
