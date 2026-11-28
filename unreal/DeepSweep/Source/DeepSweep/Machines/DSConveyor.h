#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "DSConveyor.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UDSPickupData;
class ADSCounter;
class ADSConveyor;

/** Objet en transit : element LOGIQUE (type + distance sur la spline), jamais un corps physique. */
USTRUCT()
struct FDSBeltItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY() int32 Id = 0;
	UPROPERTY() uint8 TypeIndex = 0;
	UPROPERTY() float Distance = 0.f;
	/** Deja debloque une fois : ne rebloque plus. */
	UPROPERTY() bool bPassedJam = false;
};

USTRUCT()
struct FDSBeltArray : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY() TArray<FDSBeltItem> Items;
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDSBeltItem, FDSBeltArray>(Items, DeltaParms, *this);
	}
};
template<> struct TStructOpsTypeTraits<FDSBeltArray> : public TStructOpsTypeTraitsBase2<FDSBeltArray>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Convoyeur logique (GDD 14) : cuve -> comptoir. Le serveur fait avancer les elements et ne
 * replique que les ajouts / retraits / blocages ; les clients extrapolent (vitesse repliquee).
 * Un objet trop gros (caddie) bloque le tapis a mi-chemin : un plongeur doit le debloquer.
 */
UCLASS()
class DEEPSWEEP_API ADSConveyor : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSConveyor();

	UPROPERTY(EditAnywhere, Category = "Conveyor") TObjectPtr<ADSCounter> Counter;
	/** cm/s : 300, x2 avec "Tapis rapide" (etape 9). */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Conveyor") float Speed = 300.f;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bJammed = false;

	UFUNCTION(BlueprintPure) float GetLength() const;
	int32 NumItems() const { return Belt.Items.Num(); }

	/** Serveur : ajoute des objets au debut du tapis (vidage de la cuve). */
	void AddItems(const TArray<TObjectPtr<UDSPickupData>>& Types);

	virtual bool IsInRange(const AActor* Diver) const override;
	virtual bool CanInteract(const ADSDiver* Diver) const override;
	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual void Interact(ADSDiver* Diver) override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USplineComponent> Spline;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> BeltISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> StripeISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> JamLabel;
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> ItemISMs;

	UPROPERTY(Replicated) FDSBeltArray Belt;
	UPROPERTY(Replicated) int32 JamId = INDEX_NONE;

	FVector GetJamLocation() const;
public:
	FVector GetJamWorldLocation() const { return GetJamLocation(); }
protected:
	FTransform ItemTransform(const FDSBeltItem& It) const;
	void ServerAdvance(float DeltaSeconds);
	void ServerCatchFallenPickups();
	void UpdateVisuals(float DeltaSeconds);
	void BuildBelt();

	int32 NextId = 1;
	float StripePhase = 0.f;
	float CatchTimer = 0.f;
};
