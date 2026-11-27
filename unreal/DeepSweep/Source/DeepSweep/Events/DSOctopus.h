#pragma once
#include "CoreMinimal.h"
#include "World/DSInteractable.h"
#include "DSOctopus.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UDSPickupData;

UENUM()
enum class EDSOctoState : uint8 { Idle, Hunt, Flee };

/**
 * Evenement "Le poulpe voleur" (GDD 12) : arrive par un bord, vole jusqu'a 3 objets precieux,
 * fuit. L'attraper ("attraper" a portee) lui fait tout lacher (+20 credits). S'il s'echappe,
 * le butin n'est pas perdu : il est cache pres du bord (anti-frustration).
 */
UCLASS()
class DEEPSWEEP_API ADSOctopus : public ADSInteractable
{
	GENERATED_BODY()
public:
	ADSOctopus();

	UPROPERTY(EditAnywhere, Category = "Octopus") float ArenaRadius = 3900.f;
	UPROPERTY(EditAnywhere, Category = "Octopus") float HuntSpeed = 750.f;
	UPROPERTY(EditAnywhere, Category = "Octopus") float FleeSpeed = 650.f;
	UPROPERTY(EditAnywhere, Category = "Octopus") int32 MaxLoot = 3;
	UPROPERTY(EditAnywhere, Category = "Octopus") int32 CatchReward = 20;

	UPROPERTY(ReplicatedUsing = OnRep_State) EDSOctoState State = EDSOctoState::Idle;
	/** Types du butin (pour l'afficher autour du poulpe chez tous les joueurs). */
	UPROPERTY(Replicated) TArray<TObjectPtr<UDSPickupData>> Loot;

	/** Serveur : declenche l'evenement maintenant (debug / directeur d'evenements). */
	void TriggerNow();

	virtual bool IsInRange(const AActor* Diver) const override;
	virtual bool CanInteract(const ADSDiver* Diver) const override;
	virtual FText GetPrompt(const ADSDiver* Diver) const override;
	virtual void Interact(ADSDiver* Diver) override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Legs;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> LootMeshes;

	UFUNCTION() void OnRep_State();
	UFUNCTION(NetMulticast, Unreliable) void Multicast_Burst();
	void ApplyVisibility();
	void ServerTick(float DeltaSeconds);
	void Reset();
	void DropLoot(bool bHideAtEdge);
	int32 NumPlayers() const;

	float IdleTimer = 60.f;
	FVector FleeTarget = FVector::ZeroVector;
	bool bHasFleeTarget = false;
};
