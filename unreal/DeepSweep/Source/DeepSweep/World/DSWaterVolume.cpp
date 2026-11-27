#include "World/DSWaterVolume.h"
#include "World/DSFlowParticlesComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ADSWaterVolume::ADSWaterVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(300.f, 4000.f, 900.f));
	Box->SetCollisionProfileName(TEXT("OverlapAll"));
	Box->SetGenerateOverlapEvents(true);
	Box->SetHiddenInGame(true);
	RootComponent = Box;

	Particles = CreateDefaultSubobject<UDSFlowParticlesComponent>(TEXT("Particles"));
	Particles->SetupAttachment(Box);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Bubble(TEXT("/Game/Materials/M_DSBubble.M_DSBubble"));
	Particles->SetStaticMesh(Sphere.Object);
	if (Bubble.Succeeded()) Particles->SetMaterial(0, Bubble.Object);
}

void ADSWaterVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureParticles();
}

void ADSWaterVolume::BeginPlay()
{
	ConfigureParticles(); // avant le BeginPlay des composants (proprietes posees par script)
	Super::BeginPlay();
}

void ADSWaterVolume::ConfigureParticles()
{
	const FVector E = Box->GetUnscaledBoxExtent();
	Particles->Extent = E;
	// Les particules derivent dans le sens de la force (espace local).
	const FVector LocalDir = GetActorTransform().InverseTransformVectorNoScale(Acceleration.GetSafeNormal());
	if (Kind == EDSWaterVolumeKind::BubbleColumn)
	{
		Particles->bColumn = true;
		Particles->Velocity = FVector(0.f, 0.f, 320.f);
		Particles->Count = 90;
		Particles->ScaleRange = FVector2D(0.06f, 0.22f);
		Particles->Stretch = 1.f;
		Particles->Wobble = 20.f;
	}
	else
	{
		Particles->bColumn = false;
		Particles->Velocity = LocalDir * Acceleration.Size() * 1.4f;
		Particles->Count = FMath::Clamp(int32(E.X * E.Y * E.Z / 2.0e7f), 40, 220);
		Particles->ScaleRange = FVector2D(0.03f, 0.06f);
		Particles->Stretch = 12.f;
		Particles->Wobble = 8.f;
	}
}

FVector ADSWaterVolume::GetAccelerationAt(const FVector& WorldPos) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPos);
	const FVector E = Box->GetUnscaledBoxExtent();
	if (FMath::Abs(Local.X) > E.X || FMath::Abs(Local.Y) > E.Y || FMath::Abs(Local.Z) > E.Z) return FVector::ZeroVector;
	if (Kind == EDSWaterVolumeKind::BubbleColumn)
	{
		// Plus fort au centre de la colonne
		const float R = FVector2D(Local.X, Local.Y).Size() / FMath::Max(1.f, E.X);
		return Acceleration * FMath::Clamp(1.2f - R, 0.f, 1.f);
	}
	return Acceleration;
}

void ADSWaterVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TArray<AActor*> Overlaps;
	Box->GetOverlappingActors(Overlaps);
	for (AActor* A : Overlaps)
	{
		if (ACharacter* C = Cast<ACharacter>(A))
		{
			// Simule la ou le mouvement est calcule : serveur et client proprietaire.
			if (!C->HasAuthority() && !C->IsLocallyControlled()) continue;
			const FVector Acc = GetAccelerationAt(C->GetActorLocation()) * DiverScale;
			if (!Acc.IsNearlyZero()) C->GetCharacterMovement()->AddImpulse(Acc * DeltaSeconds, true);
		}
		else if (HasAuthority())
		{
			TArray<UPrimitiveComponent*> Prims;
			A->GetComponents<UPrimitiveComponent>(Prims);
			for (UPrimitiveComponent* P : Prims)
				if (P->IsSimulatingPhysics())
				{
					const FVector Acc = GetAccelerationAt(P->GetComponentLocation());
					if (!Acc.IsNearlyZero()) P->AddForce(Acc, NAME_None, true);
				}
		}
	}
}
