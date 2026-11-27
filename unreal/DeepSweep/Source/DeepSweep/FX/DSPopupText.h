#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSPopupText.generated.h"

class UTextRenderComponent;

/** Texte flottant local (ex. "+12" credits) qui monte, grossit puis disparait. Jamais replique. */
UCLASS(NotPlaceable)
class DEEPSWEEP_API ADSPopupText : public AActor
{
	GENERATED_BODY()
public:
	ADSPopupText();
	static ADSPopupText* Spawn(const UObject* WorldContext, const FVector& Location, const FText& Text, FColor Color, float Size = 90.f);
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Text;
	float Age = 0.f;
	float Life = 1.3f;
	float BaseSize = 90.f;
};
