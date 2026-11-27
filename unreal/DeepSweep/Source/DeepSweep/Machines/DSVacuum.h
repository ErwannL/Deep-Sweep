#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSVacuum.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UCableComponent;
class ADSVacuumNozzle;
class ADSDiver;
class UDSPickupData;

/**
 * Aspirateur industriel a 2 postes (GDD 6.1 / 15) : la cuve (ce qui est aspire y est stocke
 * de facon "virtualisee" : type + valeur, plus d'acteur), la puissance 1..Max, le bouchon,
 * le vidage par le levier (ou Bob si personne n'y est).
 */
UCLASS()
class DEEPSWEEP_API ADSVacuum : public AActor
{
	GENERATED_BODY()
public:
	ADSVacuum();

	UPROPERTY(EditAnywhere, Category = "Vacuum") TObjectPtr<ADSVacuumNozzle> Nozzle;
	UPROPERTY(EditAnywhere, Category = "Vacuum") float HoseLength = 3000.f;
	UPROPERTY(EditAnywhere, Replicated, Category = "Vacuum") int32 Capacity = 30;
	/** 3 de base, 5 avec "Turbo aspirateur" (etape 9). */
	UPROPERTY(EditAnywhere, Replicated, Category = "Vacuum") int32 MaxPower = 3;

	UPROPERTY(Replicated, BlueprintReadOnly) int32 Power = 2;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 TankCount = 0;
	/** Nombre de tapes restantes pour deboucher (0 = libre). */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Clog = 0;

	bool IsFull() const { return TankCount >= Capacity; }
	float GetSuctionRadius() const { return 300.f + Power * 160.f; }
	FVector GetOutletLocation() const;
	FVector GetRecycleLocation() const;

	// Serveur
	void CyclePower();
	/** Objet aspire : stocke dans la cuve. */
	void StoreItem(UDSPickupData* Type);
	/** Vide la cuve (levier ou Bob). Etape 7 : vers le convoyeur. */
	void DumpTank(AActor* By);
	/** Contenu vide : types dans l'ordre (le convoyeur s'en sert, etape 7). */
	TArray<TObjectPtr<UDSPickupData>> TakeTankContents();

	/** Joueur humain au levier ? (sinon Bob vide la cuve). */
	bool IsLeverManned() const;
	UPROPERTY(EditAnywhere, Category = "Vacuum") TObjectPtr<AActor> LeverActor;
	UPROPERTY(EditAnywhere, Category = "Vacuum") TObjectPtr<AActor> ConsoleActor;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TankMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TankRing;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCableComponent> Hose;

	/** Contenu (serveur uniquement : objets virtualises). */
	UPROPERTY(Transient) TArray<TObjectPtr<UDSPickupData>> TankItems;

	float BobTimer = 0.f;
	int32 ShownTank = -1;
	void UpdateLabel();
};
