#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSFX.generated.h"

class UInstancedStaticMeshComponent;

/** Effets visuels du jeu (particules legeres en instances, locales, jamais repliquees). */
UENUM()
enum class EDSFX : uint8
{
	Bubbles,     // bouffee de bulles qui montent
	BigBubbles,  // gros nuage de bulles (collegue recycle, debouchage)
	Coins,       // gerbe de pieces dorees (vente)
	Confetti,    // confettis multicolores (objet geant livre, achat)
	Shards,      // eclats de verre (vase brise)
	Sparks,      // etincelles (bouchon, coup, tapis bloque)
	Ink,         // nuage d'encre (poulpe)
	Dust,        // nuage de sable (objet qui tombe au fond)
};

/** Une rafale de particules : instances qui partent, tournent, retombent ou montent, puis retrecissent. */
UCLASS(NotPlaceable)
class DEEPSWEEP_API ADSFXBurst : public AActor
{
	GENERATED_BODY()
public:
	ADSFXBurst();
	/** Joue un effet a un endroit (sur la machine locale uniquement). */
	static void Play(const UObject* WorldContext, EDSFX Type, const FVector& Location, float Scale = 1.f);
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> ISM;
	struct FP { FVector Pos, Vel, Spin; FRotator Rot; float Size, Life, Age; };
	TArray<FP> Parts;
	float Gravity = 0.f;
	float Drag = 1.5f;
	void Setup(EDSFX Type, float Scale);
};
