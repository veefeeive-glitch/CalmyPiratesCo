#include "Gameplay/CalmySettingsWidget.h"
#include "Gameplay/CalmyGameMode.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/UnrealType.h"

static FText Txt(const FString& S){return FText::FromString(S);}
static const FTextBlockStyle& SettingsButtonText()
{
    static const FTextBlockStyle Style=FTextBlockStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
        .SetFont(FCoreStyle::GetDefaultFontStyle("Regular",13)).SetColorAndOpacity(FLinearColor::White);
    return Style;
}
static FString QualityName(int32 Q)
{static const TCHAR* Names[]={TEXT("Низкое"),TEXT("Среднее"),TEXT("Высокое"),TEXT("Epic")};return Names[FMath::Clamp(Q,0,3)];}
void FCalmyVideoOptions::Read()
{
    auto* S=GEngine->GetGameUserSettings();Resolution=S->GetScreenResolution();Mode=S->GetFullscreenMode();VSync=S->IsVSyncEnabled();Limit=S->GetFrameRateLimit();
    float N,Min,Max;S->GetResolutionScaleInformationEx(N,Scale,Min,Max);
    AutoScale=Scale<=0;Scale=AutoScale?100.f:FMath::Clamp(Scale,50.f,100.f);
    const int32 Q[]={S->GetViewDistanceQuality(),S->GetAntiAliasingQuality(),S->GetShadowQuality(),S->GetGlobalIlluminationQuality(),S->GetReflectionQuality(),S->GetTextureQuality(),S->GetVisualEffectQuality(),S->GetPostProcessingQuality(),S->GetFoliageQuality(),S->GetShadingQuality()};
    for(int32 I=0;I<10;++I)Quality[I]=FMath::Clamp(Q[I],0,3);
}
void FCalmyVideoOptions::Write(bool ResolutionToo) const
{
    auto* S=GEngine->GetGameUserSettings();
    if(ResolutionToo){S->SetScreenResolution(Resolution);S->SetFullscreenMode(Mode);}
    S->SetVSyncEnabled(VSync);S->SetFrameRateLimit(Limit);S->SetResolutionScaleValueEx(Scale);
    // Zero is UE's project-default screen percentage, not a 0% render target.
    if(AutoScale)S->ScalabilityQuality.ResolutionQuality=0;
    S->SetViewDistanceQuality(Quality[0]);S->SetAntiAliasingQuality(Quality[1]);S->SetShadowQuality(Quality[2]);S->SetGlobalIlluminationQuality(Quality[3]);S->SetReflectionQuality(Quality[4]);
    S->SetTextureQuality(Quality[5]);S->SetVisualEffectQuality(Quality[6]);S->SetPostProcessingQuality(Quality[7]);S->SetFoliageQuality(Quality[8]);S->SetShadingQuality(Quality[9]);
}
void SCalmySettingsWidget::Construct(const FArguments& Args)
{
    PC=Args._Controller;Draft.Reset(DuplicateObject<UCalmyPreferences>(UCalmyPreferences::Get(),GetTransientPackage()));Video.Read();
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
    Resolutions.RemoveAll([](const FIntPoint& P){return P.X<800||P.Y<600;});
    Resolutions.AddUnique(Video.Resolution);Resolutions.AddUnique(FIntPoint(1280,720));Resolutions.AddUnique(FIntPoint(1280,800));
    Resolutions.Sort([](const FIntPoint& A,const FIntPoint& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});
    TSharedPtr<SHorizontalBox> Tabs;
    ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(24).BorderBackgroundColor(FLinearColor(.008,.018,.026,1))[
        SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(6,4)[SNew(STextBlock).Text(Txt(TEXT("НАСТРОЙКИ"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",27))]
        +SVerticalBox::Slot().AutoHeight().Padding(6,4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).Text(Txt(TEXT("Изменения вступают в силу после «Применить». Escape — назад."))).ColorAndOpacity(FLinearColor(.6,.72,.75))]
        +SVerticalBox::Slot().AutoHeight().Padding(0,14)[SAssignNew(Tabs,SHorizontalBox)]
        +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(PageBody,SVerticalBox)]]
        +SVerticalBox::Slot().AutoHeight().Padding(6,10)[SAssignNew(Notice,STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.6,.95,.83))]
        +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().Padding(5)[SNew(SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,8)).Text(Txt(TEXT("По умолчанию"))).OnClicked_Lambda([this]{if(VideoDeadline>0)RevertVideo();Draft.Reset(NewObject<UCalmyPreferences>());Draft->Bindings.Reset();Draft->MasterVolume=Draft->AmbienceVolume=Draft->EffectsVolume=1;Draft->Sensitivity=1;Draft->FieldOfView=92;Draft->CameraMotion=1;Draft->SteadyCamera=Draft->InvertY=Draft->ShowFPS=Draft->MuteUnfocused=false;Draft->HUDScale=1;Draft->ShowCrosshair=Draft->MotionBlur=Draft->Bloom=true;Draft->AAMethod=4;Draft->Gamma=2.2;Video.Limit=60;Video.VSync=false;Video.Scale=100;Video.AutoScale=true;for(int32& Q:Video.Quality)Q=2;BuildPage();Notice->SetText(Txt(TEXT("Значения сброшены в редакторе. Нажмите «Применить», чтобы сохранить.")));return FReply::Handled();})]
            +SHorizontalBox::Slot().FillWidth(1)[SNew(SSpacer)]
            +SHorizontalBox::Slot().AutoWidth().Padding(5)[SAssignNew(KeepButton,SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,10)).Text(Txt(TEXT("Сохранить экран"))).Visibility_Lambda([this]{return VideoDeadline>0?EVisibility::Visible:EVisibility::Collapsed;}).OnClicked_Lambda([this]{KeepVideo();return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(5)[SNew(SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,8)).Text(Txt(TEXT("Вернуть экран"))).Visibility_Lambda([this]{return VideoDeadline>0?EVisibility::Visible:EVisibility::Collapsed;}).OnClicked_Lambda([this]{RevertVideo();return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(5)[SAssignNew(ApplyButton,SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,10)).Text(Txt(TEXT("Применить"))).IsEnabled_Lambda([this]{return VideoDeadline<=0;}).OnClicked_Lambda([this]{Apply();return FReply::Handled();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(5)[SAssignNew(BackButton,SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,10)).Text(Txt(TEXT("Назад"))).OnClicked_Lambda([this]{Close();return FReply::Handled();})]
        ]
    ]];
    const TCHAR* Titles[]={TEXT("Экран"),TEXT("Графика"),TEXT("Звук"),TEXT("Управление"),TEXT("Камера и интерфейс")};
    TabButtons.SetNum(5);
    for(int32 I=0;I<5;++I)Tabs->AddSlot().FillWidth(1).Padding(3)[SAssignNew(TabButtons[I],SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(8,12)).Text(Txt(Titles[I])).ButtonColorAndOpacity_Lambda([this,I]{return Page==I?FLinearColor(.12,.42,.39):FLinearColor(.09,.13,.17);}).OnClicked_Lambda([this,I]{SetPage(I);return FReply::Handled();})];
    BuildPage();
}
void SCalmySettingsWidget::Row(const FString& Label,TSharedRef<SWidget> Control,const FString& Help)
{
    PageBody->AddSlot().AutoHeight().Padding(4,5)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.045,.075,.09)).Padding(14)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))]
            +SVerticalBox::Slot().AutoHeight().Padding(0,4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).Text(Txt(Help)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.55,.66,.69)).Visibility(Help.IsEmpty()?EVisibility::Collapsed:EVisibility::Visible)]]
        +SHorizontalBox::Slot().FillWidth(.85f).VAlign(VAlign_Center).Padding(18,0)[Control]]];
}
void SCalmySettingsWidget::Toggle(const FString& Label,bool* V,const FString& Help)
{Row(Label,SNew(SCheckBox).IsChecked_Lambda([V]{return *V?ECheckBoxState::Checked:ECheckBoxState::Unchecked;}).OnCheckStateChanged_Lambda([V](ECheckBoxState S){*V=S==ECheckBoxState::Checked;}),Help);}
void SCalmySettingsWidget::Slider(const FString& Label,float* V,float Min,float Max,float Step,const FString& Suffix,bool* Automatic)
{
    Row(Label,SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(SSlider).IsEnabled_Lambda([Automatic]{return !Automatic||!*Automatic;}).MinValue(Min).MaxValue(Max).StepSize(Step).Value_Lambda([V]{return *V;}).OnValueChanged_Lambda([V,Step](float X){*V=FMath::RoundToFloat(X/Step)*Step;})]
        +SHorizontalBox::Slot().AutoWidth().Padding(14,0)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinDesiredWidth(90).Text_Lambda([V,Suffix,Automatic]{return Txt(Automatic&&*Automatic?TEXT("Авто"):FString::Printf(TEXT("%.2f %s"),*V,*Suffix));})]);
}
void SCalmySettingsWidget::Choice(const FString& Label,TFunction<FString()> Value,TFunction<void(int32)> Change,const FString& Help)
{
    Row(Label,SNew(SHorizontalBox)
        +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,8)).Text(Txt(TEXT("◀"))).OnClicked_Lambda([Change]{Change(-1);return FReply::Handled();})]
        +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).Text_Lambda([Value]{return Txt(Value());})]
        +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,8)).Text(Txt(TEXT("▶"))).OnClicked_Lambda([Change]{Change(1);return FReply::Handled();})],Help);
}
void SCalmySettingsWidget::SetPage(int32 I){Page=FMath::Clamp(I,0,4);Capturing=FKey();BuildPage();}
void SCalmySettingsWidget::BuildPage()
{
    PageBody->ClearChildren();BindingButtons.Reset();auto* P=Draft.Get();
    if(Page==0)
    {
        Choice(TEXT("Режим экрана"),[this]{return Video.Mode==EWindowMode::Fullscreen?TEXT("Полный экран"):Video.Mode==EWindowMode::WindowedFullscreen?TEXT("Без рамки"):TEXT("Окно");},[this](int32 D){Video.Mode=static_cast<EWindowMode::Type>((int32(Video.Mode)+D+3)%3);});
        Choice(TEXT("Разрешение"),[this]{return FString::Printf(TEXT("%d × %d"),Video.Resolution.X,Video.Resolution.Y);},[this](int32 D){const int32 I=FMath::Max(0,Resolutions.IndexOfByKey(Video.Resolution));Video.Resolution=Resolutions[(I+D+Resolutions.Num())%Resolutions.Num()];},TEXT("В режиме без рамки используется размер рабочего стола."));
        Toggle(TEXT("Вертикальная синхронизация"),&Video.VSync,TEXT("Убирает разрывы изображения; может увеличить задержку управления."));
        Choice(TEXT("Ограничение FPS"),[this]{return Video.Limit==0?TEXT("Без ограничения"):FString::Printf(TEXT("%.0f FPS"),Video.Limit);},[this](int32 D){const TArray<float> Limits={30,45,60,90,120,144,165,240,0};int32 I=FMath::Max(0,Limits.IndexOfByKey(Video.Limit));Video.Limit=Limits[(I+D+Limits.Num())%Limits.Num()];});
        Toggle(TEXT("Автоматический масштаб рендеринга"),&Video.AutoScale,TEXT("Использует масштаб проекта. Выключите для ручного выбора."));
        Slider(TEXT("Масштаб рендеринга"),&Video.Scale,50,100,5,TEXT("%"),&Video.AutoScale);
        Slider(TEXT("Яркость (гамма)"),&P->Gamma,1.6,2.8,.1,TEXT(""));
        Toggle(TEXT("Показывать FPS и время кадра"),&P->ShowFPS);
    }
    else if(Page==1)
    {
        Choice(TEXT("Общее качество"),[this]{for(int32 Q:Video.Quality)if(Q!=Video.Quality[0])return FString(TEXT("Свои настройки"));return QualityName(Video.Quality[0]);},[this](int32 D){int32 Q=(Video.Quality[0]+D+4)%4;for(int32& V:Video.Quality)V=Q;},TEXT("Среднее — для скорости. Высокое — баланс освещения и нагрузки. Epic — максимальная нагрузка."));
        const TCHAR* Names[]={TEXT("Дальность прорисовки"),TEXT("Качество сглаживания"),TEXT("Тени"),TEXT("Глобальное освещение"),TEXT("Отражения"),TEXT("Текстуры"),TEXT("Эффекты"),TEXT("Постобработка"),TEXT("Растительность"),TEXT("Шейдинг")};
        for(int32 I=0;I<10;++I)Choice(Names[I],[this,I]{return QualityName(Video.Quality[I]);},[this,I](int32 D){Video.Quality[I]=(Video.Quality[I]+D+4)%4;});
        Choice(TEXT("Метод сглаживания"),[P]{return P->AAMethod==4?TEXT("TSR"):P->AAMethod==2?TEXT("TAA"):P->AAMethod==1?TEXT("FXAA"):TEXT("Выключено");},[P](int32 D){TArray<int32>A={0,1,2,4};P->AAMethod=A[(A.IndexOfByKey(P->AAMethod)+D+4)%4];});
        Toggle(TEXT("Размытие в движении"),&P->MotionBlur);Toggle(TEXT("Свечение ярких объектов"),&P->Bloom);
    }
    else if(Page==2)
    {
        Slider(TEXT("Общая громкость"),&P->MasterVolume,0,1,.05,TEXT(""));
        Slider(TEXT("Окружение корабля и ветер"),&P->AmbienceVolume,0,1,.05,TEXT(""));
        Slider(TEXT("Звуковые эффекты"),&P->EffectsVolume,0,1,.05,TEXT(""));
        Toggle(TEXT("Без звука в фоне"),&P->MuteUnfocused);
    }
    else if(Page==3)
    {
        Slider(TEXT("Чувствительность мыши"),&P->Sensitivity,.25,3,.05,TEXT("×"));Toggle(TEXT("Инверсия мыши по вертикали"),&P->InvertY);
        for(const FKey& K:UCalmyPreferences::DefaultKeys())
        {
            auto Button=SNew(SButton).TextStyle(&SettingsButtonText()).ContentPadding(FMargin(14,8)).Text_Lambda([this,K]{return Txt(Capturing==K?TEXT("Нажмите клавишу…"):Draft->KeyFor(K).GetDisplayName().ToString());}).OnClicked_Lambda([this,K]{Capturing=K;Notice->SetText(Txt(TEXT("Нажмите новую клавишу или кнопку мыши. Escape — отмена.")));FSlateApplication::Get().SetKeyboardFocus(AsShared());return FReply::Handled();});
            BindingButtons.Add(K.GetFName(),Button);Row(UCalmyPreferences::ActionLabel(K),Button);
        }
    }
    else
    {
        Slider(TEXT("Поле зрения"),&P->FieldOfView,70,110,1,TEXT("°"));Toggle(TEXT("Спокойная камера"),&P->SteadyCamera,TEXT("Уменьшает движение камеры от тела."));
        Slider(TEXT("Интенсивность движения камеры"),&P->CameraMotion,0,1,.05,TEXT(""));
        Slider(TEXT("Размер игрового интерфейса"),&P->HUDScale,.75,1.5,.05,TEXT("×"));Toggle(TEXT("Прицел"),&P->ShowCrosshair);Toggle(TEXT("Показывать FPS и время кадра"),&P->ShowFPS);
        Row(TEXT("Меню и взаимодействие"),SNew(STextBlock).Text(Txt(TEXT("Esc — меню / назад\nEnter — подтверждение\nМышь — выбор пунктов"))),TEXT("Эти клавиши остаются доступными при переназначении управления."));
    }
}
void SCalmySettingsWidget::Apply()
{
    if(VideoDeadline>0)return;Draft->Sanitize();PreviousVideo.Read();Video.Write(true);
    auto* S=GEngine->GetGameUserSettings();S->ApplyNonResolutionSettings();
    // Copy preference fields into the existing CDO used by live input/audio.
    for(TFieldIterator<FProperty> It(UCalmyPreferences::StaticClass());It;++It)if(It->HasAnyPropertyFlags(CPF_Config))It->CopyCompleteValue_InContainer(UCalmyPreferences::Get(),Draft.Get());
    UCalmyPreferences::Get()->Apply(PC.Get());UCalmyPreferences::Get()->Persist();
    if(Video.Resolution!=PreviousVideo.Resolution||Video.Mode!=PreviousVideo.Mode)
    {S->ApplyResolutionSettings(false);VideoDeadline=FPlatformTime::Seconds()+15;}
    else {S->SaveSettings();Notice->SetText(Txt(TEXT("Настройки применены и сохранены.")));}
}
TSharedPtr<SWidget> SCalmySettingsWidget::GetActionWidget(FName Name) const
{
    if(Name==TEXT("Apply"))return ApplyButton;if(Name==TEXT("Keep"))return KeepButton;if(Name==TEXT("Back"))return BackButton;
    const FString N=Name.ToString();if(N.StartsWith(TEXT("Tab"))){int32 I=FCString::Atoi(*N.Mid(3));if(TabButtons.IsValidIndex(I))return TabButtons[I];}
    if(const auto* B=BindingButtons.Find(Name))return *B;return nullptr;
}
void SCalmySettingsWidget::KeepVideo()
{if(VideoDeadline<=0)return;VideoDeadline=0;auto* S=GEngine->GetGameUserSettings();S->ConfirmVideoMode();S->SaveSettings();Notice->SetText(Txt(TEXT("Новый режим экрана сохранён.")));}
void SCalmySettingsWidget::RevertVideo()
{
    if(VideoDeadline<=0)return;VideoDeadline=0;
    auto* S=GEngine->GetGameUserSettings();S->SetScreenResolution(PreviousVideo.Resolution);S->SetFullscreenMode(PreviousVideo.Mode);S->ApplyResolutionSettings(false);S->ConfirmVideoMode();S->SaveSettings();
    Video.Resolution=PreviousVideo.Resolution;Video.Mode=PreviousVideo.Mode;Notice->SetText(Txt(TEXT("Прежний режим экрана восстановлен.")));
}
void SCalmySettingsWidget::Close(){RevertVideo();if(PC.IsValid())PC->CloseSettings();}
void SCalmySettingsWidget::Tick(const FGeometry& G,double T,float Dt)
{SCompoundWidget::Tick(G,T,Dt);UpdateDeadline();}
void SCalmySettingsWidget::UpdateDeadline()
{if(VideoDeadline>0){const double Left=VideoDeadline-FPlatformTime::Seconds();if(Left<=0)RevertVideo();else Notice->SetText(Txt(FString::Printf(TEXT("Изображение видно? Сохраните экран — иначе откат через %.0f с."),FMath::CeilToDouble(Left))));}}
void SCalmySettingsWidget::BindKey(FKey K)
{FString Error;if(Draft->SetBinding(Capturing,K,Error)){Capturing=FKey();Notice->SetText(Txt(TEXT("Назначение изменено. Нажмите «Применить».")));}else Notice->SetText(Txt(Error));}
FReply SCalmySettingsWidget::OnKeyDown(const FGeometry& G,const FKeyEvent& E)
{if(E.GetKey()==EKeys::Escape){if(Capturing.IsValid())Capturing=FKey();else Close();return FReply::Handled();}if(Capturing.IsValid()){BindKey(E.GetKey());return FReply::Handled();}return SCompoundWidget::OnKeyDown(G,E);}
FReply SCalmySettingsWidget::OnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{if(Capturing.IsValid()){BindKey(E.GetEffectingButton());return FReply::Handled();}return SCompoundWidget::OnPreviewMouseButtonDown(G,E);}
