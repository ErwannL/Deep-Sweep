#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DSGrabComponent.generated.h"

class ADSPickup;

/**
 * Pince du plongeur : attraper, porter (au-dessus de la tete), lacher, lancer.
 * Le client demande, le serveur valide (portee, capacite, objet libre) et fait autorite.
 */
UCLASS(ClassGroup = (DeepSweep), meta = (BlueprintSpawnableComponent))
class DEEPSWEEP_API UDSGrabComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDSGrabComponent();

	/** Nombre d'objets portables (1 ; 3 avec l'amelioration "sac"). */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Grab") int32 Capacity = 1;
	/** Portee (cm depuis le bord de l'objet) ; x1,6 avec la pince telescopique. */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Grab") float Range = 220.f;
	UPROPERTY(EditAnywhere, Category = "Grab") float ThrowSpeed = 1500.f;
	/** Ralentissement par objet porte (prototype : -12 %). */
	UPROPERTY(EditAnywhere, Category = "Grab") float SlowPerItem = 0.12f;

	const TArray<TObjectPtr<ADSPickup>>& GetHeld() const { return Held; }
	int32 NumHeld() const { return Held.Num(); }
	bool IsFull() const { return Held.Num() >= Capacity; }

	// Appels locaux (envoient la requete au serveur)
	void RequestGrab(ADSPickup* Target);
	void RequestDrop();
	void RequestThrow(const FVector& AimDirection);

	/** Serveur : retire un objet porte (vendu, aspire...) sans le relacher. */
	void ServerForget(ADSPickup* Pickup);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Held) TArray<TObjectPtr<ADSPickup>> Held;

	UFUNCTION(Server, Reliable) void Server_Grab(ADSPickup* Target);
	UFUNCTION(Server, Reliable) void Server_Drop();
	UFUNCTION(Server, Reliable) void Server_Throw(FVector_NetQuantizeNormal AimDirection);

	UFUNCTION() void OnRep_Held();
	void OnHeldChanged();
	void LayoutHeld();
	void Release(ADSPickup* P, const FVector& Velocity);
};
