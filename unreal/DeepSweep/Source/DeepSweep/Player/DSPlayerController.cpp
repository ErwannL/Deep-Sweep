#include "Player/DSPlayerController.h"
#include "Net/DSSessionSubsystem.h"
#include "Core/DSUserSettings.h"
#include "UI/DSUIStyle.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

void SDSPauseMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	TWeakObjectPtr<ADSPlayerController> O = Owner;
	UDSUserSettings* Settings = O.IsValid() ? UDSUserSettings::Get(O.Get()) : nullptr;
	const bool bHost = O.IsValid() && O->HasAuthority() && O->GetNetMode() == NM_ListenServer;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SBorder).BorderImage(DSUI::Rounded(FLinearColor(0.f, 0.02f, 0.05f, 0.55f), 0.f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			DSUI::Card(
				SAssignNew(Switcher, SWidgetSwitcher)
				// 0 : principal
				+ SWidgetSwitcher::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ DSUI::Title(LOCTEXT("Pause", "PAUSE"), 46) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 18)
					[ SNew(STextBlock).Text(LOCTEXT("PauseSub", "La partie continue pour les autres plongeurs")).Font(DSUI::Font(14, false)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.75f)) ]
					+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Resume", "Reprendre"), FOnClicked::CreateLambda([O]() { if (O.IsValid()) O->ClosePauseMenu(); return FReply::Handled(); }), DSUI::Green) ]
					+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Options", "Options et commandes"), FOnClicked::CreateLambda([this]() { Switcher->SetActiveWidgetIndex(1); return FReply::Handled(); }), DSUI::Blue) ]
					+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(bHost ? LOCTEXT("LeaveHost", "Terminer la partie (hote)") : LOCTEXT("Leave", "Quitter la partie"),
						FOnClicked::CreateLambda([O]() { if (O.IsValid()) O->LeaveGame(); return FReply::Handled(); }), DSUI::Orange) ]
					+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("QuitGame", "Quitter le jeu"), FOnClicked::CreateLambda([O]() { if (O.IsValid()) O->QuitGame(); return FReply::Handled(); }), DSUI::Grey) ]
				]
				// 1 : options
				+ SWidgetSwitcher::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)[ DSUI::Title(LOCTEXT("OptTitle", "OPTIONS"), 36) ]
					+ SVerticalBox::Slot().AutoHeight()[ SNew(SDSOptionsPanel).Settings(Settings) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)
					[ DSUI::Button(LOCTEXT("Back", "Retour"), FOnClicked::CreateLambda([this]() { Switcher->SetActiveWidgetIndex(0); return FReply::Handled(); }), DSUI::Grey, 260.f, 18) ]
				]
			)
		]
	];
}

FReply SDSPauseMenu::OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key)
{
	const FKey K = Key.GetKey();
	if (K == EKeys::Escape || K == EKeys::Gamepad_Special_Right || K == EKeys::Gamepad_Special_Left || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (Switcher.IsValid() && Switcher->GetActiveWidgetIndex() != 0) Switcher->SetActiveWidgetIndex(0);
		else if (Owner.IsValid()) Owner->ClosePauseMenu();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(Geo, Key);
}

// ------------------------------------------------------------------

void ADSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController()) ApplyGameInput();
}

void ADSPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	ClosePauseMenu();
	Super::EndPlay(Reason);
}

void ADSPlayerController::ApplyGameInput()
{
	// Le menu principal laissait la saisie en "interface seulement" : on la rend au jeu.
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void ADSPlayerController::TogglePauseMenu()
{
	if (PauseMenu.IsValid()) { ClosePauseMenu(); return; }
	if (!GEngine || !GEngine->GameViewport) return;
	PauseMenu = SNew(SDSPauseMenu).Owner(this);
	GEngine->GameViewport->AddViewportWidgetContent(PauseMenu.ToSharedRef(), 50);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(PauseMenu);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	FlushPressedKeys(); // pas de touche de nage "collee" pendant la pause
}

void ADSPlayerController::ClosePauseMenu()
{
	if (!PauseMenu.IsValid()) return;
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(PauseMenu.ToSharedRef());
	PauseMenu.Reset();
	if (IsLocalController() && !IsActorBeingDestroyed()) ApplyGameInput();
}

void ADSPlayerController::LeaveGame()
{
	ClosePauseMenu();
	if (UDSSessionSubsystem* S = GetGameInstance()->GetSubsystem<UDSSessionSubsystem>()) S->LeaveToMenu();
}

void ADSPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
