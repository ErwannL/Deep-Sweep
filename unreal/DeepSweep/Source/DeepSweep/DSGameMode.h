#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DSGameMode.generated.h"

/** Mode de jeu Deep Sweep (listen server, 1 a 4 joueurs). */
UCLASS()
class DEEPSWEEP_API ADSGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADSGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
