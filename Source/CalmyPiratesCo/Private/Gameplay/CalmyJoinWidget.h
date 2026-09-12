#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class ACalmyPlayerController;
class SCalmyJoinWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCalmyJoinWidget) {}
		SLATE_ARGUMENT(ACalmyPlayerController*, Controller)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	void Submit();
	void Cancel();
	FString GetDraft() const;
private:
	TWeakObjectPtr<ACalmyPlayerController> PC;
	TSharedPtr<class SEditableTextBox> AddressBox;
	TSharedPtr<class STextBlock> Notice;
	void SetNotice(const FString& Text);
};
