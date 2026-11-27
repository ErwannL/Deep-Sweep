#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DSHUD.generated.h"

/** HUD de plongee dessine au Canvas (aucun asset) : credits, proprete, chrono, aide contextuelle. */
UCLASS()
class DEEPSWEEP_API ADSHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

private:
	float ShownCredits = 0.f;
	void Pill(float X, float Y, float W, float H, const FLinearColor& Col);
	void Label(const FString& S, float X, float Y, const FLinearColor& Col, float Scale = 1.f, bool bCenter = false);
};
