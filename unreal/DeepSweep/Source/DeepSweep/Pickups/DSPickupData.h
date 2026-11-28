#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DSPickupData.generated.h"

class UStaticMesh;

/** Code couleur universel (GDD 4 : reperer). */
UENUM(BlueprintType)
enum class EDSPickupCategory : uint8
{
	Trash,       // dechet : gris / bleu
	Recyclable,  // vert
	Metal,       // orange
	Precious,    // dore, brillant
};

/** Type d'objet ramassable (canette, bouteille, ferraille, pneu, caddie, vase...). */
UCLASS(BlueprintType)
class DEEPSWEEP_API UDSPickupData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup") FName TypeId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup") EDSPickupCategory Category = EDSPickupCategory::Trash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") FVector MeshScale = FVector(1.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") FLinearColor Color = FLinearColor::Gray;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") float Emissive = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") float Metallic = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") float Roughness = 0.5f;

	/** Valeur de vente (credits). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy") int32 Value = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics") float MassKg = 2.f;
	/** -1 remonte franchement, 0 neutre, +1 coule (GDD 7). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics") float Sink = 0.6f;
	/** Rayon approximatif (cm) : placement, portee de saisie. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics") float Radius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") bool bFragile = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") bool bClogs = false;

	/** Nombre d'exemplaires dans la crique MVP. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn") int32 SpawnCount = 10;
};
