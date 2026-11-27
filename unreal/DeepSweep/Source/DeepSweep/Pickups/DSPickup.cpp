#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSBuoyancyComponent.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

static UMaterialInterface* GOutlineMaterial = nullptr;

ADSPickup::ADSPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	SetReplicatingMovement(true);
	NetUpdateFrequency = 10.f;     // GDD 14 : 5-10 Hz
	MinNetUpdateFrequency = 2.f;
	NetCullDistanceSquared = FMath::Square(9000.f);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(false);
	Mesh->SetLinearDamping(1.6f);   // trainee de l'eau
	Mesh->SetAngularDamping(1.2f);
	Mesh->BodyInstance.bUseCCD = false;
	Mesh->SetCanEverAffectNavigation(false);
	RootComponent = Mesh;

	Buoyancy = CreateDefaultSubobject<UDSBuoyancyComponent>(TEXT("Buoyancy"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Outline(TEXT("/Game/Materials/M_DSOutline.M_DSOutline"));
	if (Outline.Succeeded()) GOutlineMaterial = Outline.Object;
}

void ADSPickup::InitPickup(UDSPickupData* InType, int32 InId)
{
	Type = InType;
	PickupId = InId;
	ApplyType();
}

void ADSPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ADSPickup, Type, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ADSPickup, PickupId, COND_InitialOnly);
}

void ADSPickup::OnRep_Type() { ApplyType(); }

void ADSPickup::ApplyType()
{
	if (!Type) return;
	Mesh->SetStaticMesh(Type->Mesh);
	Mesh->SetWorldScale3D(Type->MeshScale);
	Mesh->SetMassOverrideInKg(NAME_None, Type->MassKg, true);
	Buoyancy->Sink = Type->Sink;
	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Type->Color);
		MID->SetScalarParameterValue(TEXT("Emissive"), FMath::Max(Type->Emissive, 0.12f)); // lisible a travers le bleu
		MID->SetScalarParameterValue(TEXT("Roughness"), Type->Category == EDSPickupCategory::Precious ? 0.2f : 0.5f);
	}
}

void ADSPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyType();
	// Cote client, le corps simule aussi localement et la "physics replication" d'Unreal
	// le ramene en douceur vers l'etat serveur (pas de saccades a 10 Hz).
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>()) Sub->RegisterActive(this);
}

void ADSPickup::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>()) Sub->UnregisterActive(this);
	Super::EndPlay(Reason);
}

void ADSPickup::SetHighlighted(bool bOn)
{
	if (bHighlighted == bOn) return;
	bHighlighted = bOn;
	Mesh->SetOverlayMaterial(bOn ? GOutlineMaterial : nullptr);
}

void ADSPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;

	// Retour au repos : immobile 3 s -> dormant (si personne n'est proche).
	if (!bKeepAwake && Mesh->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(12.f))
	{
		StillTime += DeltaSeconds;
		if (StillTime > 3.f)
		{
			if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
				Sub->TrySleep(this);
			StillTime = 0.f;
		}
	}
	else StillTime = 0.f;
}
