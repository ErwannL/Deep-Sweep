#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DSGameState.generated.h"

/** Etat partage de la partie : porte-monnaie commun (GDD 11), proprete, chrono. */
UCLASS()
class DEEPSWEEP_API ADSGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	ADSGameState();

	UFUNCTION(BlueprintPure) int32 GetCredits() const { return Credits; }
	/** 0..1 : part des objets de la zone collectes. */
	UFUNCTION(BlueprintPure) float GetCleanliness() const { return Cleanliness; }
	/** Secondes depuis le debut de la plongee (synchronise serveur). */
	UFUNCTION(BlueprintPure) float GetElapsedTime() const;

	// Serveur
	void AddCredits(int32 Amount);
	bool SpendCredits(int32 Amount);
	void SetCleanliness(float Value);

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated) int32 Credits = 0;
	UPROPERTY(Replicated) float Cleanliness = 0.f;
	UPROPERTY(Replicated) float MatchStartTime = 0.f;
};
