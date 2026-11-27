#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"

class UDSUserSettings;

/** Style commun des menus (Slate pur, aucun asset) : couleurs saturees, boutons arrondis, cartes. */
namespace DSUI
{
	FSlateFontInfo Font(int32 Size, bool bBold = true);
	const FSlateBrush* Rounded(const FLinearColor& Color, float Radius = 14.f);
	const FButtonStyle* ButtonStyle(const FLinearColor& Base);
	TSharedRef<SWidget> Button(const FText& Label, FOnClicked OnClick, const FLinearColor& Color, float Width = 420.f, int32 FontSize = 22);
	TSharedRef<SWidget> Card(TSharedRef<SWidget> Content, const FMargin& Padding = FMargin(36.f, 28.f));
	TSharedRef<SWidget> Title(const FText& Text, int32 Size);
	TSharedRef<SWidget> ControlsTable();

	extern const FLinearColor Teal, Green, Blue, Orange, Purple, Grey, Red, Ink;
}

/** Panneau d'options partage (menu principal et pause) : sensibilite, inversion, volumes. */
class SDSOptionsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDSOptionsPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDSUserSettings>, Settings)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
private:
	TWeakObjectPtr<UDSUserSettings> Settings;
	TSharedRef<SWidget> SliderRow(const FText& Label, TFunction<float()> Get, TFunction<void(float)> Set, float Min, float Max);
};
