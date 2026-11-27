#include "DSGameMode.h"
#include "DeepSweep.h"
#include "Player/DSDiver.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

ADSGameMode::ADSGameMode()
{
	DefaultPawnClass = ADSDiver::StaticClass();
	bUseSeamlessTravel = true;
}

void ADSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogDeepSweep, Log, TEXT("Joueur connecte : %s (%d joueurs)"), *GetNameSafe(NewPlayer), GetNumPlayers());
}

void ADSGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	if (ADSDiver* Diver = NewPlayer ? Cast<ADSDiver>(NewPlayer->GetPawn()) : nullptr)
	{
		// Couleur = rang d'arrivee dans la partie
		const int32 Index = GameState ? GameState->PlayerArray.IndexOfByKey(NewPlayer->PlayerState) : 0;
		Diver->SetColorIndex(static_cast<uint8>(FMath::Max(0, Index) % 4));
	}
}
