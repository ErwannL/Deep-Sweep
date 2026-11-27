#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupSubsystem.h"
#include "DeepSweep.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Core/DSGameState.h"
#include "Machines/DSVacuumNozzle.h"
#include "Giants/DSGiantPickup.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarPickupDebug(TEXT("ds.Debug"), 0,
	TEXT("1 = affiche les compteurs dormant/actif et colore les niveaux de simulation."));

void FDSDormantItem::PostReplicatedAdd(const FDSDormantArray& A) { if (A.Owner) A.Owner->MarkVisualsDirty(); }
void FDSDormantItem::PostReplicatedChange(const FDSDormantArray& A) { if (A.Owner) A.Owner->MarkVisualsDirty(); }
void FDSDormantItem::PreReplicatedRemove(const FDSDormantArray& A) { if (A.Owner) A.Owner->MarkVisualsDirty(); }

ADSPickupManager::ADSPickupManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 5.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Dormant.Owner = this;
}

void ADSPickupManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSPickupManager, Dormant);
	DOREPLIFETIME(ADSPickupManager, TotalItems);
	DOREPLIFETIME(ADSPickupManager, CollectedItems);
}

void ADSPickupManager::BeginPlay()
{
	Super::BeginPlay();
	Dormant.Owner = this;

	// Une ISM par type (serveur et clients).
	UMaterialInterface* ColorMat = UDSPickupSubsystem::GetColorMaterial();
	for (UDSPickupData* T : Types)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
		ISM->SetupAttachment(RootComponent);
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCanEverAffectNavigation(false);
		ISM->RegisterComponent();
		if (T)
		{
			ISM->SetStaticMesh(T->Mesh);
			if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, ColorMat))
			{
				MID->SetVectorParameterValue(TEXT("Color"), T->Color);
				MID->SetScalarParameterValue(TEXT("Emissive"), FMath::Max(T->Emissive, 0.12f));
			}
		}
		TypeISMs.Add(ISM);
	}

	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>()) Sub->SetManager(this);
	if (HasAuthority()) SpawnInitialItems();
	bVisualsDirty = true;
}

void ADSPickupManager::SpawnInitialItems()
{
	FRandomStream Rng(Seed);
	int32& NextId = NextItemId;
	for (int32 TypeIndex = 0; TypeIndex < Types.Num(); ++TypeIndex)
	{
		const UDSPickupData* T = Types[TypeIndex];
		if (!T || PileCenters.Num() == 0) continue;
		for (int32 i = 0; i < T->SpawnCount; ++i)
		{
			const FVector C = PileCenters[Rng.RandRange(0, PileCenters.Num() - 1)];
			const float A = Rng.FRand() * UE_TWO_PI, D = FMath::Sqrt(Rng.FRand()) * PileRadius;
			FVector P(C.X + FMath::Cos(A) * D, C.Y + FMath::Sin(A) * D, 0.f);
			// Ceux qui coulent reposent au fond (en petit tas), ceux qui flottent derivent en hauteur.
			P.Z = T->Sink > 0.f ? T->Radius + Rng.FRandRange(0.f, 120.f) : Rng.FRandRange(500.f, SurfaceZ - 150.f);

			FDSDormantItem& It = Dormant.Items.AddDefaulted_GetRef();
			It.Id = NextId++;
			It.TypeIndex = static_cast<uint8>(TypeIndex);
			It.Location = P;
			It.Rotation = FRotator(Rng.FRandRange(-30.f, 30.f), Rng.FRand() * 360.f, Rng.FRandRange(-30.f, 30.f));
			Dormant.MarkItemDirty(It);
		}
	}
	int32 Giants = 0;
	for (TActorIterator<ADSGiantPickup> It(GetWorld()); It; ++It) ++Giants;
	TotalItems = NextId + Giants;
	UE_LOG(LogDeepSweep, Log, TEXT("PickupManager : %d objets places (%d types)"), TotalItems, Types.Num());
}

UDSPickupData* ADSPickupManager::TakeDormant(int32 DormantIndex)
{
	if (!HasAuthority() || !Dormant.Items.IsValidIndex(DormantIndex)) return nullptr;
	const uint8 TI = Dormant.Items[DormantIndex].TypeIndex;
	Dormant.Items.RemoveAtSwap(DormantIndex);
	Dormant.MarkArrayDirty();
	bVisualsDirty = true;
	return Types.IsValidIndex(TI) ? Types[TI].Get() : nullptr;
}

void ADSPickupManager::AddDormant(UDSPickupData* Type, const FVector& Location)
{
	if (!HasAuthority() || !Type) return;
	FDSDormantItem& It = Dormant.Items.AddDefaulted_GetRef();
	It.Id = NextItemId++;
	It.TypeIndex = static_cast<uint8>(FMath::Max(0, GetTypeIndex(Type)));
	It.Location = Location;
	It.Rotation = FRotator(0.f, FMath::FRand() * 360.f, 0.f);
	Dormant.MarkItemDirty(It);
	bVisualsDirty = true;
}

ADSPickup* ADSPickupManager::SpawnActive(UDSPickupData* Type, const FVector& Location, const FVector& Velocity)
{
	if (!HasAuthority() || !Type) return nullptr;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	ADSPickup* P = GetWorld()->SpawnActor<ADSPickup>(ADSPickup::StaticClass(), FTransform(Location), Params);
	if (!P) return nullptr;
	P->InitPickup(Type, NextItemId++);
	P->FinishSpawning(FTransform(FRotator::ZeroRotator, Location, Type->MeshScale));
	P->GetMesh()->SetPhysicsLinearVelocity(Velocity);
	return P;
}

void ADSPickupManager::GatherDiverLocations(TArray<FVector>& Out) const
{
	for (TActorIterator<APawn> It(GetWorld()); It; ++It) Out.Add(It->GetActorLocation());
}

void ADSPickupManager::NotifyCollected(int32 Count)
{
	if (!HasAuthority()) return;
	CollectedItems += Count;
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
		GS->SetCleanliness(TotalItems > 0 ? float(CollectedItems) / float(TotalItems) : 0.f);
}

void ADSPickupManager::AddExtraItems(int32 Count)
{
	if (HasAuthority()) TotalItems += Count;
}

ADSPickup* ADSPickupManager::Activate(int32 DormantIndex)
{
	if (!HasAuthority() || !Dormant.Items.IsValidIndex(DormantIndex)) return nullptr;
	const FDSDormantItem Item = Dormant.Items[DormantIndex];
	UDSPickupData* T = Types.IsValidIndex(Item.TypeIndex) ? Types[Item.TypeIndex] : nullptr;
	if (!T) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	ADSPickup* P = GetWorld()->SpawnActor<ADSPickup>(ADSPickup::StaticClass(), FTransform(Item.Rotation, Item.Location), Params);
	if (!P) return nullptr;
	P->InitPickup(T, Item.Id);
	P->FinishSpawning(FTransform(Item.Rotation, Item.Location, T->MeshScale));

	Dormant.Items.RemoveAtSwap(DormantIndex);
	Dormant.MarkArrayDirty();
	bVisualsDirty = true;
	return P;
}

bool ADSPickupManager::TrySleep(ADSPickup* Pickup)
{
	if (!HasAuthority() || !Pickup || !Pickup->GetType()) return false;
	const FVector L = Pickup->GetActorLocation();
	TArray<FVector> Divers;
	GatherDiverLocations(Divers);
	for (const FVector& D : Divers)
		if (FVector::DistSquared(D, L) < FMath::Square(SleepRadius)) return false;

	FDSDormantItem& It = Dormant.Items.AddDefaulted_GetRef();
	It.Id = Pickup->GetPickupId();
	It.TypeIndex = static_cast<uint8>(FMath::Max(0, GetTypeIndex(Pickup->GetType())));
	It.Location = L;
	It.Rotation = Pickup->GetActorRotation();
	Dormant.MarkItemDirty(It);
	Pickup->Destroy();
	bVisualsDirty = true;
	return true;
}

void ADSPickupManager::UpdateActivation()
{
	TArray<FVector> Divers;
	GatherDiverLocations(Divers);
	if (Divers.Num() == 0) return;
	const UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	int32 Active = Sub ? Sub->NumActive() : 0;
	// Sources d'activation : plongeurs + buses qui aspirent (rayon d'aspiration).
	TArray<TPair<FVector, float>> Sources;
	for (const FVector& D : Divers) Sources.Emplace(D, FMath::Square(ActivationRadius));
	for (TActorIterator<ADSVacuumNozzle> It(GetWorld()); It; ++It)
		if (It->IsSucking()) Sources.Emplace(It->GetActorLocation(), FMath::Square(It->GetActivationRadius()));
	for (int32 i = Dormant.Items.Num() - 1; i >= 0 && Active < MaxActive; --i)
	{
		const FVector L = Dormant.Items[i].Location;
		for (const TPair<FVector, float>& Src : Sources)
			if (FVector::DistSquared(Src.Key, L) < Src.Value)
			{
				if (Activate(i)) ++Active;
				break;
			}
	}
}

void ADSPickupManager::RebuildVisuals()
{
	bVisualsDirty = false;
	TArray<TArray<FTransform>> PerType;
	PerType.SetNum(TypeISMs.Num());
	for (const FDSDormantItem& It : Dormant.Items)
		if (PerType.IsValidIndex(It.TypeIndex) && Types[It.TypeIndex])
			PerType[It.TypeIndex].Add(FTransform(It.Rotation, It.Location, Types[It.TypeIndex]->MeshScale));
	for (int32 t = 0; t < TypeISMs.Num(); ++t)
	{
		TypeISMs[t]->ClearInstances();
		TypeISMs[t]->AddInstances(PerType[t], false, true);
	}
}

void ADSPickupManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		ActivationTimer -= DeltaSeconds;
		if (ActivationTimer <= 0.f) { ActivationTimer = 0.2f; UpdateActivation(); }
	}
	if (bVisualsDirty) RebuildVisuals();

	if (CVarPickupDebug.GetValueOnGameThread() > 0 && GEngine)
	{
		const UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
		GEngine->AddOnScreenDebugMessage(1001, 0.f, FColor::Cyan, FString::Printf(TEXT("Objets : %d total | %d dormants | %d actifs | %d collectes"),
			TotalItems, Dormant.Items.Num(), Sub ? Sub->NumActive() : 0, CollectedItems));
	}
}
