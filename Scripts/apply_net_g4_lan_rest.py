# -*- coding: utf-8 -*-
"""Apply Net G4 LAN session slice to CalmyPiratesCoUE (absolute paths)."""
from pathlib import Path

ROOT = Path(r"E:\Game\CalmyPiratesCoUE")


def w(rel: str, text: str) -> None:
    path = ROOT / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.replace("\r\n", "\n"), encoding="utf-8", newline="\n")
    print(f"WROTE {rel} {path.stat().st_size}")


w(
    "Source/CalmyPiratesCo/CalmyPiratesCo.Build.cs",
    """using UnrealBuildTool;

public class CalmyPiratesCo : ModuleRules
{
	public CalmyPiratesCo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
            "AIModule",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralMeshComponent", "Json", "CoreOnline", "RenderCore", "RHI", "Slate", "SlateCore", "ApplicationCore", "Sockets" });
	}
}
""",
)

w(
    "Source/CalmyPiratesCo/Private/Gameplay/CalmyJoinWidget.h",
    """#pragma once
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
""",
)

w(
    "Source/CalmyPiratesCo/Private/Gameplay/CalmyJoinWidget.cpp",
    """#include "Gameplay/CalmyJoinWidget.h"
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
""",
)

w(
    "Source/CalmyPiratesCo/Public/Testing/CalmySessionProbe.h",
    """#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CalmySessionProbe.generated.h"

UCLASS()
class CALMYPIRATESCO_API UCalmySessionProbe : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CalmySessionProbe, STATGROUP_Tickables); }
private:
	void Finish(bool OK, const FString& Detail);
	FString Role;
	int32 Stage = 0;
	float Age = 0.f;
	int32 ExpectedPeers = 3;
	bool bStarted = false;
};
""",
)

w(
    "Source/CalmyPiratesCo/Private/Testing/CalmySessionProbe.cpp",
    """#include "Testing/CalmySessionProbe.h"
#include "Core/CalmySession.h"
#include "Gameplay/CalmyCharacter.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/OutputDeviceRedirector.h"

bool UCalmySessionProbe::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString ProbeRole;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CalmySessionProbe="), ProbeRole);
#endif
}

void UCalmySessionProbe::Finish(bool Success, const FString& Details)
{
	UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s %s %s"), *Role, Success ? TEXT("PASS") : TEXT("FAIL"), *Details);
	GLog->Flush();
	Stage = 999;
	FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}

void UCalmySessionProbe::Tick(float DeltaSeconds)
{
	if (Stage == 999 || !GetWorld()->HasBegunPlay()) return;
	if (Role.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("CalmySessionProbe="), Role);
		FParse::Value(FCommandLine::Get(), TEXT("CalmySessionPeers="), ExpectedPeers);
		ExpectedPeers = FMath::Clamp(ExpectedPeers, 2, FCalmySession::MaxCrew);
	}
	Age += DeltaSeconds;
	if (Age > 90.f)
	{
		Finish(false, FString::Printf(TEXT("timeout crew=%d expected=%d mode=%d"),
			FCalmySession::CountCrew(GetWorld()), ExpectedPeers, (int32)GetWorld()->GetNetMode()));
		return;
	}

	const bool Host = Role == TEXT("Host");
	const bool Late = Role == TEXT("LateJoin");
	const bool Clientish = Role == TEXT("Client") || Role == TEXT("Client2") || Late;
	if ((Host && GetWorld()->GetNetMode() != NM_ListenServer) ||
		(Clientish && GetWorld()->GetNetMode() != NM_Client)) return;

	int32 Pawns = 0;
	for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It) ++Pawns;
	const int32 Crew = FCalmySession::CountCrew(GetWorld());

	if (!bStarted)
	{
		bStarted = true;
		UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s START crew=%d pawns=%d expected=%d endpoint=%s"),
			*Role, Crew, Pawns, ExpectedPeers, *FCalmySession::DescribeListenEndpoint(GetWorld()));
		GLog->Flush();
	}

	if (Host)
	{
		if (Stage == 0 && Crew >= 2)
		{
			UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST Host READY crew=%d"), Crew);
			GLog->Flush();
			Stage = 1;
		}
		if (Stage == 1 && Crew >= ExpectedPeers && Pawns >= ExpectedPeers)
		{
			Finish(true, FString::Printf(TEXT("four_slot_cap=%d crew=%d pawns=%d listen=1 late_join_ok=1"),
				FCalmySession::MaxCrew, Crew, Pawns));
		}
		return;
	}

	if (Crew >= 1 && Pawns >= 1)
	{
		UE_LOG(LogTemp, Display, TEXT("CALMY_SESSION_TEST %s READY crew=%d pawns=%d"), *Role, Crew, Pawns);
		GLog->Flush();
		const bool Enough = !Late || (Crew >= ExpectedPeers && Pawns >= ExpectedPeers);
		if (Enough || (!Late && Crew >= 2 && Pawns >= 2))
		{
			Finish(true, FString::Printf(TEXT("joined=1 crew=%d pawns=%d expected=%d late=%d"),
				Crew, Pawns, ExpectedPeers, Late ? 1 : 0));
		}
	}
}
""",
)

print("join+probe+buildcs done")
