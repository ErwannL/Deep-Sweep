#include "Machines/DSBobRobot.h"
#include "Machines/DSVacuum.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSBobRobot::ADSBobRobot()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // purement visuel, deduit de l'etat replique de l'aspirateur
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	auto Part = [&](const TCHAR* N, UStaticMesh* M, FVector L, FVector S, USceneComponent* P)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(N);
		C->SetupAttachment(P);
		C->SetStaticMesh(M);
		C->SetRelativeLocation(L);
		C->SetRelativeScale3D(S);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};
	Body = Part(TEXT("Body"), Cube.Object, FVector(0, 0, 70.f), FVector(0.9f, 0.9f, 1.1f), Root);
	HeadMesh = Part(TEXT("Head"), Cube.Object, FVector(0, 0, 160.f), FVector(0.75f, 0.85f, 0.6f), Root);
	Eye = Part(TEXT("Eye"), Sph.Object, FVector(38.f, 0, 162.f), FVector(0.22f, 0.5f, 0.22f), Root);
	Antenna = Part(TEXT("Antenna"), Cyl.Object, FVector(0, 0, 215.f), FVector(0.05f, 0.05f, 0.5f), Root);
	Arm = Part(TEXT("Arm"), Cyl.Object, FVector(0, 55.f, 100.f), FVector(0.12f, 0.12f, 0.9f), Root);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(38.f);
	Label->SetRelativeLocation(FVector(0, 0, 270.f));
	Label->SetTextRenderColor(FColor(140, 230, 255));
}

void ADSBobRobot::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col, float E)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), E);
		}
	};
	Tint(Body, FLinearColor(0.75f, 0.8f, 0.85f), 0.1f);
	Tint(HeadMesh, FLinearColor(0.85f, 0.88f, 0.9f), 0.1f);
	Tint(Eye, FLinearColor(0.2f, 0.9f, 1.f), 2.f);
	Tint(Antenna, FLinearColor(1.f, 0.3f, 0.2f), 1.f);
	Tint(Arm, FLinearColor(1.f, 0.75f, 0.1f), 0.2f);
}

void ADSBobRobot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Vacuum) return;
	const float T = GetWorld()->GetTimeSeconds();
	const bool bHuman = Vacuum->IsLeverManned();

	// Detecte le vidage (la cuve repasse a 0) : Bob tire le levier
	if (LastTank > 0 && Vacuum->TankCount == 0 && !bHuman) PullT = 1.f;
	LastTank = Vacuum->TankCount;
	PullT = FMath::Max(0.f, PullT - DeltaSeconds * 1.2f);

	// Au levier si personne, sinon s'ecarte
	const FVector Target = bHuman ? Home + IdleOffset : Home;
	FVector L = FMath::VInterpTo(GetActorLocation(), Target, DeltaSeconds, 2.5f);
	L.Z = Target.Z + FMath::Abs(FMath::Sin(T * 3.f)) * 8.f; // petits sauts
	SetActorLocation(L);

	Antenna->SetRelativeRotation(FRotator(FMath::Sin(T * 4.f) * 12.f, 0.f, 0.f));
	const float Wave = bHuman ? FMath::Sin(T * 8.f) * 50.f - 120.f : -FMath::Sin(PullT * PI) * 90.f;
	Arm->SetRelativeRotation(FRotator(0.f, 0.f, Wave));
	Eye->SetRelativeScale3D(FVector(0.22f, 0.5f, FMath::Fmod(T, 3.f) < 0.12f ? 0.03f : 0.22f)); // clignement

	Label->SetText(bHuman ? LOCTEXT("BobRest", "Bob : coucou !")
		: Vacuum->TankCount > 0 ? FText::Format(LOCTEXT("BobWait", "Bob surveille la cuve ({0})"), Vacuum->TankCount)
		: LOCTEXT("BobIdle", "Bob (poste du levier)"));
}

#undef LOCTEXT_NAMESPACE
