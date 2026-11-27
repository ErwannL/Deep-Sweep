#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DSPickupSubsystem.generated.h"

class ADSPickup;
class ADSPickupManager;
class UMaterialInterface;

/**
 * Point d'acces aux objets de la zone (GDD 14 : UDSPickupSubsystem).
 * Connait le manager (dormants) et la liste des objets actifs (serveur et clients).
 */
UCLASS()
class DEEPSWEEP_API UDSPickupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void SetManager(ADSPickupManager* InManager);
	ADSPickupManager* GetManager() const;

	void RegisterActive(ADSPickup* P);
	void UnregisterActive(ADSPickup* P);
	int32 NumActive() const { return Actives.Num(); }
	const TArray<TWeakObjectPtr<ADSPickup>>& GetActives() const { return Actives; }

	/** Serveur : rendort l'objet si personne n'est proche. */
	bool TrySleep(ADSPickup* P);

	/**
	 * Meilleure cible devant un plongeur : la plus proche dans Range, ponderee par l'alignement
	 * avec Forward (pour la saisie et le survol).
	 */
	ADSPickup* FindBestPickup(const FVector& From, const FVector& Forward, float Range) const;

	/** Materiau couleur unie partage (M_DSColor). */
	static UMaterialInterface* GetColorMaterial();

private:
	TWeakObjectPtr<ADSPickupManager> Manager;
	TArray<TWeakObjectPtr<ADSPickup>> Actives;
};
