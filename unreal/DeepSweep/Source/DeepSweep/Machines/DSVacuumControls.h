#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSVacuumControls.generated.h"

class ADSVacuum;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Console de puissance (poste "operateur") : "attraper" = puissance +1 (1..Max). */
UCLASS()
class DEEPSWEEP_API ADSVacuumConsole : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSVacuumConsole();
	UPROPERTY(EditAnywhere, Category = "Vacuum") TObjectPtr<ADSVacuum> Vacuum;

	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual void Interact(ADSDiver* Diver) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gauge;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	int32 ShownPower = -1;
};

/** Levier de vidage de la cuve (poste "videur"). Sans humain, Bob vide la cuve tout seul. */
UCLASS()
class DEEPSWEEP_API ADSVacuumLever : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSVacuumLever();
	UPROPERTY(EditAnywhere, Category = "Vacuum") TObjectPtr<ADSVacuum> Vacuum;

	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual void Interact(ADSDiver* Diver) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Stick;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UFUNCTION(NetMulticast, Unreliable) void Multicast_Pull();
	float PullAnim = 0.f;
};
