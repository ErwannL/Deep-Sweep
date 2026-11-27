#include "Machines/DSVacuum.h"
#include "Machines/DSVacuumNozzle.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Core/DSGameState.h"
#include "World/DSCounter.h"
#include "Machines/DSConveyor.h"
#include "Player/DSDiver.h"
#include "DeepSweep.h"
#include "CableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSVacuum::ADSVacuum()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	TankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank"));
	TankMesh->SetStaticMesh(Cyl.Object);
	TankMesh->SetRelativeScale3D(FVector(4.f, 4.f, 4.f));
	TankMesh->SetRelativeLocation(FVector(0, 0, 200.f));
	RootComponent = TankMesh;

	TankRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	TankRing->SetupAttachment(TankMesh);
	TankRing->SetStaticMesh(Cyl.Object);
	TankRing->SetRelativeLocation(FVector(0, 0, 42.f));
	TankRing->SetRelativeScale3D(FVector(1.08f, 1.08f, 0.08f));
	TankRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(TankMesh);
	Label->SetAbsolute(false, false, true);
	Label->SetRelativeLocation(FVector(0, 0, 75.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(55.f);
	Label->SetTextRenderColor(FColor::White);

	Hose = CreateDefaultSubobject<UCableComponent>(TEXT("Hose"));
	Hose->SetupAttachment(TankMesh);
	Hose->SetAbsolute(false, false, true);
	Hose->SetRelativeLocation(FVector(55.f, 0.f, -20.f)); // sortie cote arene
	Hose->CableWidth = 22.f;
	Hose->NumSegments = 14;
	Hose->NumSides = 8;
	Hose->SolverIterations = 4;
	Hose->bEnableCollision = true;   // le tuyau traine et accroche le decor (cosmetique, local)
	Hose->CableGravityScale = 0.6f;
	Hose->EndLocation = FVector::ZeroVector;
}

void ADSVacuum::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSVacuum, Power);
	DOREPLIFETIME(ADSVacuum, TankCount);
	DOREPLIFETIME(ADSVacuum, Clog);
	DOREPLIFETIME(ADSVacuum, Capacity);
	DOREPLIFETIME(ADSVacuum, MaxPower);
}

void ADSVacuum::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UMeshComponent* C, const FLinearColor& Col)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), 0.15f);
		}
	};
	Tint(TankMesh, FLinearColor(0.9f, 0.15f, 0.12f));
	Tint(TankRing, FLinearColor(1.f, 0.85f, 0.2f));
	Tint(Hose, FLinearColor(1.f, 0.55f, 0.1f));
	if (Nozzle)
	{
		Nozzle->Vacuum = this;
		Hose->SetAttachEndTo(Nozzle, NAME_None);
		Hose->EndLocation = FVector(-40.f, 0.f, 0.f);
	}
	UpdateLabel();
}

FVector ADSVacuum::GetOutletLocation() const { return Hose->GetComponentLocation(); }
FVector ADSVacuum::GetRecycleLocation() const { return GetActorLocation() + FVector(320.f, FMath::FRandRange(-150.f, 150.f), 0.f); }

void ADSVacuum::CyclePower()
{
	if (!HasAuthority()) return;
	Power = Power % MaxPower + 1;
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
		GS->Multicast_Toast(Power >= 4 ? FText::Format(LOCTEXT("PowerDanger", "Puissance {0} : DANGER, ca aspire les collegues !"), Power)
		                               : FText::Format(LOCTEXT("Power", "Puissance {0}"), Power));
}

void ADSVacuum::StoreItem(UDSPickupData* Type)
{
	if (!HasAuthority() || !Type) return;
	TankItems.Add(Type);
	TankCount = TankItems.Num();
	if (ADSGameState* G = GetWorld()->GetGameState<ADSGameState>()) G->Multicast_Sfx(EDSSfx::Slurp, Nozzle ? Nozzle->GetActorLocation() : GetActorLocation());
	if (IsFull())
		if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
			{ GS->Multicast_Toast(LOCTEXT("TankFull", "Cuve pleine ! Au levier !")); GS->Multicast_Sfx(EDSSfx::Alarm, GetActorLocation()); }
}

TArray<TObjectPtr<UDSPickupData>> ADSVacuum::TakeTankContents()
{
	TArray<TObjectPtr<UDSPickupData>> Out = MoveTemp(TankItems);
	TankItems.Reset();
	TankCount = 0;
	BobTimer = 0.f;
	return Out;
}

void ADSVacuum::DumpTank(AActor* By)
{
	if (!HasAuthority()) return;
	if (TankItems.Num() == 0)
	{
		if (By) if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>()) GS->Multicast_Toast(LOCTEXT("TankEmpty", "Cuve vide"));
		return;
	}
	const TArray<TObjectPtr<UDSPickupData>> Items = TakeTankContents();
	// La cuve se deverse sur le convoyeur (vente a l'arrivee au comptoir).
	for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It)
	{
		It->AddItems(Items);
		UE_LOG(LogDeepSweep, Log, TEXT("Cuve videe sur le tapis : %d objets"), Items.Num());
		return;
	}
	// Repli sans convoyeur : vente directe.
	int32 Value = 0;
	for (const UDSPickupData* T : Items) Value += T ? T->Value : 0;
	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It)
	{
		It->SellValue(Value, nullptr, It->GetHopperLocation() + FVector(0, 0, 150.f));
		break;
	}
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
		if (ADSPickupManager* M = Sub->GetManager()) M->NotifyCollected(Items.Num());
	UE_LOG(LogDeepSweep, Log, TEXT("Cuve videe : %d objets, %d credits"), Items.Num(), Value);
}

bool ADSVacuum::IsLeverManned() const
{
	if (!LeverActor) return false;
	for (TActorIterator<ADSDiver> It(GetWorld()); It; ++It)
		if (FVector::Dist2D(It->GetActorLocation(), LeverActor->GetActorLocation()) < 300.f) return true;
	return false;
}

void ADSVacuum::UpdateLabel()
{
	if (ShownTank == TankCount) return;
	ShownTank = TankCount;
	Label->SetText(FText::Format(LOCTEXT("TankLabel", "Cuve {0}/{1}"), TankCount, Capacity));
	Label->SetTextRenderColor(IsFull() ? FColor(255, 80, 60) : FColor::White);
}

void ADSVacuum::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLabel();

	// Longueur du tuyau : suit la distance (un peu de mou pour qu'il traine au sol).
	if (Nozzle)
		Hose->CableLength = FMath::Max(200.f, FVector::Dist(GetOutletLocation(), Nozzle->GetActorLocation()) * 1.12f);

	// Bob vide la cuve quand personne n'est au levier (GDD 6.3).
	if (HasAuthority() && TankCount > 0 && !IsLeverManned())
	{
		BobTimer += DeltaSeconds;
		const bool bOn = Nozzle && Nozzle->bOn;
		const float Delay = IsFull() ? 2.5f : (!bOn || TankCount >= 10) ? 5.f : 10.f;
		if (BobTimer > Delay)
		{
			DumpTank(nullptr);
			if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
				GS->Multicast_Toast(LOCTEXT("BobDump", "Bob le robot a vide la cuve"));
		}
	}
	else if (TankCount == 0) BobTimer = 0.f;
}

#undef LOCTEXT_NAMESPACE
