#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSInteractable.generated.h"

class ADSDiver;

/**
 * Base des elements de la base avec lesquels on interagit avec "attraper" :
 * comptoir, levier, console, pads d'amelioration... L'interaction s'execute sur le serveur.
 */
UCLASS(Abstract)
class DEEPSWEEP_API ADSInteractable : public AActor
{
	GENERATED_BODY()
public:
	ADSInteractable();

	/** Distance d'interaction (cm, depuis le centre de l'acteur, a plat). */
	UPROPERTY(EditAnywhere, Category = "Interact") float InteractRadius = 400.f;
	/** Priorite quand plusieurs choix sont possibles (plus haut = prioritaire). */
	UPROPERTY(EditAnywhere, Category = "Interact") int32 Priority = 0;

	virtual bool CanInteract(const ADSDiver* Diver) const { return true; }
	/** Texte d'aide affiche quand le plongeur est a portee. */
	virtual FText GetPrompt(const ADSDiver* Diver) const { return FText::GetEmpty(); }
	/** Serveur. */
	virtual void Interact(ADSDiver* Diver) {}

	bool IsInRange(const AActor* Diver) const;
	/** Meilleur interactable utilisable a portee (local ou serveur). */
	static ADSInteractable* FindBest(const ADSDiver* Diver);
};
