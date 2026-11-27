#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSCounter.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class ADSPickup;

/**
 * "Le Comptoir" (GDD 9) : convertit les objets en credits partages.
 * - Depot a la main : "attraper" a portee en portant des objets.
 * - Tremie : tout objet libre qui y tombe (lance, convoyeur) est vendu.
 */
UCLASS()
class DEEPSWEEP_API ADSCounter : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSCounter();

	virtual bool CanInteract(const ADSDiver* Diver) const override;
	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual void Interact(ADSDiver* Diver) override;
	virtual void BeginPlay() override;

	/** Serveur : vend un objet (credits au joueur Seller s'il y en a un). */
	void Sell(ADSPickup* Pickup, AActor* Seller);
	/** Serveur : credite une valeur directe (objets geants, poulpe...). */
	void SellValue(int32 Value, AActor* Seller, const FVector& PopupLocation);

	/** Point de sortie du convoyeur (etape 7). */
	FVector GetHopperLocation() const;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Rim;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Hopper;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Sign;

	UFUNCTION() void OnHopperOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION(NetMulticast, Unreliable) void Multicast_SaleFX(int32 Value, FVector_NetQuantize Location);
};
