#include "UI/DSMainMenu.h"
#include "Net/DSSessionSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

static FSlateFontInfo DSFont(int32 Size) { return FCoreStyle::GetDefaultFontStyle("Bold", Size); }

TSharedRef<SWidget> SDSMainMenu::MakeButton(const FText& Label, FOnClicked OnClick, FLinearColor Color)
{
	return SNew(SBox).WidthOverride(420.f).HeightOverride(64.f).Padding(FMargin(0, 6))
		[
			SNew(SButton).ButtonColorAndOpacity(Color).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked(OnClick)
			[ SNew(STextBlock).Text(Label).Font(DSFont(22)).ColorAndOpacity(FLinearColor::White) ]
		];
}

void SDSMainMenu::Construct(const FArguments& InArgs)
{
	Sessions = InArgs._Sessions;
	const FString Name = Sessions.IsValid() ? Sessions->PlayerName : TEXT("Plongeur");

	ChildSlot
	[
		SNew(SBorder).BorderBackgroundColor(FLinearColor(0.01f, 0.13f, 0.23f, 0.85f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 4)
			[ SNew(STextBlock).Text(LOCTEXT("Title", "DEEP SWEEP")).Font(DSFont(64)).ColorAndOpacity(FLinearColor(0.4f, 0.95f, 1.f)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
			[ SNew(STextBlock).Text(LOCTEXT("Sub", "Nettoyez la crique a 100 %. 1 a 4 plongeurs en ligne.")).Font(DSFont(16)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.8f)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[ SNew(STextBlock).Text(LOCTEXT("Pseudo", "Pseudo")).Font(DSFont(18)) ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(300.f) [ SAssignNew(NameBox, SEditableTextBox).Text(FText::FromString(Name)).Font(DSFont(18))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type) { CommitName(); }) ] ]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[ MakeButton(LOCTEXT("Host", "Heberger une partie (LAN)"), FOnClicked::CreateLambda([this]() { CommitName(); if (Sessions.IsValid()) Sessions->HostLAN(); return FReply::Handled(); }), FLinearColor(0.1f, 0.7f, 0.35f)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[ MakeButton(LOCTEXT("Find", "Chercher une partie (LAN)"), FOnClicked::CreateLambda([this]() { CommitName(); if (Sessions.IsValid()) Sessions->FindLAN(); return FReply::Handled(); }), FLinearColor(0.15f, 0.4f, 0.95f)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
			[ SAssignNew(ResultsBox, SVerticalBox) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
				[ SNew(SBox).WidthOverride(240.f) [ SAssignNew(IpBox, SEditableTextBox).HintText(LOCTEXT("IpHint", "IP de l'hote (127.0.0.1)")).Font(DSFont(16)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SButton).ButtonColorAndOpacity(FLinearColor(0.5f, 0.3f, 0.9f)).OnClicked_Lambda([this]() { CommitName(); if (Sessions.IsValid()) Sessions->JoinIP(IpBox->GetText().ToString()); return FReply::Handled(); })
					[ SNew(STextBlock).Text(LOCTEXT("JoinIp", "Rejoindre par IP")).Font(DSFont(16)) ] ]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 16, 0, 0)
			[ SNew(STextBlock).Text(this, &SDSMainMenu::GetStatus).Font(DSFont(16)).ColorAndOpacity(FLinearColor(1.f, 0.9f, 0.5f)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 16, 0, 0)
			[ MakeButton(LOCTEXT("Quit", "Quitter"), FOnClicked::CreateLambda([]() { if (GEngine && GEngine->GameViewport) UKismetSystemLibrary::QuitGame(GEngine->GameViewport->GetWorld(), nullptr, EQuitPreference::Quit, false); return FReply::Handled(); }), FLinearColor(0.35f, 0.35f, 0.4f)) ]
		]
	];
	Refresh();
}

void SDSMainMenu::CommitName()
{
	if (Sessions.IsValid() && NameBox.IsValid())
	{
		const FString N = NameBox->GetText().ToString().TrimStartAndEnd();
		if (!N.IsEmpty()) Sessions->PlayerName = N;
	}
}

FText SDSMainMenu::GetStatus() const
{
	return Sessions.IsValid() ? FText::FromString(Sessions->Status) : FText::GetEmpty();
}

void SDSMainMenu::Refresh()
{
	if (!ResultsBox.IsValid() || !Sessions.IsValid()) return;
	ResultsBox->ClearChildren();
	const TArray<FDSFoundSession>& F = Sessions->GetFound();
	for (int32 i = 0; i < F.Num(); ++i)
	{
		const FText Label = FText::Format(LOCTEXT("Row", "Rejoindre {0}  ({1}/{2} places, {3} ms)"),
			FText::FromString(F[i].HostName), F[i].MaxPlayers - F[i].OpenSlots, F[i].MaxPlayers, F[i].PingMs);
		ResultsBox->AddSlot().AutoHeight()
		[ MakeButton(Label, FOnClicked::CreateLambda([this, i]() { CommitName(); if (Sessions.IsValid()) Sessions->JoinFound(i); return FReply::Handled(); }), FLinearColor(0.9f, 0.55f, 0.1f)) ];
	}
}

// ------------------------------------------------------------------

void ADSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || !GEngine || !GEngine->GameViewport) return;
	UDSSessionSubsystem* S = GetGameInstance()->GetSubsystem<UDSSessionSubsystem>();
	Menu = SNew(SDSMainMenu).Sessions(S);
	GEngine->GameViewport->AddViewportWidgetContent(Menu.ToSharedRef(), 10);
	if (S) ChangedHandle = S->OnChanged.AddLambda([this]() { if (Menu.IsValid()) Menu->Refresh(); });
	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu);
	SetInputMode(Mode);
}

void ADSMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UDSSessionSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDSSessionSubsystem>() : nullptr) S->OnChanged.Remove(ChangedHandle);
	if (Menu.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
	Menu.Reset();
	Super::EndPlay(Reason);
}

ADSMenuGameMode::ADSMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ADSMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

#undef LOCTEXT_NAMESPACE
