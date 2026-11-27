#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DSPlayerState.generated.h"

/** Statistiques individuelles (ecran de fin drole, GDD 6.4). */
UCLASS()
class DEEPSWEEP_API ADSPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	/** Credits rapportes par ce joueur. */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 StatSold = 0;
	/** Collegues aspires (etape 6). */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 StatSucked = 0;
	/** Vases casses (etape 6). */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 StatBroken = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
