#pragma once
#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DSFlowParticlesComponent.generated.h"

/**
 * Particules cosmetiques tres legeres (bulles, filets de courant) : instances d'un mesh
 * qui derivent dans une boite locale et rebouclent. Aucune replication, aucun cout reseau.
 */
UCLASS(ClassGroup = (DeepSweep), meta = (BlueprintSpawnableComponent))
class DEEPSWEEP_API UDSFlowParticlesComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
public:
	UDSFlowParticlesComponent();

	/** Demi-taille de la boite (espace local du composant). */
	UPROPERTY(EditAnywhere, Category = "Flow") FVector Extent = FVector(100.f, 100.f, 500.f);
	/** Vitesse de derive (cm/s, espace local). */
	UPROPERTY(EditAnywhere, Category = "Flow") FVector Velocity = FVector(0.f, 0.f, 250.f);
	UPROPERTY(EditAnywhere, Category = "Flow") int32 Count = 60;
	UPROPERTY(EditAnywhere, Category = "Flow") FVector2D ScaleRange = FVector2D(0.05f, 0.15f);
	/** Etirement dans le sens du flux (filets de courant). */
	UPROPERTY(EditAnywhere, Category = "Flow") float Stretch = 1.f;
	UPROPERTY(EditAnywhere, Category = "Flow") float Wobble = 15.f;
	/** Reserre la boite vers son axe a la source (colonne de bulles). */
	UPROPERTY(EditAnywhere, Category = "Flow") bool bColumn = false;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FParticle { FVector Pos; float Scale; float Phase; float SpeedMul; };
	TArray<FParticle> Particles;
	TArray<FTransform> Scratch;
	FVector RandomPoint(bool bAtSource) const;
};
