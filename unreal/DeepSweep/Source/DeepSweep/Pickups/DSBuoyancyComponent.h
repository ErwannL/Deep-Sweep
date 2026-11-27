#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DSBuoyancyComponent.generated.h"

/**
 * Flottabilite stylisee (GDD 5.3 / 14) : force verticale + trainee de l'eau.
 * Le serveur fait autorite ; les clients l'appliquent aussi pour predire entre deux mises a jour.
 */
UCLASS(ClassGroup = (DeepSweep), meta = (BlueprintSpawnableComponent))
class DEEPSWEEP_API UDSBuoyancyComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDSBuoyancyComponent();

	/** -1 remonte, 0 neutre, +1 coule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float Sink = 0.5f;
	/** Bonus de flottaison (ballons, etape 9) retranche a Sink. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float Lift = 0.f;
	/** Acceleration pour |Sink| = 1 (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float BaseAcceleration = 450.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
