#include "FX/DSFX.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static UStaticMesh* GFXSphere = nullptr;
static UStaticMesh* GFXCube = nullptr;
static UMaterialInterface* GFXBubble = nullptr;

ADSFXBurst::ADSFXBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Bub(TEXT("/Game/Materials/M_DSBubble.M_DSBubble"));
	GFXSphere = Sph.Object; GFXCube = Cube.Object; GFXBubble = Bub.Object;
	ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISM"));
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(false);
	ISM->SetMobility(EComponentMobility::Movable);
	RootComponent = ISM;
}

void ADSFXBurst::Play(const UObject* WorldContext, EDSFX Type, const FVector& Location, float Scale)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W || W->GetNetMode() == NM_DedicatedServer) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADSFXBurst* B = W->SpawnActor<ADSFXBurst>(Location, FRotator::ZeroRotator, P)) B->Setup(Type, Scale);
}

void ADSFXBurst::Setup(EDSFX Type, float Scale)
{
	int32 N = 12;
	float Speed = 300.f, Size = 0.1f, Life = 1.2f;
	FLinearColor Col = FLinearColor::White;
	float Emi = 1.5f;
	bool bCube = false, bBubble = false;
	TArray<FLinearColor> Palette;
	switch (Type)
	{
	case EDSFX::Bubbles: N = 14; Speed = 120.f; Size = 0.08f; Life = 1.8f; Gravity = -380.f; bBubble = true; break;
	case EDSFX::BigBubbles: N = 40; Speed = 320.f; Size = 0.16f; Life = 2.2f; Gravity = -420.f; bBubble = true; break;
	case EDSFX::Coins: N = 16; Speed = 520.f; Size = 0.14f; Life = 1.3f; Gravity = 900.f; bCube = true; Col = FLinearColor(1.f, 0.78f, 0.1f); Emi = 1.2f; break;
	case EDSFX::Confetti: N = 46; Speed = 760.f; Size = 0.12f; Life = 2.2f; Gravity = 420.f; bCube = true;
		Palette = { FLinearColor(1, 0.2f, 0.4f), FLinearColor(0.2f, 0.9f, 1), FLinearColor(1, 0.85f, 0.1f), FLinearColor(0.5f, 1, 0.3f), FLinearColor(0.75f, 0.4f, 1) }; break;
	case EDSFX::Shards: N = 18; Speed = 600.f; Size = 0.1f; Life = 1.1f; Gravity = 600.f; bCube = true; Col = FLinearColor(1.f, 0.85f, 0.3f); Emi = 2.f; break;
	case EDSFX::Sparks: N = 16; Speed = 700.f; Size = 0.05f; Life = 0.5f; Gravity = 500.f; Col = FLinearColor(1.f, 0.55f, 0.1f); Emi = 6.f; break;
	case EDSFX::Ink: N = 22; Speed = 260.f; Size = 0.9f; Life = 2.4f; Gravity = -40.f; Col = FLinearColor(0.04f, 0.02f, 0.07f); Emi = 0.f; Drag = 2.5f; break;
	case EDSFX::Dust: N = 12; Speed = 200.f; Size = 0.5f; Life = 1.6f; Gravity = -60.f; Col = FLinearColor(0.6f, 0.52f, 0.4f); Emi = 0.f; Drag = 3.f; break;
	}
	ISM->SetStaticMesh(bCube ? GFXCube : GFXSphere);
	if (bBubble && GFXBubble) ISM->SetMaterial(0, GFXBubble);
	else if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Col);
		MID->SetScalarParameterValue(TEXT("Emissive"), Emi);
		MID->SetScalarParameterValue(TEXT("Metallic"), Type == EDSFX::Coins ? 1.f : 0.f);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.3f);
	}
	// Confettis : une instance par couleur de la palette (un ISM ne porte qu'un materiau)
	if (Palette.Num() > 0)
	{
		for (int32 c = 1; c < Palette.Num(); ++c)
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ADSFXBurst* Extra = GetWorld()->SpawnActor<ADSFXBurst>(GetActorLocation(), FRotator::ZeroRotator, P))
			{
				Extra->Setup(EDSFX::Sparks, 0.f); // vide, puis on le reconfigure
				Extra->Parts.Reset();
				Extra->ISM->ClearInstances();
				Extra->ISM->SetStaticMesh(GFXCube);
				Extra->Gravity = Gravity; Extra->Drag = Drag;
				if (UMaterialInstanceDynamic* MID = Extra->ISM->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
				{
					MID->SetVectorParameterValue(TEXT("Color"), Palette[c]);
					MID->SetScalarParameterValue(TEXT("Emissive"), 1.5f);
				}
				for (int32 i = 0; i < N / Palette.Num(); ++i)
				{
					FP Pp{ FVector::ZeroVector, FMath::VRand() * Speed * Scale * FMath::FRandRange(0.5f, 1.f) + FVector(0, 0, Speed * 0.6f), FMath::VRand() * 600.f,
						FRotator(FMath::FRand() * 360.f, FMath::FRand() * 360.f, 0.f), Size * Scale * FMath::FRandRange(0.6f, 1.2f), Life * FMath::FRandRange(0.7f, 1.1f), 0.f };
					Extra->Parts.Add(Pp);
					Extra->ISM->AddInstance(FTransform(Extra->GetActorLocation()), true);
				}
			}
		}
		Col = Palette[0];
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(ISM->GetMaterial(0))) MID->SetVectorParameterValue(TEXT("Color"), Col);
		N /= Palette.Num();
	}
	if (Scale <= 0.f) return;
	for (int32 i = 0; i < N; ++i)
	{
		FVector Dir = FMath::VRand();
		if (Gravity > 0.f) Dir.Z = FMath::Abs(Dir.Z) + 0.4f; // jaillit vers le haut puis retombe
		FP Pp{ FVector::ZeroVector, Dir.GetSafeNormal() * Speed * Scale * FMath::FRandRange(0.4f, 1.f), FMath::VRand() * 500.f,
			FRotator(FMath::FRand() * 360.f, FMath::FRand() * 360.f, 0.f), Size * Scale * FMath::FRandRange(0.6f, 1.3f), Life * FMath::FRandRange(0.7f, 1.1f), 0.f };
		Parts.Add(Pp);
		ISM->AddInstance(FTransform(GetActorLocation()), true);
	}
}

void ADSFXBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bAlive = false;
	TArray<FTransform> Xf;
	Xf.Reserve(Parts.Num());
	for (FP& P : Parts)
	{
		P.Age += DeltaSeconds;
		P.Vel.Z -= Gravity * DeltaSeconds;
		P.Vel *= FMath::Exp(-Drag * DeltaSeconds);
		P.Pos += P.Vel * DeltaSeconds;
		P.Rot += FRotator(P.Spin.X, P.Spin.Y, P.Spin.Z) * DeltaSeconds;
		const float K = FMath::Clamp(P.Age / FMath::Max(0.01f, P.Life), 0.f, 1.f);
		bAlive |= K < 1.f;
		const float S = P.Size * (K < 0.1f ? K * 10.f : 1.f) * (1.f - FMath::Pow(K, 3.f));
		Xf.Add(FTransform(P.Rot, GetActorLocation() + P.Pos, FVector(FMath::Max(S, 0.001f))));
	}
	if (Xf.Num()) ISM->BatchUpdateInstancesTransforms(0, Xf, true, true, true);
	if (!bAlive) Destroy();
}
