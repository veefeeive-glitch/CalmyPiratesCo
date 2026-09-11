#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Gameplay/CalmyPreferences.h"
#include "GameFramework/GameUserSettings.h"

struct FCalmyVideoOptions
{
    FIntPoint Resolution;
    EWindowMode::Type Mode=EWindowMode::Windowed;
    bool VSync=false,AutoScale=false;
    float Limit=60,Scale=100;
    int32 Quality[10]={2,2,2,2,2,2,2,2,2,2};
    void Read();
    void Write(bool ResolutionToo) const;
};
class SCalmySettingsWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCalmySettingsWidget) {} SLATE_ARGUMENT(class ACalmyPlayerController*,Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override {return true;}
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual FReply OnPreviewMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual void Tick(const FGeometry&,double,float) override;
    void Apply();
    void RevertVideo();
    void KeepVideo();
    void UpdateDeadline();
    void Close();
    void SetPage(int32 Index);
    int32 GetPage() const {return Page;}
    bool IsConfirmingVideo() const {return VideoDeadline>0;}
    UCalmyPreferences* GetDraft() const {return Draft.Get();}
    FCalmyVideoOptions& GetVideoDraft(){return Video;}
    TSharedPtr<SWidget> GetActionWidget(FName Name) const;
private:
    TWeakObjectPtr<class ACalmyPlayerController> PC;
    TStrongObjectPtr<UCalmyPreferences> Draft;
    TSharedPtr<class SVerticalBox> PageBody;
    TSharedPtr<class STextBlock> Notice;
    FCalmyVideoOptions Video,PreviousVideo;
    int32 Page=0;
    FKey Capturing;
    double VideoDeadline=0;
    TArray<FIntPoint> Resolutions;
    TSharedPtr<class SButton> ApplyButton,KeepButton,BackButton;
    TArray<TSharedPtr<class SButton>> TabButtons;
    TMap<FName,TSharedPtr<class SButton>> BindingButtons;
    void BuildPage();
    void BindKey(FKey Key);
    void Row(const FString& Label,TSharedRef<SWidget> Control,const FString& Help=FString());
    void Toggle(const FString& Label,bool* Value,const FString& Help=FString());
    void Slider(const FString& Label,float* Value,float Min,float Max,float Step,const FString& Suffix,bool* Automatic=nullptr);
    void Choice(const FString& Label,TFunction<FString()> Value,TFunction<void(int32)> Change,const FString& Help=FString());
};
