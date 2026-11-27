#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSAmbienceController.generated.h"

class AExponentialHeightFog;
class APostProcessVolume;
class ADirectionalLight;
class UMaterialParameterCollection;

/**
 * Fait passer la zone de "sale" a "coloree" selon le % de proprete (GDD 13, avant/apres) :
 * parametre global des materiaux (MPC Clean), eau trouble -> claire, saturation.
 * Purement local, lit la proprete repliquee par le GameState.
 */
UCLASS()
class DEEPSWEEP_API ADSAmbienceController : public AActor
{
	GENERATED_BODY()
public:
	ADSAmbienceController();

	UPROPERTY(EditAnywhere, Category = "Refs") TObjectPtr<AExponentialHeightFog> Fog;
	UPROPERTY(EditAnywhere, Category = "Refs") TObjectPtr<APostProcessVolume> PostProcess;
	UPROPERTY(EditAnywhere, Category = "Refs") TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY(EditAnywhere, Category = "Refs") TObjectPtr<UMaterialParameterCollection> Collection;

	UPROPERTY(EditAnywhere, Category = "Dirty") FLinearColor DirtyFogColor = FLinearColor(0.025f, 0.075f, 0.06f);
	UPROPERTY(EditAnywhere, Category = "Dirty") FLinearColor DirtyAlbedo = FLinearColor(0.35f, 0.45f, 0.3f);
	UPROPERTY(EditAnywhere, Category = "Dirty") float DirtyFogDensity = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Dirty") float DirtySaturation = 0.75f;
	UPROPERTY(EditAnywhere, Category = "Dirty") float DirtySun = 5.f;

	UPROPERTY(EditAnywhere, Category = "Clean") FLinearColor CleanFogColor = FLinearColor(0.01f, 0.12f, 0.22f);
	UPROPERTY(EditAnywhere, Category = "Clean") FLinearColor CleanAlbedo = FLinearColor(0.15f, 0.55f, 0.85f);
	UPROPERTY(EditAnywhere, Category = "Clean") float CleanFogDensity = 0.16f;
	UPROPERTY(EditAnywhere, Category = "Clean") float CleanSaturation = 1.25f;
	UPROPERTY(EditAnywhere, Category = "Clean") float CleanSun = 9.f;

	/** Vitesse de transition affichee (par seconde). */
	UPROPERTY(EditAnywhere, Category = "Clean") float BlendSpeed = 0.25f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float Shown = -1.f;
	void Apply(float Clean);
};
