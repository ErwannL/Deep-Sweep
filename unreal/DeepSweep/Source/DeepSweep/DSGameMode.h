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
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

private:
	/** Pseudo demande dans l'URL (?Name=), applique apres la connexion (le sous-systeme en ligne l'ecrase sinon). */
	TMap<TWeakObjectPtr<APlayerController>, FString> RequestedNames;
};
