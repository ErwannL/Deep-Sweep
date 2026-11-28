#include "UI/DSUIStyle.h"
#include "Core/DSUserSettings.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Sound/SoundBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

namespace DSUI
{
	const FLinearColor Teal(0.1f, 0.85f, 0.95f);
	const FLinearColor Green(0.12f, 0.72f, 0.38f);
	const FLinearColor Blue(0.16f, 0.42f, 0.95f);
	const FLinearColor Orange(0.98f, 0.55f, 0.1f);
	const FLinearColor Purple(0.55f, 0.32f, 0.92f);
	const FLinearColor Grey(0.28f, 0.31f, 0.36f);
	const FLinearColor Red(0.9f, 0.2f, 0.25f);
	const FLinearColor Ink(0.01f, 0.06f, 0.11f, 0.86f);

	static TArray<TSharedPtr<FSlateBrush>> GBrushes;
	static TMap<uint32, TSharedPtr<FButtonStyle>> GButtons;

	FSlateFontInfo Font(int32 Size, bool bBold) { return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); }

	const FSlateBrush* Rounded(const FLinearColor& Color, float Radius)
	{
		TSharedPtr<FSlateBrush> B = MakeShared<FSlateRoundedBoxBrush>(Color, Radius);
		GBrushes.Add(B);
		return B.Get();
	}

	const FButtonStyle* ButtonStyle(const FLinearColor& Base)
	{
		const uint32 Key = GetTypeHash(Base.ToFColor(true));
		if (TSharedPtr<FButtonStyle>* Found = GButtons.Find(Key)) return Found->Get();
		TSharedPtr<FButtonStyle> S = MakeShared<FButtonStyle>();
		const FLinearColor Hover = Base.Desaturate(-0.2f) * 1.18f;
		S->SetNormal(*Rounded(Base, 14.f));
		S->SetHovered(*Rounded(FLinearColor(FMath::Min(Hover.R, 1.f), FMath::Min(Hover.G, 1.f), FMath::Min(Hover.B, 1.f), 1.f), 14.f));
		S->SetPressed(*Rounded(Base * 0.72f, 14.f));
		S->SetDisabled(*Rounded(FLinearColor(0.2f, 0.2f, 0.22f, 0.8f), 14.f));
		S->SetNormalPadding(FMargin(18.f, 10.f));
		S->SetPressedPadding(FMargin(18.f, 12.f, 18.f, 8.f));
		if (USoundBase* ClickSnd = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Sfx/click_001.click_001"))) { ClickSnd->AddToRoot(); FSlateSound Snd; Snd.SetResourceObject(ClickSnd); S->SetPressedSound(Snd); }
		if (USoundBase* HoverSnd = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Sfx/rollover2.rollover2"))) { HoverSnd->AddToRoot(); FSlateSound Snd; Snd.SetResourceObject(HoverSnd); S->SetHoveredSound(Snd); }
		GButtons.Add(Key, S);
		return S.Get();
	}

	TSharedRef<SWidget> Button(const FText& Label, FOnClicked OnClick, const FLinearColor& Color, float Width, int32 FontSize)
	{
		return SNew(SBox).WidthOverride(Width).HeightOverride(FontSize * 1.6f + 34.f).Padding(FMargin(0, 5))
			[
				SNew(SButton).ButtonStyle(ButtonStyle(Color)).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked(OnClick)
				[ SNew(STextBlock).Text(Label).Font(Font(FontSize)).ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(1, 2)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.45f)) ]
			];
	}

	TSharedRef<SWidget> Card(TSharedRef<SWidget> Content, const FMargin& Padding)
	{
		return SNew(SBorder).BorderImage(Rounded(Ink, 22.f)).Padding(Padding)[ Content ];
	}

	TSharedRef<SWidget> Title(const FText& Text, int32 Size)
	{
		return SNew(STextBlock).Text(Text).Font(Font(Size)).ColorAndOpacity(Teal)
			.ShadowOffset(FVector2D(2, 3)).ShadowColorAndOpacity(FLinearColor(0, 0.05f, 0.1f, 0.8f));
	}

	TSharedRef<SWidget> ControlsTable()
	{
		struct FRow { const TCHAR* Action; const TCHAR* Keys; const TCHAR* Pad; };
		static const FRow Rows[] = {
			{ TEXT("Nager"), TEXT("ZQSD / WASD"), TEXT("Stick gauche") },
			{ TEXT("Regarder"), TEXT("Souris"), TEXT("Stick droit") },
			{ TEXT("Monter / descendre"), TEXT("Espace / Ctrl ou C"), TEXT("Gachettes") },
			{ TEXT("Attraper / utiliser"), TEXT("F ou clic gauche"), TEXT("A") },
			{ TEXT("Lancer / action"), TEXT("G ou clic droit"), TEXT("X ou B") },
			{ TEXT("Menu"), TEXT("Echap"), TEXT("Start / Select") },
		};
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		for (const FRow& R : Rows)
		{
			Box->AddSlot().AutoHeight().Padding(0, 3)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Text(FText::FromString(R.Action)).Font(Font(15)).ColorAndOpacity(FLinearColor(1, 1, 1, 0.9f)) ]
				+ SHorizontalBox::Slot().FillWidth(1.1f)[ SNew(STextBlock).Text(FText::FromString(R.Keys)).Font(Font(15)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.4f)) ]
				+ SHorizontalBox::Slot().FillWidth(0.9f)[ SNew(STextBlock).Text(FText::FromString(R.Pad)).Font(Font(15, false)).ColorAndOpacity(FLinearColor(0.7f, 0.9f, 1.f)) ]
			];
		}
		return Box;
	}
}

// ------------------------------------------------------------------ Options

TSharedRef<SWidget> SDSOptionsPanel::SliderRow(const FText& Label, TFunction<float()> Get, TFunction<void(float)> Set, float Min, float Max)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.9f).VAlign(VAlign_Center)[ SNew(STextBlock).Text(Label).Font(DSUI::Font(16)) ]
		+ SHorizontalBox::Slot().FillWidth(1.2f).VAlign(VAlign_Center)
		[
			SNew(SSlider).MinValue(Min).MaxValue(Max).Value_Lambda([Get]() { return Get(); })
			.OnValueChanged_Lambda([this, Set](float V) { Set(V); })
			.OnMouseCaptureEnd_Lambda([this]() { if (Settings.IsValid()) Settings->Save(); })
			.OnControllerCaptureEnd_Lambda([this]() { if (Settings.IsValid()) Settings->Save(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0, 0, 0)
		[ SNew(STextBlock).Font(DSUI::Font(15)).Text_Lambda([Get]() { return FText::AsNumber(FMath::RoundToInt(Get() * 100.f)); }) ];
}

void SDSOptionsPanel::Construct(const FArguments& InArgs)
{
	Settings = InArgs._Settings;
	TWeakObjectPtr<UDSUserSettings> S = Settings;
	ChildSlot
	[
		SNew(SBox).WidthOverride(520.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
			[ SliderRow(LOCTEXT("Sens", "Sensibilite souris"), [S]() { return S.IsValid() ? S->MouseSensitivity : 1.f; }, [S](float V) { if (S.IsValid()) S->MouseSensitivity = V; }, 0.2f, 3.f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.9f).VAlign(VAlign_Center)[ SNew(STextBlock).Text(LOCTEXT("Invert", "Inverser l'axe vertical")).Font(DSUI::Font(16)) ]
				+ SHorizontalBox::Slot().FillWidth(1.2f)
				[
					SNew(SCheckBox).IsChecked_Lambda([S]() { return S.IsValid() && S->bInvertY ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([S](ECheckBoxState St) { if (S.IsValid()) { S->bInvertY = St == ECheckBoxState::Checked; S->Save(); } })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
			[ SliderRow(LOCTEXT("Music", "Volume musique"), [S]() { return S.IsValid() ? S->MusicVolume : 0.6f; }, [S](float V) { if (S.IsValid()) S->MusicVolume = V; }, 0.f, 1.f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
			[ SliderRow(LOCTEXT("Sfx", "Volume effets"), [S]() { return S.IsValid() ? S->SfxVolume : 0.9f; }, [S](float V) { if (S.IsValid()) S->SfxVolume = V; }, 0.f, 1.f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 6)
			[ SNew(STextBlock).Text(LOCTEXT("Controls", "Commandes")).Font(DSUI::Font(18)).ColorAndOpacity(DSUI::Teal) ]
			+ SVerticalBox::Slot().AutoHeight()[ DSUI::ControlsTable() ]
		]
	];
}

#undef LOCTEXT_NAMESPACE
