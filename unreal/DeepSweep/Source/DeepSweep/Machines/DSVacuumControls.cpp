#include "Machines/DSVacuumControls.h"
#include "Machines/DSVacuum.h"
#include "Player/DSDiver.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

static void DSTint(UStaticMeshComponent* C, const FLinearColor& Col, float Emi = 0.15f)
{
	if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Col);
		MID->SetScalarParameterValue(TEXT("Emissive"), Emi);
	}
}

static UTextRenderComponent* DSMakeLabel(AActor* Owner, USceneComponent* Parent, float Z)
{
	UTextRenderComponent* L = Owner->CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	L->SetupAttachment(Parent);
	L->SetAbsolute(false, false, true);
	L->SetHorizontalAlignment(EHTA_Center);
	L->SetWorldSize(40.f);
	L->SetRelativeLocation(FVector(0, 0, Z));
	return L;
}

// ------------------------------------------------------------------ Console

ADSVacuumConsole::ADSVacuumConsole()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 250.f;
	LabelHeight = 220.f;
	LabelColor = FLinearColor(0.1f, 0.25f, 0.8f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetStaticMesh(Cube.Object);
	Body->SetRelativeScale3D(FVector(1.f, 1.6f, 1.4f));
	Body->SetRelativeLocation(FVector(0, 0, 70.f));
	RootComponent = Body;
	Gauge = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gauge"));
	Gauge->SetupAttachment(Body);
	Gauge->SetStaticMesh(Cube.Object);
	Gauge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label = DSMakeLabel(this, Body, 75.f);
}

void ADSVacuumConsole::BeginPlay()
{
	Super::BeginPlay();
	DSTint(Body, FLinearColor(0.15f, 0.3f, 1.f));
}

FText ADSVacuumConsole::GetPrompt(const ADSDiver*) const
{
	return Vacuum ? FText::Format(LOCTEXT("ConsolePrompt", "Attraper : puissance +1  (P{0}/{1})"), Vacuum->Power, Vacuum->MaxPower) : FText::GetEmpty();
}

void ADSVacuumConsole::Interact(ADSDiver*)
{
	if (Vacuum) Vacuum->CyclePower();
}

void ADSVacuumConsole::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Vacuum || ShownPower == Vacuum->Power) return;
	ShownPower = Vacuum->Power;
	Label->SetText(FText::Format(LOCTEXT("ConsoleLabel", "Puissance {0}/{1}"), Vacuum->Power, Vacuum->MaxPower));
	// Jauge : barre qui monte et rougit
	const float K = float(Vacuum->Power) / 5.f;
	Gauge->SetRelativeScale3D(FVector(1.05f, 0.2f, 0.9f * K));
	Gauge->SetRelativeLocation(FVector(0.f, 0.f, -50.f + 45.f * K));
	DSTint(Gauge, FMath::Lerp(FLinearColor(0.2f, 1.f, 0.3f), FLinearColor(1.f, 0.1f, 0.05f), FMath::Clamp((Vacuum->Power - 1) / 4.f, 0.f, 1.f)), 1.f);
}

// ------------------------------------------------------------------ Levier

ADSVacuumLever::ADSVacuumLever()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 250.f;
	LabelHeight = 420.f;
	LabelColor = FLinearColor(0.75f, 0.6f, 0.05f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetStaticMesh(Cube.Object);
	Base->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.3f));
	Base->SetRelativeLocation(FVector(0, 0, 15.f));
	RootComponent = Base;
	Stick = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stick"));
	Stick->SetupAttachment(Base);
	Stick->SetAbsolute(false, false, true);
	Stick->SetStaticMesh(Cyl.Object);
	Stick->SetRelativeScale3D(FVector(0.15f, 0.15f, 1.6f));
	Stick->SetRelativeLocation(FVector(0, 0, 250.f));
	Stick->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label = DSMakeLabel(this, Base, 650.f);
	Label->SetText(LOCTEXT("LeverLabel", "Levier : vider"));
}

void ADSVacuumLever::BeginPlay()
{
	Super::BeginPlay();
	DSTint(Base, FLinearColor(0.25f, 0.25f, 0.3f));
	DSTint(Stick, FLinearColor(1.f, 0.9f, 0.2f), 0.3f);
}

FText ADSVacuumLever::GetPrompt(const ADSDiver*) const
{
	return Vacuum ? FText::Format(LOCTEXT("LeverPrompt", "Attraper : vider la cuve ({0}/{1})"), Vacuum->TankCount, Vacuum->Capacity) : FText::GetEmpty();
}

void ADSVacuumLever::Interact(ADSDiver* Diver)
{
	if (!Vacuum) return;
	Vacuum->DumpTank(Diver);
	Multicast_Pull();
}

void ADSVacuumLever::Multicast_Pull_Implementation() { PullAnim = 1.f; }

void ADSVacuumLever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PullAnim = FMath::Max(0.f, PullAnim - DeltaSeconds * 1.5f);
	Stick->SetRelativeRotation(FRotator(0.f, 0.f, -35.f * FMath::Sin(PullAnim * PI)));
}

FText ADSVacuumConsole::GetStationLabel() const { return LOCTEXT("StConsole", "CONSOLE : puissance de l'aspirateur"); }
FText ADSVacuumLever::GetStationLabel() const { return LOCTEXT("StLever", "LEVIER : vider la cuve sur le tapis"); }

#undef LOCTEXT_NAMESPACE
