#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "DSMainMenu.generated.h"

class UDSSessionSubsystem;
class SVerticalBox;
class SEditableTextBox;

/** Menu principal en Slate pur (aucun asset) : pseudo, Heberger (LAN), Chercher, Rejoindre par IP. */
class SDSMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDSMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDSSessionSubsystem>, Sessions)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Refresh();

private:
	TWeakObjectPtr<UDSSessionSubsystem> Sessions;
	TSharedPtr<SEditableTextBox> NameBox;
	TSharedPtr<SEditableTextBox> IpBox;
	TSharedPtr<SVerticalBox> ResultsBox;
	FText GetStatus() const;
	void CommitName();
	TSharedRef<SWidget> MakeButton(const FText& Label, FOnClicked OnClick, FLinearColor Color);
};

UCLASS()
class DEEPSWEEP_API ADSMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	TSharedPtr<SDSMainMenu> Menu;
	FDelegateHandle ChangedHandle;
};

UCLASS()
class DEEPSWEEP_API ADSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADSMenuGameMode();
};
