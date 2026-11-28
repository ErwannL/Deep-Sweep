#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSUpgradePad.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** Les 5 ameliorations du MVP (GDD 15). Achetees une fois pour toute l'equipe. */
UENUM(BlueprintType)
enum class EDSUpgrade : uint8
{
	Bag,       // Sac plus grand : porter 3 objets
	Claw,      // Pince telescopique : portee x1,6
	Turbo,     // Turbo aspirateur : puissance max 5
	FastBelt,  // Tapis rapide : convoyeur x2
	Balloons,  // Ballons : objets geants plus legers (x1,6)
	Count UMETA(Hidden)
};

/** Pad d'achat a la base : "attraper" pour acheter avec les credits partages. */
UCLASS()
class DEEPSWEEP_API ADSUpgradePad : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSUpgradePad();

	UPROPERTY(EditAnywhere, Category = "Upgrade") EDSUpgrade Upgrade = EDSUpgrade::Bag;
	UPROPERTY(EditAnywhere, Category = "Upgrade") FText Title;
	UPROPERTY(EditAnywhere, Category = "Upgrade") FText Description;
	UPROPERTY(EditAnywhere, Category = "Upgrade") int32 Cost = 30;

	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual FText GetStationLabel() const override;
	virtual void Interact(ADSDiver* Diver) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Serveur : applique tous les effets des ameliorations possedees au monde (et aux nouveaux joueurs). */
	static void ApplyOwnedUpgrades(UWorld* World);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Disc;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Icon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UFUNCTION(NetMulticast, Unreliable) void Multicast_Bought();
	int32 ShownState = -1;
	float BounceT = 0.f;
};
