#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSPickup.generated.h"

class UDSPickupData;
class UDSBuoyancyComponent;
class UStaticMeshComponent;

/**
 * Objet ACTIF (GDD 14) : corps rigide Chaos simule sur le serveur, mouvement replique
 * a basse frequence et interpole cote client. Redevient DORMANT apres 3 s d'immobilite
 * si aucun plongeur n'est proche.
 */
UCLASS()
class DEEPSWEEP_API ADSPickup : public AActor
{
	GENERATED_BODY()
public:
	ADSPickup();

	/** A appeler avant FinishSpawning (serveur). */
	void InitPickup(UDSPickupData* InType, int32 InId);

	UFUNCTION(BlueprintPure) UDSPickupData* GetType() const { return Type; }
	int32 GetPickupId() const { return PickupId; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Contour lumineux au survol (local uniquement). */
	void SetHighlighted(bool bOn);

	/** Porteur actuel (plongeur), nullptr si libre. Serveur uniquement pour l'ecriture. */
	void SetHolder(AActor* NewHolder);
	AActor* GetHolder() const { return Holder; }
	bool IsHeld() const { return Holder != nullptr; }

	/** Dernier lanceur (statistiques, credits attribues). */
	void SetLastThrower(AActor* A) { LastThrower = A; }
	AActor* GetLastThrower() const { return LastThrower.Get(); }

	/** Empeche l'endormissement (porte, sur machine...). */
	UPROPERTY(BlueprintReadWrite) bool bKeepAwake = false;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDSBuoyancyComponent> Buoyancy;

	UPROPERTY(ReplicatedUsing = OnRep_Type) TObjectPtr<UDSPickupData> Type;
	UPROPERTY(Replicated) int32 PickupId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Holder) TObjectPtr<AActor> Holder;
	TWeakObjectPtr<AActor> LastThrower;

	UFUNCTION() void OnRep_Holder();
	void ApplyHeldState();

	UFUNCTION() void OnRep_Type();
	void ApplyType();

	float StillTime = 0.f;
	bool bHighlighted = false;
};
