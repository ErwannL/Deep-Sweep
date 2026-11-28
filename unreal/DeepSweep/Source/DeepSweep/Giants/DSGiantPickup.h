#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSGiantPickup.generated.h"

class UDSCarryComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class ADSDiver;

UENUM(BlueprintType)
enum class EDSGiantShape : uint8 { Chest, Statue };

/** Objet geant cooperatif (coffre, statue...) : se porte a plusieurs jusqu'au comptoir. */
UCLASS()
class DEEPSWEEP_API ADSGiantPickup : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSGiantPickup();

	UPROPERTY(EditAnywhere, Category = "Giant") FText DisplayName;
	UPROPERTY(EditAnywhere, Category = "Giant") EDSGiantShape Shape = EDSGiantShape::Chest;
	UPROPERTY(EditAnywhere, Category = "Giant") int32 Value = 300;
	UPROPERTY(EditAnywhere, Category = "Giant") FLinearColor Color = FLinearColor(0.55f, 0.32f, 0.12f);

	UDSCarryComponent* GetCarry() const { return Carry; }

	virtual bool CanInteract(const ADSDiver* Diver) const override;
	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual FText GetStationLabel() const override;
	virtual void Interact(ADSDiver* Diver) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Detail;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDSCarryComponent> Carry;

	void ApplyShape();
	void LayoutCarriers();
	void ServerMove(float DeltaSeconds);
	void Deliver();
	float GetFootprint() const;

	/** Porteurs attaches localement (pour detacher ceux qui partent). */
	TArray<TWeakObjectPtr<ADSDiver>> Attached;
	float HalfHeight = 100.f;
	bool bDelivered = false;
};
