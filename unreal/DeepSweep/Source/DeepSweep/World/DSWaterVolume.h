#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSWaterVolume.generated.h"

class UBoxComponent;
class UDSFlowParticlesComponent;

UENUM(BlueprintType)
enum class EDSWaterVolumeKind : uint8
{
	Current,       // courant directionnel (convoyeur naturel)
	BubbleColumn,  // ascenseur a bulles
};

/**
 * Volume d'eau a force (GDD 14 : ADSWaterVolume). Pousse plongeurs et objets physiques.
 * La force est appliquee la ou le mouvement est simule (serveur + client proprietaire),
 * ce qui reste coherent avec la prediction du CharacterMovement.
 */
UCLASS()
class DEEPSWEEP_API ADSWaterVolume : public AActor
{
	GENERATED_BODY()
public:
	ADSWaterVolume();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water") EDSWaterVolumeKind Kind = EDSWaterVolumeKind::Current;
	/** Acceleration appliquee (cm/s^2), espace monde. Reste < vitesse de nage (GDD 7 anti-frustration). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water") FVector Acceleration = FVector(0.f, 350.f, 0.f);
	/** Facteur pour les plongeurs (plus faible que pour les objets). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water") float DiverScale = 0.5f;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Box;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDSFlowParticlesComponent> Particles;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	void ConfigureParticles();
	virtual void Tick(float DeltaSeconds) override;

	/** Acceleration subie en un point (0 hors du volume). */
	FVector GetAccelerationAt(const FVector& WorldPos) const;
};
