#include "UI/DSMainMenu.h"
#include "UI/DSUIStyle.h"
#include "Net/DSSessionSubsystem.h"
#include "Core/DSUserSettings.h"
#include "Camera/CameraActor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

static TAutoConsoleVariable<FString> CVarMenuAuto(TEXT("ds.MenuAuto"), TEXT(""),
	TEXT("Test : action automatique du menu apres 3 s (solo, host, join:IP, options)."));

enum EMenuPage { PageHome = 0, PageJoin = 1, PageOptions = 2 };

static TSharedRef<SEditableTextBox> MakeField(TSharedPtr<SEditableTextBox>& Out, const FText& Text, const FText& Hint)
{
	return SAssignNew(Out, SEditableTextBox).Text(Text).HintText(Hint).Font(DSUI::Font(18))
		.BackgroundColor(FLinearColor(0.06f, 0.16f, 0.26f)).ForegroundColor(FLinearColor::White)
		.Padding(FMargin(12.f, 8.f));
}

void SDSMainMenu::Construct(const FArguments& InArgs)
{
	Sessions = InArgs._Sessions;
	Settings = InArgs._Settings;

	ChildSlot
	[
		SNew(SOverlay)
		// Voile degrade a gauche pour la lisibilite (le decor 3D reste visible a droite)
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[ SNew(SBox).WidthOverride(640.f)[ SNew(SBorder).BorderImage(DSUI::Rounded(FLinearColor(0.f, 0.03f, 0.07f, 0.35f), 0.f)) ] ]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(90.f, 40.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ DSUI::Title(LOCTEXT("Title", "DEEP SWEEP"), 78) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 26)
			[ SNew(STextBlock).Text(LOCTEXT("Tag", "Le fond de l'ocean est un enorme tas. Videz-le. Ensemble.")).Font(DSUI::Font(17, false)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.85f)) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				DSUI::Card(
					SAssignNew(Pages, SWidgetSwitcher)
					+ SWidgetSwitcher::Slot()[ BuildHome() ]
					+ SWidgetSwitcher::Slot()[ BuildJoin() ]
					+ SWidgetSwitcher::Slot()[ BuildOptions() ]
				)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4, 16, 0, 0)
			[ SNew(SBox).WidthOverride(560.f)[ SNew(STextBlock).Text(this, &SDSMainMenu::GetStatus).ColorAndOpacity(this, &SDSMainMenu::GetStatusColor).Font(DSUI::Font(16)).AutoWrapText(true) ] ]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(30.f)
		[ SNew(STextBlock).Text(LOCTEXT("Ver", "v0.2  -  1 a 4 plongeurs  -  Hamachi : rejoins avec l'IP 25.x.x.x de l'hote")).Font(DSUI::Font(13, false)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.6f)) ]
	];
	Refresh();
}

TSharedRef<SWidget> SDSMainMenu::BuildHome()
{
	const FString Name = Sessions.IsValid() ? Sessions->GetPlayerName() : TEXT("Plongeur");
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)[ SNew(STextBlock).Text(LOCTEXT("Pseudo", "Pseudo")).Font(DSUI::Font(18)) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[ MakeField(NameBox, FText::FromString(Name), LOCTEXT("PseudoHint", "Ton pseudo")) ]
		]
		+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Solo", "Jouer en solo"), FOnClicked::CreateLambda([this]() { CommitName(); if (Sessions.IsValid()) Sessions->PlaySolo(); return FReply::Handled(); }), DSUI::Green, 440.f, 24) ]
		+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Host", "Heberger une partie"), FOnClicked::CreateLambda([this]() { CommitName(); if (Sessions.IsValid()) Sessions->HostLAN(); return FReply::Handled(); }), FLinearColor(0.07f, 0.66f, 0.78f), 440.f, 24) ]
		+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Join", "Rejoindre une partie"), FOnClicked::CreateLambda([this]() { CommitName(); ShowPage(PageJoin); return FReply::Handled(); }), DSUI::Blue, 440.f, 24) ]
		+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Options", "Options et commandes"), FOnClicked::CreateLambda([this]() { CommitName(); ShowPage(PageOptions); return FReply::Handled(); }), DSUI::Purple, 440.f, 20) ]
		+ SVerticalBox::Slot().AutoHeight()[ DSUI::Button(LOCTEXT("Quit", "Quitter"), FOnClicked::CreateLambda([]() { if (GEngine && GEngine->GameViewport) UKismetSystemLibrary::QuitGame(GEngine->GameViewport->GetWorld(), nullptr, EQuitPreference::Quit, false); return FReply::Handled(); }), DSUI::Grey, 440.f, 20) ];
}

TSharedRef<SWidget> SDSMainMenu::BuildJoin()
{
	const FString Last = Settings.IsValid() ? Settings->LastJoinAddress : FString();
	return SNew(SBox).WidthOverride(520.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ SNew(STextBlock).Text(LOCTEXT("ByIp", "Rejoindre par IP (Hamachi : IP 25.x.x.x de l'hote)")).Font(DSUI::Font(17)).ColorAndOpacity(DSUI::Teal) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 0, 10, 0)[ MakeField(IpBox, FText::FromString(Last), LOCTEXT("IpHint", "ex. 25.123.45.67")) ]
			+ SHorizontalBox::Slot().AutoWidth()[ DSUI::Button(LOCTEXT("Go", "Rejoindre"), FOnClicked::CreateLambda([this]() { if (Sessions.IsValid()) Sessions->JoinIP(IpBox->GetText().ToString()); return FReply::Handled(); }), DSUI::Green, 170.f, 18) ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Text(LOCTEXT("Lan", "Parties sur le reseau local")).Font(DSUI::Font(17)).ColorAndOpacity(DSUI::Teal) ]
			+ SHorizontalBox::Slot().AutoWidth()[ DSUI::Button(LOCTEXT("Refresh", "Actualiser"), FOnClicked::CreateLambda([this]() { if (Sessions.IsValid()) Sessions->FindLAN(); return FReply::Handled(); }), DSUI::Blue, 170.f, 16) ]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[ SNew(SBox).HeightOverride(170.f)[ SNew(SScrollBox) + SScrollBox::Slot()[ SAssignNew(ResultsBox, SVerticalBox) ] ] ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 10, 0, 0)
		[ DSUI::Button(LOCTEXT("Back", "Retour"), FOnClicked::CreateLambda([this]() { ShowPage(PageHome); return FReply::Handled(); }), DSUI::Grey, 200.f, 18) ]
	];
}

TSharedRef<SWidget> SDSMainMenu::BuildOptions()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SDSOptionsPanel).Settings(Settings) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 14, 0, 0)
		[ DSUI::Button(LOCTEXT("Back2", "Retour"), FOnClicked::CreateLambda([this]() { ShowPage(PageHome); return FReply::Handled(); }), DSUI::Grey, 200.f, 18) ];
}

void SDSMainMenu::ShowPage(int32 Page)
{
	if (!Pages.IsValid()) return;
	Pages->SetActiveWidgetIndex(Page);
	if (Page == PageJoin && Sessions.IsValid()) Sessions->FindLAN(); // cherche automatiquement en arrivant
}

void SDSMainMenu::CommitName()
{
	if (Sessions.IsValid() && NameBox.IsValid()) Sessions->SetPlayerName(NameBox->GetText().ToString());
}

FText SDSMainMenu::GetStatus() const { return Sessions.IsValid() ? FText::FromString(Sessions->Status) : FText::GetEmpty(); }

FSlateColor SDSMainMenu::GetStatusColor() const
{
	return Sessions.IsValid() && Sessions->bStatusIsError ? FLinearColor(1.f, 0.45f, 0.45f) : FLinearColor(1.f, 0.9f, 0.5f);
}

void SDSMainMenu::Refresh()
{
	if (!ResultsBox.IsValid() || !Sessions.IsValid()) return;
	ResultsBox->ClearChildren();
	const TArray<FDSFoundSession>& F = Sessions->GetFound();
	if (F.Num() == 0)
	{
		ResultsBox->AddSlot().AutoHeight().Padding(4, 8)
		[ SNew(STextBlock).Font(DSUI::Font(15, false)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.6f))
			.Text(Sessions->IsSearching() ? LOCTEXT("Searching", "Recherche en cours...") : LOCTEXT("None", "Aucune partie pour l'instant.")) ];
		return;
	}
	for (int32 i = 0; i < F.Num(); ++i)
	{
		const FText Label = FText::Format(LOCTEXT("Row", "{0}   {1}/{2} joueurs   {3} ms"),
			FText::FromString(F[i].HostName), F[i].MaxPlayers - F[i].OpenSlots, F[i].MaxPlayers, F[i].PingMs);
		ResultsBox->AddSlot().AutoHeight()
		[ DSUI::Button(Label, FOnClicked::CreateLambda([this, i]() { if (Sessions.IsValid()) Sessions->JoinFound(i); return FReply::Handled(); }), DSUI::Orange, 520.f, 17) ];
	}
}

FReply SDSMainMenu::OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key)
{
	const FKey K = Key.GetKey();
	if ((K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) && Pages.IsValid() && Pages->GetActiveWidgetIndex() != PageHome)
	{
		ShowPage(PageHome);
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(Geo, Key);
}

void SDSMainMenu::AutoAction(const FString& Action)
{
	if (!Sessions.IsValid()) return;
	if (Action == TEXT("solo")) Sessions->PlaySolo();
	else if (Action == TEXT("host")) Sessions->HostLAN();
	else if (Action == TEXT("options")) ShowPage(PageOptions);
	else if (Action == TEXT("joinpage")) ShowPage(PageJoin);
	else if (Action.StartsWith(TEXT("join:"))) Sessions->JoinIP(Action.Mid(5));
}

// ------------------------------------------------------------------

ADSMenuPlayerController::ADSMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || !GEngine || !GEngine->GameViewport) return;

	// Decor : camera qui tourne lentement, crique en version "propre" et coloree
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { Cam = *It; SetViewTarget(*It); break; }
	if (UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materials/MPC_DS.MPC_DS")))
		UKismetMaterialLibrary::SetScalarParameterValue(this, MPC, TEXT("Clean"), 1.f);

	UDSSessionSubsystem* S = GetGameInstance()->GetSubsystem<UDSSessionSubsystem>();
	UDSUserSettings* Set = GetGameInstance()->GetSubsystem<UDSUserSettings>();
	Menu = SNew(SDSMainMenu).Sessions(S).Settings(Set);
	GEngine->GameViewport->AddViewportWidgetContent(Menu.ToSharedRef(), 10);
	if (S) ChangedHandle = S->OnChanged.AddLambda([this]() { if (Menu.IsValid()) Menu->Refresh(); });
	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

void ADSMenuPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ACameraActor* C = Cam.Get())
	{
		Orbit += DeltaSeconds * 0.06f;
		const FVector Center(0.f, 0.f, 180.f);
		const FVector P = Center + FVector(FMath::Cos(Orbit) * 1150.f, FMath::Sin(Orbit) * 1150.f, 420.f + FMath::Sin(Orbit * 2.f) * 100.f);
		C->SetActorLocationAndRotation(P, (Center - P).Rotation());
	}
	const FString Auto = CVarMenuAuto.GetValueOnGameThread();
	if (!bAutoDone && !Auto.IsEmpty() && GetWorld()->GetTimeSeconds() > 3.f && Menu.IsValid())
	{
		bAutoDone = true;
		if (Auto == TEXT("shot")) ConsoleCommand(TEXT("shot showui")); // capture interne (fenetre du jeu uniquement)
		else Menu->AutoAction(Auto);
	}
}

void ADSMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UDSSessionSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDSSessionSubsystem>() : nullptr) S->OnChanged.Remove(ChangedHandle);
	if (Menu.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
	Menu.Reset();
	// Rend la saisie au jeu pour la carte suivante (c'etait la cause du "on ne peut plus bouger")
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	Super::EndPlay(Reason);
}

ADSMenuGameMode::ADSMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ADSMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

#undef LOCTEXT_NAMESPACE
