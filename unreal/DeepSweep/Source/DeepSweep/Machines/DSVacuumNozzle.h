#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSVacuumNozzle.generated.h"

class ADSVacuum;
class ADSDiver;
class UStaticMeshComponent;

/**
 * Buse de l'aspirateur (poste "porte-buse") : on la prend avec "attraper", "action" allume/eteint
 * ou tape sur le bouchon. L'aspiration est calculee sur le serveur ; les objets aspires
 * disparaissent (flux virtualise) et sont stockes dans la cuve.
 */
UCLASS()
class DEEPSWEEP_API ADSVacuumNozzle : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSVacuumNozzle();

	UPROPERTY(Transient) TObjectPtr<ADSVacuum> Vacuum;
	UPROPERTY(ReplicatedUsing = OnRep_Holder, BlueprintReadOnly) TObjectPtr<ADSDiver> Holder;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bOn = false;

	bool IsSucking() const;
	float GetActivationRadius() const;

	virtual bool CanInteract(const ADSDiver* Diver) const override;
	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual FText GetStationLabel() const override;
	virtual void Interact(ADSDiver* Diver) override;
	/** Serveur : "action" du porteur (allumer/eteindre ou taper sur le bouchon). */
	void PrimaryAction(ADSDiver* Diver);
	/** Serveur : lache la buse a l'endroit ou elle est. */
	void Release();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mouth;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Handle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SuckCone;

	UFUNCTION() void OnRep_Holder();
	void ApplyHolder();
	void ServerSuck(float DeltaSeconds);
	void SuckDivers(float DeltaSeconds, float Radius);

	UFUNCTION(NetMulticast, Unreliable) void Multicast_Popup(FVector_NetQuantize Location, const FText& Text, FColor Color);

	TWeakObjectPtr<ADSDiver> LastHolder;
	float SwirlTimer = 0.f;
};
