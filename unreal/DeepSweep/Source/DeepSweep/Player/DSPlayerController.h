#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SCompoundWidget.h"
#include "DSPlayerController.generated.h"

class SWidgetSwitcher;
class ADSPlayerController;

/** Menu pause (la partie continue pour les autres) : reprendre, options, quitter la partie, quitter le jeu. */
class SDSPauseMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDSPauseMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ADSPlayerController>, Owner)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key) override;
private:
	TWeakObjectPtr<ADSPlayerController> Owner;
	TSharedPtr<SWidgetSwitcher> Switcher;
};

/**
 * Controleur du joueur en partie : remet la saisie au jeu en arrivant (le menu la laissait
 * captee par l'interface), gere le menu pause et la sortie propre.
 */
UCLASS()
class DEEPSWEEP_API ADSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	void TogglePauseMenu();
	void ClosePauseMenu();
	bool IsPauseMenuOpen() const { return PauseMenu.IsValid(); }
	void LeaveGame();
	void QuitGame();

private:
	TSharedPtr<SDSPauseMenu> PauseMenu;
	void ApplyGameInput();
};
