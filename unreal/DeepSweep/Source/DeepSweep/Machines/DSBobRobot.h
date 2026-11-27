#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSBobRobot.generated.h"

class ADSVacuum;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Bob le robot (GDD 6.3) : tient les postes vides de l'aspirateur. Il se place au levier quand
 * aucun joueur n'y est (et le tire quand il vide la cuve), sinon il s'ecarte et fait coucou.
 * Purement visuel : la logique de vidage est dans ADSVacuum (serveur).
 */
UCLASS()
class DEEPSWEEP_API ADSBobRobot : public AActor
{
	GENERATED_BODY()
public:
	ADSBobRobot();
	UPROPERTY(EditAnywhere, Category = "Bob") TObjectPtr<ADSVacuum> Vacuum;
	/** Position de repos quand un humain tient le levier. */
	UPROPERTY(EditAnywhere, Category = "Bob") FVector IdleOffset = FVector(-250.f, -250.f, 0.f);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Eye;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Arm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Antenna;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;

	FVector Home = FVector::ZeroVector;
	int32 LastTank = 0;
	float PullT = 0.f;
};
