#include "Giants/DSGiantPickup.h"
#include "Giants/DSCarryComponent.h"
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Pickups/DSPickupManager.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "World/DSCounter.h"
#include "DeepSweep.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

static UStaticMesh* GCube = nullptr;
static UStaticMesh* GCyl = nullptr;
static UStaticMesh* GSphere = nullptr;

ADSGiantPickup::ADSGiantPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(true);
	NetUpdateFrequency = 30.f; // objet "critique" (GDD 14)
	bAlwaysRelevant = true;
	InteractRadius = 420.f;
	Priority = 4;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	GCube = Cube.Object; GCyl = Cyl.Object; GSphere = Sph.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // les porteurs collent a l'objet
	Detail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Detail"));
	Detail->SetupAttachment(Root);
	Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(50.f);
	Label->SetTextRenderColor(FColor(255, 235, 140));
	Carry = CreateDefaultSubobject<UDSCarryComponent>(TEXT("Carry"));
	ApplyShape();
}

void ADSGiantPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShape();
}

void ADSGiantPickup::ApplyShape()
{
	if (Shape == EDSGiantShape::Statue)
	{
		Body->SetStaticMesh(GCyl);
		Body->SetRelativeScale3D(FVector(2.2f, 2.2f, 4.5f));
		Body->SetRelativeLocation(FVector::ZeroVector);
		Detail->SetStaticMesh(GSphere);
		Detail->SetRelativeScale3D(FVector(1.7f));
		Detail->SetRelativeLocation(FVector(0, 0, 300.f));
		HalfHeight = 225.f;
		Carry->DesignCarriers = 3;
	}
	else
	{
		Body->SetStaticMesh(GCube);
		Body->SetRelativeScale3D(FVector(3.f, 2.f, 1.8f));
		Body->SetRelativeLocation(FVector::ZeroVector);
		Detail->SetStaticMesh(GCube);
		Detail->SetRelativeScale3D(FVector(3.08f, 0.35f, 1.86f)); // bande doree
		Detail->SetRelativeLocation(FVector::ZeroVector);
		HalfHeight = 90.f;
		Carry->DesignCarriers = 2;
	}
	Label->SetRelativeLocation(FVector(0, 0, HalfHeight * 2.f + 140.f));
}

float ADSGiantPickup::GetFootprint() const { return Shape == EDSGiantShape::Statue ? 110.f : 150.f; }

void ADSGiantPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyShape();
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col, float E)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), E);
		}
	};
	Tint(Body, Color, 0.15f);
	Tint(Detail, Shape == EDSGiantShape::Statue ? Color : FLinearColor(1.f, 0.78f, 0.15f), 0.4f);
	Carry->OnCarriersChanged.AddUObject(this, &ADSGiantPickup::LayoutCarriers);
}

bool ADSGiantPickup::CanInteract(const ADSDiver* Diver) const
{
	if (!Diver || bDelivered) return false;
	if (Carry->Carriers.Contains(Diver)) return true; // lacher
	return Carry->HasFreeSlot() && Diver->GetGrab()->NumHeld() == 0 && !Diver->GetHeldNozzle() && !Diver->GetCarryingGiant();
}

FText ADSGiantPickup::GetPrompt(const ADSDiver* Diver) const
{
	if (Carry->Carriers.Contains(Diver)) return FText::Format(LOCTEXT("GiantLeave", "Attraper : lacher le {0}"), DisplayName);
	return FText::Format(LOCTEXT("GiantTake", "Attraper : porter le {0} ({1}/{2})"), DisplayName, Carry->Carriers.Num(), Carry->GetRequiredCarriers());
}

void ADSGiantPickup::Interact(ADSDiver* Diver)
{
	if (!HasAuthority() || !Diver) return;
	if (Carry->Carriers.Contains(Diver)) { Carry->RemoveCarrier(Diver); Diver->SetCarryingGiant(nullptr); return; }
	if (Carry->AddCarrier(Diver)) Diver->SetCarryingGiant(this);
}

void ADSGiantPickup::LayoutCarriers()
{
	// Detache ceux qui ne portent plus
	for (const TWeakObjectPtr<ADSDiver>& W : Attached)
		if (ADSDiver* D = W.Get())
			if (!Carry->Carriers.Contains(D)) D->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Attached.Reset();
	// Les porteurs se repartissent autour (comme le prototype), face a l'objet
	const int32 C = Carry->Carriers.Num();
	for (int32 k = 0; k < C; ++k)
	{
		ADSDiver* D = Carry->Carriers[k];
		if (!D) continue;
		const float A = (float(k) / C) * UE_TWO_PI + PI * 0.5f;
		const float R = GetFootprint() + 95.f;
		D->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		D->SetActorRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 30.f));
		D->SetActorRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 180.f, 0.f));
		Attached.Add(D);
	}
}

void ADSGiantPickup::ServerMove(float DeltaSeconds)
{
	FVector Vel;
	float YawRate;
	Carry->ComputeMotion(Vel, YawRate);
	FVector L = GetActorLocation() + Vel * DeltaSeconds;
	L.X = FMath::Clamp(L.X, -3800.f, 3800.f);
	L.Y = FMath::Clamp(L.Y, -3800.f, 3800.f);
	// Pose au fond ; souleve de 40 cm quand il est porte.
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(GiantFloor), false, this);
	float FloorZ = 0.f;
	if (GetWorld()->LineTraceSingleByChannel(Hit, L + FVector(0, 0, 500.f), L - FVector(0, 0, 3000.f), ECC_WorldStatic, Q)) FloorZ = Hit.Location.Z;
	const float TargetZ = FloorZ + HalfHeight + (Carry->Carriers.Num() ? 40.f : 0.f);
	L.Z = FMath::FInterpTo(GetActorLocation().Z, TargetZ, DeltaSeconds, 4.f);
	SetActorLocationAndRotation(L, GetActorRotation() + FRotator(0.f, YawRate * DeltaSeconds, 0.f));
}

void ADSGiantPickup::Deliver()
{
	bDelivered = true;
	TArray<ADSDiver*> Team;
	for (ADSDiver* D : Carry->Carriers) if (D) Team.Add(D);
	Carry->ReleaseAll();
	for (ADSDiver* D : Team) D->SetCarryingGiant(nullptr);

	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It)
	{
		// Credits partages ; la stat "rapporte" est repartie entre les porteurs.
		It->SellValue(Value, nullptr, It->GetHopperLocation() + FVector(0, 0, 250.f));
		for (ADSDiver* D : Team)
			if (ADSPlayerState* PS = D->GetPlayerState<ADSPlayerState>()) PS->StatSold += Value / FMath::Max(1, Team.Num());
		break;
	}
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
		if (ADSPickupManager* M = Sub->GetManager()) M->NotifyCollected(1);
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
	{
		GS->Multicast_Toast(FText::Format(LOCTEXT("Delivered", "{0} livre ! +{1} credits"), DisplayName, Value));
		GS->Multicast_Sfx(EDSSfx::Jingle, GetActorLocation(), 1.f); // camera qui tremble a l arrivee d un objet geant
	}
	UE_LOG(LogDeepSweep, Log, TEXT("Objet geant livre : %s (%d)"), *DisplayName.ToString(), Value);
	SetLifeSpan(0.2f);
}

void ADSGiantPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDelivered) return;
	Label->SetText(FText::Format(LOCTEXT("GiantLabel", "{0}  {1}/{2} porteurs  -  {3} credits"), DisplayName, Carry->Carriers.Num(), Carry->GetRequiredCarriers(), Value));
	if (!HasAuthority()) return;
	ServerMove(DeltaSeconds);
	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It)
		if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 560.f) { Deliver(); break; }
}

#undef LOCTEXT_NAMESPACE
