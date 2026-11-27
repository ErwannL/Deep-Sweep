#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DSDiver.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
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
	void ActionPressed();

	UFUNCTION() void OnRep_ColorIndex();
	void ApplyColor();

private:
	float BobTime = 0.f;
	FRotator VisualTilt = FRotator::ZeroRotator;
};
