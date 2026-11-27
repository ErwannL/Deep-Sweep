#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Upgrades/DSUpgradePad.h"
#include "Audio/DSAudioSubsystem.h"
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

	/** Ameliorations achetees (communes a l'equipe). */
	bool HasUpgrade(EDSUpgrade U) const { return (OwnedUpgrades & (1u << uint8(U))) != 0; }
	void GrantUpgrade(EDSUpgrade U) { if (HasAuthority()) OwnedUpgrades |= (1u << uint8(U)); }

	/** Fin de plongee : crique nettoyee a 100 %. */
	UFUNCTION(BlueprintPure) bool IsMatchOver() const { return bMatchOver; }
	float GetFinalTime() const { return FinalTime; }
	/** Serveur. */
	void EndMatch();

	/** Message court affiche a tous (radio de M. Abysso, alertes). */
	UFUNCTION(NetMulticast, Reliable) void Multicast_Toast(const FText& Message);
	/** Son + secousse d'ecran (pres de Location) chez tous les joueurs. Serveur. */
	UFUNCTION(NetMulticast, Unreliable) void Multicast_Sfx(EDSSfx Sfx, FVector_NetQuantize Location, float Shake = 0.f);
	FText GetToast(float& OutAge) const { OutAge = GetWorld() ? GetWorld()->GetTimeSeconds() - ToastTime : 99.f; return Toast; }

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated) int32 Credits = 0;
	UPROPERTY(Replicated) float Cleanliness = 0.f;
	UPROPERTY(Replicated) float MatchStartTime = 0.f;
	UPROPERTY(Replicated) uint8 OwnedUpgrades = 0;
	UPROPERTY(Replicated) bool bMatchOver = false;
	UPROPERTY(Replicated) float FinalTime = 0.f;
	FText Toast;
	float ToastTime = -99.f;
};
