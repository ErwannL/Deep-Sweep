#include "DSGameMode.h"
#include "DeepSweep.h"

ADSGameMode::ADSGameMode()
{
	bUseSeamlessTravel = true;
}

void ADSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogDeepSweep, Log, TEXT("Joueur connecte : %s (%d joueurs)"), *GetNameSafe(NewPlayer), GetNumPlayers());
}
