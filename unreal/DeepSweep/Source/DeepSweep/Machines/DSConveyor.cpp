#include "Machines/DSConveyor.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Core/DSGameState.h"
#include "World/DSCounter.h"
#include "Player/DSDiver.h"
#include "DeepSweep.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSConveyor::ADSConveyor()
{
	PrimaryActorTick.bCanEverTick = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 10.f;
	InteractRadius = 320.f;
	Priority = 6;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;
	Spline->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	BeltISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BeltISM"));
	BeltISM->SetupAttachment(Spline);
	BeltISM->SetStaticMesh(Cube.Object);
	BeltISM->SetCollisionProfileName(TEXT("BlockAll"));
	BeltISM->SetAbsolute(true, true, true);
	BeltISM->SetMobility(EComponentMobility::Movable);

	StripeISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StripeISM"));
	StripeISM->SetupAttachment(Spline);
	StripeISM->SetStaticMesh(Cube.Object);
	StripeISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StripeISM->SetAbsolute(true, true, true);
	StripeISM->SetMobility(EComponentMobility::Movable);

	JamLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("JamLabel"));
	JamLabel->SetupAttachment(Spline);
	JamLabel->SetAbsolute(true, true, true);
	JamLabel->SetHorizontalAlignment(EHTA_Center);
	JamLabel->SetWorldSize(55.f);
	JamLabel->SetTextRenderColor(FColor(255, 80, 40));
	JamLabel->SetText(LOCTEXT("Jammed", "BLOQUE ! (attraper)"));
	JamLabel->SetVisibility(false);
}

void ADSConveyor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSConveyor, Belt);
	DOREPLIFETIME(ADSConveyor, Speed);
	DOREPLIFETIME(ADSConveyor, bJammed);
	DOREPLIFETIME(ADSConveyor, JamId);
}

float ADSConveyor::GetLength() const { return Spline->GetSplineLength(); }

void ADSConveyor::BeginPlay()
{
	Super::BeginPlay();
	BuildBelt();

	// Une ISM par type d'objet (memes Data Assets que le registre de la zone).
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
		if (ADSPickupManager* M = Sub->GetManager())
			for (UDSPickupData* T : M->Types)
			{
				UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
				ISM->SetAbsolute(true, true, true);
				ISM->SetupAttachment(Spline);
				ISM->SetMobility(EComponentMobility::Movable);
				ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ISM->RegisterComponent();
				if (T)
				{
					ISM->SetStaticMesh(T->Mesh);
					if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
					{
						MID->SetVectorParameterValue(TEXT("Color"), T->Color);
						MID->SetScalarParameterValue(TEXT("Emissive"), FMath::Max(T->Emissive, 0.12f));
					}
				}
				ItemISMs.Add(ISM);
			}
}

void ADSConveyor::BuildBelt()
{
	BeltISM->ClearInstances();
	const float L = GetLength();
	const int32 N = FMath::Max(1, FMath::CeilToInt(L / 100.f));
	for (int32 i = 0; i < N; ++i)
	{
		const float D = (i + 0.5f) * L / N;
		const FVector P = Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		const FRotator R = Spline->GetRotationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		BeltISM->AddInstance(FTransform(R, P - FVector(0, 0, 18.f), FVector(L / N / 100.f + 0.02f, 2.f, 0.3f)), true);
	}
	auto Tint = [](UMeshComponent* C, const FLinearColor& Col)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
			MID->SetVectorParameterValue(TEXT("Color"), Col);
	};
	Tint(BeltISM, FLinearColor(0.12f, 0.12f, 0.16f));
	Tint(StripeISM, FLinearColor(0.95f, 0.75f, 0.1f));
	StripeISM->ClearInstances();
	const int32 NS = FMath::Max(1, FMath::FloorToInt(L / 120.f));
	for (int32 i = 0; i < NS; ++i) StripeISM->AddInstance(FTransform::Identity, true);
}

void ADSConveyor::AddItems(const TArray<TObjectPtr<UDSPickupData>>& Types)
{
	if (!HasAuthority()) return;
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	if (!M) return;
	float Offset = 0.f;
	for (UDSPickupData* T : Types)
	{
		FDSBeltItem& It = Belt.Items.AddDefaulted_GetRef();
		It.Id = NextId++;
		It.TypeIndex = static_cast<uint8>(FMath::Max(0, M->GetTypeIndex(T)));
		It.Distance = -Offset; // la file entre progressivement sur le tapis
		Offset += FMath::FRandRange(40.f, 90.f);
		Belt.MarkItemDirty(It);
	}
}

FVector ADSConveyor::GetJamLocation() const
{
	for (const FDSBeltItem& It : Belt.Items)
		if (It.Id == JamId) return Spline->GetLocationAtDistanceAlongSpline(FMath::Clamp(It.Distance, 0.f, GetLength()), ESplineCoordinateSpace::World);
	return Spline->GetLocationAtDistanceAlongSpline(GetLength() * 0.5f, ESplineCoordinateSpace::World);
}

bool ADSConveyor::IsInRange(const AActor* Diver) const
{
	return Diver && bJammed && FVector::Dist(Diver->GetActorLocation(), GetJamLocation()) < InteractRadius;
}

bool ADSConveyor::CanInteract(const ADSDiver* Diver) const { return bJammed; }

FText ADSConveyor::GetPrompt(const ADSDiver* Diver) const
{
	return LOCTEXT("Unjam", "Attraper : debloquer le tapis");
}

void ADSConveyor::Interact(ADSDiver* Diver)
{
	if (!HasAuthority() || !bJammed) return;
	for (FDSBeltItem& It : Belt.Items)
	{
		if (It.Id == JamId) It.bPassedJam = true;
		Belt.MarkItemDirty(It); // resynchronise les positions chez les clients
	}
	bJammed = false;
	JamId = INDEX_NONE;
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>()) { GS->Multicast_Toast(LOCTEXT("Unjammed", "Tapis debloque !")); GS->Multicast_Sfx(EDSSfx::Unclog, GetActorLocation()); }
}

void ADSConveyor::ServerAdvance(float DeltaSeconds)
{
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	const float L = GetLength();
	if (!bJammed)
	{
		for (FDSBeltItem& It : Belt.Items)
		{
			It.Distance += Speed * DeltaSeconds;
			const UDSPickupData* T = M && M->Types.IsValidIndex(It.TypeIndex) ? M->Types[It.TypeIndex].Get() : nullptr;
			if (T && T->bClogs && !It.bPassedJam && It.Distance > L * 0.5f)
			{
				// Objet trop gros : le tapis se bloque (GDD 15).
				bJammed = true;
				JamId = It.Id;
				for (FDSBeltItem& Other : Belt.Items) Belt.MarkItemDirty(Other);
				if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>()) { GS->Multicast_Toast(LOCTEXT("JamToast", "Convoyeur bloque par un caddie !")); GS->Multicast_Sfx(EDSSfx::Jam, GetJamLocation(), 0.3f); }
				break;
			}
		}
	}
	// Arrivee au comptoir : vente.
	int32 Value = 0, Count = 0;
	for (int32 i = Belt.Items.Num() - 1; i >= 0; --i)
	{
		if (Belt.Items[i].Distance < L) continue;
		const UDSPickupData* T = M && M->Types.IsValidIndex(Belt.Items[i].TypeIndex) ? M->Types[Belt.Items[i].TypeIndex].Get() : nullptr;
		Value += T ? T->Value : 0;
		++Count;
		Belt.Items.RemoveAtSwap(i);
		Belt.MarkArrayDirty();
	}
	if (Count > 0)
	{
		if (Counter) Counter->SellValue(Value, nullptr, Spline->GetLocationAtDistanceAlongSpline(L, ESplineCoordinateSpace::World) + FVector(0, 0, 180.f));
		if (M) M->NotifyCollected(Count);
	}
}

void ADSConveyor::ServerCatchFallenPickups()
{
	// Un objet libre qui tombe sur le tapis (lance, lache) devient un element du tapis.
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	if (!Sub || !M) return;
	TArray<ADSPickup*> Caught;
	for (const TWeakObjectPtr<ADSPickup>& W : Sub->GetActives())
	{
		ADSPickup* P = W.Get();
		if (!P || P->IsHeld() || !P->GetType()) continue;
		const FVector L = P->GetActorLocation();
		const FVector OnSpline = Spline->FindLocationClosestToWorldLocation(L, ESplineCoordinateSpace::World);
		if (FVector::Dist2D(L, OnSpline) < 110.f && L.Z - OnSpline.Z < 120.f && L.Z - OnSpline.Z > -40.f) Caught.Add(P);
	}
	for (ADSPickup* P : Caught)
	{
		const float Key = Spline->FindInputKeyClosestToWorldLocation(P->GetActorLocation());
		FDSBeltItem& It = Belt.Items.AddDefaulted_GetRef();
		It.Id = NextId++;
		It.TypeIndex = static_cast<uint8>(FMath::Max(0, M->GetTypeIndex(P->GetType())));
		It.Distance = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		Belt.MarkItemDirty(It);
		P->Destroy();
	}
}

FTransform ADSConveyor::ItemTransform(const FDSBeltItem& It) const
{
	const float D = FMath::Clamp(It.Distance, 0.f, GetLength());
	const FVector P = Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
	const FRotator R = Spline->GetRotationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
	const FVector Right = FRotationMatrix(R).GetUnitAxis(EAxis::Y);
	float Radius = 40.f;
	FVector Scale(1.f);
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
		if (ADSPickupManager* M = Sub->GetManager())
			if (M->Types.IsValidIndex(It.TypeIndex) && M->Types[It.TypeIndex]) { Radius = M->Types[It.TypeIndex]->Radius; Scale = M->Types[It.TypeIndex]->MeshScale; }
	const FVector Loc = P + Right * FMath::Sin(It.Id * 1.7f) * 45.f + FVector(0, 0, Radius * 0.7f);
	return FTransform(FRotator(0.f, R.Yaw + It.Id * 37.f, 0.f), Loc, Scale);
}

void ADSConveyor::UpdateVisuals(float DeltaSeconds)
{
	const float L = GetLength();
	// Bandes qui defilent
	if (!bJammed) StripePhase = FMath::Fmod(StripePhase + Speed * DeltaSeconds, 120.f);
	const int32 NS = StripeISM->GetInstanceCount();
	TArray<FTransform> Stripes;
	Stripes.Reserve(NS);
	for (int32 i = 0; i < NS; ++i)
	{
		const float D = FMath::Fmod(i * 120.f + StripePhase, FMath::Max(L, 1.f));
		const FVector P = Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		const FRotator R = Spline->GetRotationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		Stripes.Add(FTransform(R, P - FVector(0, 0, 2.f), FVector(0.25f, 2.02f, 0.02f)));
	}
	if (NS) StripeISM->BatchUpdateInstancesTransforms(0, Stripes, true, true, true);

	// Objets : reconstruit les instances (quelques dizaines au plus)
	TArray<TArray<FTransform>> PerType;
	PerType.SetNum(ItemISMs.Num());
	for (const FDSBeltItem& It : Belt.Items)
		if (PerType.IsValidIndex(It.TypeIndex)) PerType[It.TypeIndex].Add(ItemTransform(It));
	for (int32 t = 0; t < ItemISMs.Num(); ++t)
	{
		UInstancedStaticMeshComponent* ISM = ItemISMs[t];
		if (ISM->GetInstanceCount() != PerType[t].Num())
		{
			ISM->ClearInstances();
			ISM->AddInstances(PerType[t], false, true);
		}
		else if (PerType[t].Num())
			ISM->BatchUpdateInstancesTransforms(0, PerType[t], true, true, true);
	}

	JamLabel->SetVisibility(bJammed);
	if (bJammed) JamLabel->SetWorldLocation(GetJamLocation() + FVector(0, 0, 230.f));
}

void ADSConveyor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		ServerAdvance(DeltaSeconds);
		CatchTimer -= DeltaSeconds;
		if (CatchTimer <= 0.f) { CatchTimer = 0.2f; ServerCatchFallenPickups(); }
	}
	else if (!bJammed)
	{
		// Client : extrapole avec la vitesse repliquee (le serveur corrige aux ajouts / blocages).
		for (FDSBeltItem& It : Belt.Items) It.Distance = FMath::Min(It.Distance + Speed * DeltaSeconds, GetLength());
	}
	UpdateVisuals(DeltaSeconds);
}

#undef LOCTEXT_NAMESPACE
