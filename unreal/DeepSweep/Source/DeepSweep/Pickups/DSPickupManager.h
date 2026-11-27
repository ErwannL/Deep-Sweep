#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "DSPickupManager.generated.h"

class UDSPickupData;
class UInstancedStaticMeshComponent;
class ADSPickupManager;
class ADSPickup;

/** Objet DORMANT : aucune physique, rendu en instance. Seuls id + type + pose sont repliques. */
USTRUCT()
struct FDSDormantItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY() int32 Id = INDEX_NONE;
	UPROPERTY() uint8 TypeIndex = 0;
	UPROPERTY() FVector_NetQuantize10 Location;
	UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;

	void PostReplicatedAdd(const struct FDSDormantArray& Array);
	void PostReplicatedChange(const struct FDSDormantArray& Array);
	void PreReplicatedRemove(const struct FDSDormantArray& Array);
};

USTRUCT()
struct FDSDormantArray : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY() TArray<FDSDormantItem> Items;
	UPROPERTY(NotReplicated) TObjectPtr<ADSPickupManager> Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDSDormantItem, FDSDormantArray>(Items, DeltaParms, *this);
	}
};
template<> struct TStructOpsTypeTraits<FDSDormantArray> : public TStructOpsTypeTraitsBase2<FDSDormantArray>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Registre des objets d'une zone (GDD 14, niveaux de simulation) :
 *  - DORMANT : instances (1 ISM par type), replication par fast array (delta seulement) ;
 *  - ACTIF   : ADSPickup physique pres des plongeurs, plafonne a MaxActive ;
 *  - (PORTE / CRITIQUE : etape 4+).
 */
UCLASS()
class DEEPSWEEP_API ADSPickupManager : public AActor
{
	GENERATED_BODY()
public:
	ADSPickupManager();

	/** Types d'objets (Data Assets) ; l'index sert d'identifiant reseau compact. */
	UPROPERTY(EditAnywhere, Category = "Pickups") TArray<TObjectPtr<UDSPickupData>> Types;
	/** Centres des tas (espace monde). */
	UPROPERTY(EditAnywhere, Category = "Pickups") TArray<FVector> PileCenters;
	UPROPERTY(EditAnywhere, Category = "Pickups") float PileRadius = 700.f;
	UPROPERTY(EditAnywhere, Category = "Pickups") int32 Seed = 42;
	UPROPERTY(EditAnywhere, Category = "Pickups") float SurfaceZ = 1900.f;

	UPROPERTY(EditAnywhere, Category = "Simulation") float ActivationRadius = 650.f;
	UPROPERTY(EditAnywhere, Category = "Simulation") float SleepRadius = 950.f;
	UPROPERTY(EditAnywhere, Category = "Simulation") int32 MaxActive = 200;

	/** Nombre total d'objets de la zone et nombre collectes (pour la proprete, etape 5). */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 TotalItems = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 CollectedItems = 0;

	int32 NumDormant() const { return Dormant.Items.Num(); }
	int32 GetTypeIndex(const UDSPickupData* Type) const { return Types.IndexOfByKey(Type); }

	/** Serveur : reveille un objet dormant (renvoie l'acteur actif). */
	ADSPickup* Activate(int32 DormantIndex);
	/** Serveur : rendort un objet actif si possible. */
	bool TrySleep(ADSPickup* Pickup);

	void MarkVisualsDirty() { bVisualsDirty = true; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated) FDSDormantArray Dormant;
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> TypeISMs;

	void SpawnInitialItems();
	void UpdateActivation();
	void RebuildVisuals();
	void GatherDiverLocations(TArray<FVector>& Out) const;

	bool bVisualsDirty = true;
	float ActivationTimer = 0.f;
};
