#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DSDiver.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class ADSPickup;
class UDSGrabComponent;
class ADSVacuumNozzle;
class ADSGiantPickup;
struct FInputActionValue;

/** Plongeur : nage arcade 6 axes (mode Flying du CharacterMovement, predit et replique nativement). */
UCLASS()
class DEEPSWEEP_API ADSDiver : public ACharacter
{
	GENERATED_BODY()
public:
	ADSDiver(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Index de couleur (0..3), attribue par le GameMode. */
	UPROPERTY(ReplicatedUsing = OnRep_ColorIndex, BlueprintReadOnly, Category = "Diver")
	uint8 ColorIndex = 0;
	void SetColorIndex(uint8 NewIndex);

	static FLinearColor GetPlayerColor(int32 Index);

	UPROPERTY(EditAnywhere, Category = "Swim") float SwimSpeed = 900.f;
	UPROPERTY(EditAnywhere, Category = "Swim") float VerticalSpeedScale = 0.8f;
	UDSGrabComponent* GetGrab() const { return Grab; }
	/** Ralentissement quand on porte (applique cote serveur et client pour la prediction). */
	void SetCarrySpeedFactor(float Factor);

	/** Sonne (aspire par un collegue...) : plus de controle pendant Duration. Serveur. */
	void Stun(float Duration);
	bool IsStunned() const;

	/** Buse tenue (mise a jour par la buse, sur toutes les machines). */
	void SetHeldNozzle(ADSVacuumNozzle* N);

	/** Objet geant porte (serveur ecrit, replique). */
	void SetCarryingGiant(ADSGiantPickup* G);
	ADSGiantPickup* GetCarryingGiant() const;
	/** Direction voulue par ce porteur (plan horizontal, norme <= 1). */
	FVector GetCarryIntent() const { return CarryIntent; }
	ADSVacuumNozzle* GetHeldNozzle() const;

	/** Objet vise (local) : contour lumineux, cible de la pince. */
	ADSPickup* GetHoverTarget() const { return HoverTarget.Get(); }

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	/** Racine visuelle inclinee selon la vitesse (cosmetique, locale). */
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HelmetMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VisorMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EyeL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EyeR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TankMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ClawMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDSGrabComponent> Grab;

	// Enhanced Input cree a l'execution : pas d'asset binaire a maintenir.
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Vertical;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Grab;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Action;

	void BuildInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Vertical(const FInputActionValue& Value);
	void GrabPressed();
	UFUNCTION(Server, Reliable) void Server_Interact(class ADSInteractable* Target);
	UFUNCTION(Server, Reliable) void Server_NozzleAction();
	UFUNCTION(Server, Reliable) void Server_RequestRestart();
	UPROPERTY(Replicated) float StunUntil = -1.f;
	UPROPERTY(ReplicatedUsing = OnRep_CarryingGiant) TObjectPtr<ADSGiantPickup> CarryingGiant;
	UFUNCTION() void OnRep_CarryingGiant();
	UFUNCTION(Server, Unreliable) void Server_SetCarryIntent(FVector_NetQuantize100 Intent);
	void ActionPressed();

	UFUNCTION() void OnRep_ColorIndex();
	void ApplyColor();

private:
	TWeakObjectPtr<ADSPickup> HoverTarget;
	void UpdateHover();
	void RunDebugScenario();
	void RunSellScenario();
	void RunVacScenario();
	void RunBeltScenario();
	void RunGiantScenario();
	void RunShopScenario();
	void RunOctoScenario();
	FVector CarryIntent = FVector::ZeroVector;
	FVector PendingIntent = FVector::ZeroVector;
	bool bIntentThisFrame = false;
	float IntentSendTimer = 0.f;
	FVector LastSentIntent = FVector::ZeroVector;
	TWeakObjectPtr<ADSVacuumNozzle> HeldNozzle;
	float CarrySpeedFactor = 1.f;
	float BobTime = 0.f;
	FRotator VisualTilt = FRotator::ZeroRotator;
};
