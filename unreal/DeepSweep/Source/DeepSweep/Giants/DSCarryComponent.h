#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DSCarryComponent.generated.h"

class ADSDiver;

/**
 * Portage multi-joueurs d'un objet geant (GDD 6.1 "Transport d'objets geants", 6.3) :
 * - porteurs requis plafonnes au nombre de joueurs (jamais impossible) ;
 * - vitesse = f(porteurs) : plus rapide a plusieurs ;
 * - direction = moyenne des intentions ; s'ils tirent en sens oppose, l'objet tourne.
 * Le mouvement est calcule par le serveur (objet "critique", replication frequente).
 */
UCLASS(ClassGroup = (DeepSweep), meta = (BlueprintSpawnableComponent))
class DEEPSWEEP_API UDSCarryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDSCarryComponent();

	/** Porteurs prevus par le design (coffre 2, statue 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry") int32 DesignCarriers = 2;
	/** Vitesse a pleine equipe (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Carry") float FullSpeed = 520.f;
	/** Bonus "ballons" (etape 9). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Carry") bool bBalloons = false;

	UPROPERTY(ReplicatedUsing = OnRep_Carriers, BlueprintReadOnly) TArray<TObjectPtr<ADSDiver>> Carriers;

	/** Porteurs requis = min(design, nb de joueurs), au moins 1. */
	int32 GetRequiredCarriers() const;
	/** 0..1 : facteur de vitesse avec les porteurs actuels. */
	float GetSpeedFactor() const;
	bool HasFreeSlot() const { return Carriers.Num() < DesignCarriers; }

	// Serveur
	bool AddCarrier(ADSDiver* Diver);
	void RemoveCarrier(ADSDiver* Diver);
	void ReleaseAll();
	/** Vitesse voulue (cm/s, plan horizontal) et vitesse de rotation (deg/s). */
	void ComputeMotion(FVector& OutVelocity, float& OutYawRate) const;

	DECLARE_MULTICAST_DELEGATE(FOnCarriersChanged);
	FOnCarriersChanged OnCarriersChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION() void OnRep_Carriers();
};
