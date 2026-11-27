#include "DSGameMode.h"
#include "DeepSweep.h"
#include "Player/DSDiver.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "UI/DSHUD.h"
#include "Upgrades/DSUpgradePad.h"
#include "Player/DSPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

ADSGameMode::ADSGameMode()
{
	DefaultPawnClass = ADSDiver::StaticClass();
	GameStateClass = ADSGameState::StaticClass();
	PlayerStateClass = ADSPlayerState::StaticClass();
	HUDClass = ADSHUD::StaticClass();
	PlayerControllerClass = ADSPlayerController::StaticClass();
	bUseSeamlessTravel = true;
}

void ADSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	FString Wanted;
	if (RequestedNames.RemoveAndCopyValue(NewPlayer, Wanted) && !Wanted.IsEmpty())
		ChangeName(NewPlayer, Wanted.Left(16), false);
	if (APlayerState* PS = NewPlayer ? NewPlayer->PlayerState : nullptr)
		if (PS->GetPlayerName().IsEmpty() || PS->GetPlayerName().Contains(FPlatformProcess::ComputerName()))
			ChangeName(NewPlayer, FString::Printf(TEXT("Plongeur%d"), GameState ? GameState->PlayerArray.Num() : 1), false);
	UE_LOG(LogDeepSweep, Log, TEXT("Joueur connecte : %s (%d joueurs)"), *GetNameSafe(NewPlayer), GetNumPlayers());
}

FString ADSGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	const FString Wanted = UGameplayStatics::ParseOption(Options, TEXT("Name"));
	if (!Wanted.IsEmpty() && !Wanted.Contains(FPlatformProcess::ComputerName())) RequestedNames.Add(NewPlayerController, Wanted);
	return Result;
}

void ADSGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	if (ADSDiver* Diver = NewPlayer ? Cast<ADSDiver>(NewPlayer->GetPawn()) : nullptr)
	{
		// Couleur = rang d'arrivee dans la partie
		const int32 Index = GameState ? GameState->PlayerArray.IndexOfByKey(NewPlayer->PlayerState) : 0;
		Diver->SetColorIndex(static_cast<uint8>(FMath::Max(0, Index) % 4));
		ADSUpgradePad::ApplyOwnedUpgrades(GetWorld()); // un joueur qui arrive profite des achats
	}
}
