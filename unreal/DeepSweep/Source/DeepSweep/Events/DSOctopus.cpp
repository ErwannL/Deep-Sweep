#include "Events/DSOctopus.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Core/DSGameState.h"
#include "World/DSCounter.h"
#include "Player/DSDiver.h"
#include "FX/DSPopupText.h"
#include "DeepSweep.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSOctopus::ADSOctopus()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(true);
	bAlwaysRelevant = true;
	NetUpdateFrequency = 20.f;
	InteractRadius = 320.f;
	Priority = 20; // attraper le poulpe passe avant tout

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetStaticMesh(Sph.Object);
	Head->SetRelativeScale3D(FVector(2.f, 2.f, 2.4f));
	Head->SetRelativeLocation(FVector(0, 0, 60.f));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (int32 i = 0; i < 8; ++i)
	{
		UStaticMeshComponent* L = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Leg%d"), i));
		L->SetupAttachment(Root);
		L->SetStaticMesh(Cone.Object);
		L->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Legs.Add(L);
	}
	for (int32 s = -1; s <= 1; s += 2)
	{
		UStaticMeshComponent* E = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Eye%d"), s + 1));
		E->SetupAttachment(Head);
		E->SetStaticMesh(Sph.Object);
		E->SetRelativeLocation(FVector(40.f, s * 18.f, 15.f));
		E->SetRelativeScale3D(FVector(0.2f));
		E->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Loot%d"), i));
		M->SetupAttachment(Root);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetVisibility(false);
		LootMeshes.Add(M);
	}
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(60.f);
	Label->SetRelativeLocation(FVector(0, 0, 300.f));
	Label->SetTextRenderColor(FColor(230, 120, 255));
	Label->SetText(LOCTEXT("OctoLabel", "VOLEUR ! (attraper)"));
}

void ADSOctopus::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSOctopus, State);
	DOREPLIFETIME(ADSOctopus, Loot);
}

void ADSOctopus::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col, float E)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), E);
		}
	};
	Tint(Head, FLinearColor(0.63f, 0.25f, 0.75f), 0.25f);
	for (UStaticMeshComponent* L : Legs) Tint(L, FLinearColor(0.54f, 0.19f, 0.67f), 0.2f);
	TArray<USceneComponent*> Kids;
	Head->GetChildrenComponents(false, Kids);
	for (USceneComponent* C : Kids) if (UStaticMeshComponent* E = Cast<UStaticMeshComponent>(C)) Tint(E, FLinearColor::White, 0.8f);
	// Premiere apparition : 45-75 s (prototype)
	IdleTimer = FMath::FRandRange(45.f, 75.f);
	ApplyVisibility();
}

int32 ADSOctopus::NumPlayers() const
{
	const AGameStateBase* GS = GetWorld()->GetGameState();
	return GS ? FMath::Max(1, GS->PlayerArray.Num()) : 1;
}

void ADSOctopus::TriggerNow() { IdleTimer = 0.f; }

void ADSOctopus::OnRep_State() { ApplyVisibility(); }

void ADSOctopus::ApplyVisibility()
{
	SetActorHiddenInGame(State == EDSOctoState::Idle);
}

bool ADSOctopus::IsInRange(const AActor* Diver) const
{
	return Diver && State != EDSOctoState::Idle && FVector::Dist(Diver->GetActorLocation(), GetActorLocation()) < InteractRadius;
}

bool ADSOctopus::CanInteract(const ADSDiver* Diver) const { return State != EDSOctoState::Idle; }

FText ADSOctopus::GetPrompt(const ADSDiver*) const { return LOCTEXT("OctoPrompt", "Attraper : attraper le poulpe !"); }

void ADSOctopus::Interact(ADSDiver* Diver)
{
	if (!HasAuthority() || State == EDSOctoState::Idle) return;
	// Attrape ! Il lache tout et l'equipe gagne une prime.
	Multicast_Burst();
	DropLoot(false);
	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It) { It->SellValue(CatchReward, Diver, GetActorLocation() + FVector(0, 0, 200.f)); break; }
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
	{
		GS->Multicast_Toast(FText::Format(LOCTEXT("OctoCaught", "Poulpe attrape ! Il lache tout (+{0} credits)"), CatchReward));
		GS->Multicast_Sfx(EDSSfx::Splash, GetActorLocation(), 0.4f);
	}
	UE_LOG(LogDeepSweep, Log, TEXT("Poulpe attrape par %s"), *GetNameSafe(Diver));
	Reset();
}

void ADSOctopus::DropLoot(bool bHideAtEdge)
{
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	if (!M) return;
	const FVector Here = GetActorLocation();
	for (UDSPickupData* T : Loot)
	{
		if (bHideAtEdge)
		{
			// Cache pres du bord : redevient un objet dormant a ramasser plus tard.
			FVector P = Here.GetSafeNormal2D() * (ArenaRadius - 300.f) + FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), 0.f);
			P.Z = T->Radius;
			M->AddDormant(T, P);
		}
		else
		{
			const FVector Off(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 50.f);
			M->SpawnActive(T, Here + Off, FVector(FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(-400.f, 400.f), 300.f));
		}
	}
	Loot.Reset();
}

void ADSOctopus::Reset()
{
	State = EDSOctoState::Idle;
	bHasFleeTarget = false;
	IdleTimer = FMath::Max(60.f, 150.f - 20.f * NumPlayers()) + FMath::FRandRange(0.f, 60.f); // plus frequent a plusieurs
	ApplyVisibility();
}

void ADSOctopus::Multicast_Burst_Implementation()
{
	ADSPopupText::Spawn(this, GetActorLocation() + FVector(0, 0, 150.f), LOCTEXT("Splash", "SPLOTCH !"), FColor(40, 20, 60), 120.f);
}

void ADSOctopus::ServerTick(float DeltaSeconds)
{
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!M) return;

	if (State == EDSOctoState::Idle)
	{
		IdleTimer -= DeltaSeconds;
		if (IdleTimer > 0.f || (GS && GS->GetCleanliness() > 0.9f) || M->GetDormantItems().Num() == 0) return;
		// Arrive par un bord cote arene (pas cote base)
		const float A = FMath::FRandRange(-0.45f, 0.45f) * PI;
		SetActorLocation(FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * ArenaRadius + FVector(0, 0, 180.f));
		State = EDSOctoState::Hunt;
		ApplyVisibility();
		if (ADSGameState* G = GetWorld()->GetGameState<ADSGameState>()) { G->Multicast_Toast(LOCTEXT("OctoArrive", "Un poulpe voleur ! Attrapez-le !")); G->Multicast_Sfx(EDSSfx::OctoArrive, FVector::ZeroVector); }
		UE_LOG(LogDeepSweep, Log, TEXT("Poulpe voleur en approche"));
		return;
	}

	const FVector Here = GetActorLocation();
	FVector Goal = Here;
	float Speed = HuntSpeed;
	if (State == EDSOctoState::Hunt)
	{
		// Cible : le meilleur compromis valeur / distance parmi les objets dormants (vise les precieux)
		int32 Best = INDEX_NONE;
		float BestScore = -1e9f;
		const TArray<FDSDormantItem>& Items = M->GetDormantItems();
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			const UDSPickupData* T = M->Types.IsValidIndex(Items[i].TypeIndex) ? M->Types[Items[i].TypeIndex].Get() : nullptr;
			if (!T) continue;
			const float Score = T->Value * 300.f - FVector::Dist2D(Here, Items[i].Location);
			if (Score > BestScore) { BestScore = Score; Best = i; }
		}
		if (Best == INDEX_NONE || Loot.Num() >= MaxLoot) State = EDSOctoState::Flee;
		else
		{
			Goal = FVector(Items[Best].Location) + FVector(0, 0, 120.f);
			if (FVector::Dist(Here, Goal) < 170.f)
				if (UDSPickupData* T = M->TakeDormant(Best)) Loot.Add(T);
		}
	}
	if (State == EDSOctoState::Flee)
	{
		if (!bHasFleeTarget)
		{
			FleeTarget = Here.GetSafeNormal2D() * (ArenaRadius + 100.f);
			if (FleeTarget.IsNearlyZero()) FleeTarget = FVector(ArenaRadius, 0, 0);
			FleeTarget.X = FMath::Max(FleeTarget.X, -2600.f); // jamais vers la base
			FleeTarget.Z = 180.f;
			bHasFleeTarget = true;
		}
		Goal = FleeTarget;
		Speed = FleeSpeed;
		if (FVector::Dist2D(Here, FleeTarget) < 150.f)
		{
			DropLoot(true);
			if (ADSGameState* G = GetWorld()->GetGameState<ADSGameState>()) G->Multicast_Toast(LOCTEXT("OctoEscape", "Le poulpe a cache son butin pres du bord..."));
			Reset();
			return;
		}
	}
	const FVector D = Goal - Here;
	const float Step = FMath::Min(Speed * DeltaSeconds, D.Size());
	FVector Next = Here + D.GetSafeNormal() * Step;
	Next.Z = FMath::Max(Next.Z, 120.f);
	SetActorLocationAndRotation(Next, FRotator(0.f, D.Rotation().Yaw, 0.f));
}

void ADSOctopus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority()) ServerTick(DeltaSeconds);
	if (State == EDSOctoState::Idle) return;

	// Animation locale : ondulation des tentacules, pulsation, butin qui tourne
	const float T = GetWorld()->GetTimeSeconds();
	Head->SetRelativeLocation(FVector(0, 0, 60.f + FMath::Sin(T * 5.f) * 20.f));
	Head->SetRelativeScale3D(FVector(2.f, 2.f, 2.4f) * (1.f + FMath::Sin(T * 8.f) * 0.06f));
	for (int32 i = 0; i < Legs.Num(); ++i)
	{
		const float A = i / 8.f * UE_TWO_PI;
		const float W = FMath::Sin(T * 6.f + i) * 20.f;
		Legs[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 60.f, FMath::Sin(A) * 60.f, -40.f));
		Legs[i]->SetRelativeRotation(FRotator(FMath::Cos(A) * (-150.f - W), 0.f, FMath::Sin(A) * (150.f + W)).GetNormalized());
		Legs[i]->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.6f));
	}
	for (int32 i = 0; i < LootMeshes.Num(); ++i)
	{
		const bool bShow = Loot.IsValidIndex(i) && Loot[i];
		LootMeshes[i]->SetVisibility(bShow);
		if (!bShow) continue;
		if (LootMeshes[i]->GetStaticMesh() != Loot[i]->Mesh)
		{
			LootMeshes[i]->SetStaticMesh(Loot[i]->Mesh);
			if (UMaterialInstanceDynamic* MID = LootMeshes[i]->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Loot[i]->Color);
				MID->SetScalarParameterValue(TEXT("Emissive"), FMath::Max(0.3f, Loot[i]->Emissive));
			}
		}
		const float A = T * 3.f + i * 2.f;
		LootMeshes[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 110.f, FMath::Sin(A) * 110.f, 230.f));
		LootMeshes[i]->SetRelativeScale3D(Loot[i]->MeshScale * 0.7f);
	}
}

#undef LOCTEXT_NAMESPACE
