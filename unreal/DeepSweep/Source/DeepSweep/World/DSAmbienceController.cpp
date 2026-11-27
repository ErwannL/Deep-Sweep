#include "World/DSAmbienceController.h"
#include "Core/DSGameState.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/DirectionalLight.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> CVarForceClean(TEXT("ds.SetClean"), -1.f,
	TEXT("Debug : force la proprete affichee (0..1), -1 = valeur reelle."));

ADSAmbienceController::ADSAmbienceController()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADSAmbienceController::BeginPlay()
{
	Super::BeginPlay();
	Shown = -1.f;
}

void ADSAmbienceController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	float Target = 0.f;
	if (const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>()) Target = GS->GetCleanliness();
	const float Forced = CVarForceClean.GetValueOnGameThread();
	if (Forced >= 0.f) Target = FMath::Clamp(Forced, 0.f, 1.f);

	const float Next = Shown < 0.f || Forced >= 0.f ? Target : FMath::FInterpConstantTo(Shown, Target, DeltaSeconds, BlendSpeed);
	if (!FMath::IsNearlyEqual(Next, Shown, 1e-4f)) { Shown = Next; Apply(Shown); }
}

void ADSAmbienceController::Apply(float C)
{
	// Courbe douce : on voit deja la difference des les premiers objets.
	const float K = FMath::Pow(C, 0.7f);
	if (Collection) UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("Clean"), K);
	if (Fog)
	{
		UExponentialHeightFogComponent* F = Fog->GetComponent();
		F->SetFogInscatteringColor(FMath::Lerp(DirtyFogColor, CleanFogColor, K));
		F->SetFogDensity(FMath::Lerp(DirtyFogDensity, CleanFogDensity, K));
		F->SetVolumetricFogAlbedo(FMath::Lerp(DirtyAlbedo, CleanAlbedo, K).ToFColor(true));
	}
	if (PostProcess)
	{
		const float S = FMath::Lerp(DirtySaturation, CleanSaturation, K);
		PostProcess->Settings.bOverride_ColorSaturation = true;
		PostProcess->Settings.ColorSaturation = FVector4(S, S, S, 1.f);
	}
	if (Sun) Sun->GetComponent()->SetIntensity(FMath::Lerp(DirtySun, CleanSun, K));
}
