#include "Gameplay/CalmyJoinWidget.h"
#include "Gameplay/CalmyGameMode.h"
#include "Core/CalmySession.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

void SCalmyJoinWidget::Construct(const FArguments& Args)
{
	PC = Args._Controller;
	const FString Initial = FCalmySession::LoadSavedJoinAddress();
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(.02f,.05f,.06f,.96f))
		.Padding(24)
		[
			SNew(SBox).WidthOverride(520)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("JOIN LAN CREW")))
					.ColorAndOpacity(FLinearColor(.72f,.82f,.78f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Same LAN/VPN only. Steam invites are not wired in this build.")))
					.ColorAndOpacity(FLinearColor(.35f,.55f,.55f))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[
					SAssignNew(AddressBox, SEditableTextBox)
					.Text(FText::FromString(Initial))
					.HintText(FText::FromString(TEXT("192.168.1.10:17777")))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Commit)
					{
						if (Commit == ETextCommit::OnEnter) Submit();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)
				[
					SAssignNew(Notice, STextBlock)
					.ColorAndOpacity(FLinearColor(.85f,.55f,.35f))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
					[
						SNew(SButton).Text(FText::FromString(TEXT("JOIN"))).OnClicked_Lambda([this]()
						{
							Submit();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(FText::FromString(TEXT("CANCEL"))).OnClicked_Lambda([this]()
						{
							Cancel();
							return FReply::Handled();
						})
					]
				]
			]
		]
	];
}

FReply SCalmyJoinWidget::OnKeyDown(const FGeometry&, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape) { Cancel(); return FReply::Handled(); }
	if (KeyEvent.GetKey() == EKeys::Enter) { Submit(); return FReply::Handled(); }
	return FReply::Unhandled();
}

FString SCalmyJoinWidget::GetDraft() const
{
	return AddressBox.IsValid() ? AddressBox->GetText().ToString() : FString();
}

void SCalmyJoinWidget::SetNotice(const FString& Text)
{
	if (Notice.IsValid()) Notice->SetText(FText::FromString(Text));
}

void SCalmyJoinWidget::Submit()
{
	auto* Controller = PC.Get();
	if (!Controller) return;
	FString Error;
	if (!Controller->JoinLanSession(GetDraft(), &Error))
	{
		SetNotice(Error);
		return;
	}
	Controller->CloseJoinDialog();
}

void SCalmyJoinWidget::Cancel()
{
	if (auto* Controller = PC.Get()) Controller->CloseJoinDialog();
}
