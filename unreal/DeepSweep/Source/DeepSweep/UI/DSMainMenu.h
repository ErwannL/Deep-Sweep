#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "DSMainMenu.generated.h"

class UDSSessionSubsystem;
class UDSUserSettings;
class SVerticalBox;
class SEditableTextBox;
class SWidgetSwitcher;
class ACameraActor;

/** Menu principal (Slate pur) : Accueil / Rejoindre / Options, sur fond de decor sous-marin 3D. */
class SDSMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDSMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDSSessionSubsystem>, Sessions)
		SLATE_ARGUMENT(TWeakObjectPtr<UDSUserSettings>, Settings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Refresh();
	void ShowPage(int32 Page);
	/** Debug / tests : declenche une action comme un clic. */
	void AutoAction(const FString& Action);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key) override;

private:
	TWeakObjectPtr<UDSSessionSubsystem> Sessions;
	TWeakObjectPtr<UDSUserSettings> Settings;
	TSharedPtr<SEditableTextBox> NameBox;
	TSharedPtr<SEditableTextBox> IpBox;
	TSharedPtr<SVerticalBox> ResultsBox;
	TSharedPtr<SWidgetSwitcher> Pages;
	void CommitName();
	FText GetStatus() const;
	FSlateColor GetStatusColor() const;
	TSharedRef<SWidget> BuildHome();
	TSharedRef<SWidget> BuildJoin();
	TSharedRef<SWidget> BuildOptions();
};

UCLASS()
class DEEPSWEEP_API ADSMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ADSMenuPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
private:
	TSharedPtr<SDSMainMenu> Menu;
	FDelegateHandle ChangedHandle;
	TWeakObjectPtr<ACameraActor> Cam;
	float Orbit = 0.f;
	bool bAutoDone = false;
};

UCLASS()
class DEEPSWEEP_API ADSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADSMenuGameMode();
};
