#include "Upgrades/DSUpgradePad.h"
#include "Core/DSGameState.h"
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Machines/DSVacuum.h"
#include "Machines/DSConveyor.h"
#include "Giants/DSGiantPickup.h"
#include "Giants/DSCarryComponent.h"
#include "Pickups/DSPickupSubsystem.h"
#include "FX/DSPopupText.h"
#include "Audio/DSAudioSubsystem.h"
#include "DeepSweep.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSUpgradePad::ADSUpgradePad()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 200.f;
	LabelHeight = 330.f;
	LabelColor = FLinearColor(0.45f, 0.15f, 0.65f);
	Priority = 3;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetStaticMesh(Cyl.Object);
	Disc->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.6f));
	Disc->SetRelativeLocation(FVector(0, 0, 30.f));
	RootComponent = Disc;
	Icon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Icon"));
	Icon->SetupAttachment(Disc);
	Icon->SetAbsolute(false, false, true);
	Icon->SetStaticMesh(Sph.Object);
	Icon->SetRelativeScale3D(FVector(0.6f));
	Icon->SetRelativeLocation(FVector(0, 0, 250.f));
	Icon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Disc);
	Label->SetAbsolute(false, false, true);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(60.f);
	Label->SetRelativeLocation(FVector(0, 0, 380.f));
	Label->SetRelativeRotation(FRotator(0, 0, 0));
}

void ADSUpgradePad::BeginPlay()
{
	Super::BeginPlay();
	ShownState = -1;
}

FText ADSUpgradePad::GetPrompt(const ADSDiver*) const
{
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (GS && GS->HasUpgrade(Upgrade)) return FText::Format(LOCTEXT("PadOwned", "{0} : deja achete"), Title);
	return FText::Format(LOCTEXT("PadBuy", "Attraper : acheter {0} ({1} credits) - {2}"), Title, Cost, Description);
}

void ADSUpgradePad::Interact(ADSDiver* Diver)
{
	ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!HasAuthority() || !GS) return;
	if (GS->HasUpgrade(Upgrade)) { GS->Multicast_Toast(LOCTEXT("Already", "Deja achete")); return; }
	if (!GS->SpendCredits(Cost))
	{
		GS->Multicast_Toast(FText::Format(LOCTEXT("TooPoor", "Pas assez de credits ({0})"), Cost));
		GS->Multicast_Sfx(EDSSfx::Denied, GetActorLocation());
		return;
	}
	GS->GrantUpgrade(Upgrade);
	ApplyOwnedUpgrades(GetWorld());
	GS->Multicast_Toast(FText::Format(LOCTEXT("Bought", "{0} !"), Title));
	GS->Multicast_Sfx(EDSSfx::Buy, GetActorLocation(), 0.2f);
	Multicast_Bought();
	UE_LOG(LogDeepSweep, Log, TEXT("Amelioration achetee : %s"), *Title.ToString());
}

void ADSUpgradePad::ApplyOwnedUpgrades(UWorld* World)
{
	const ADSGameState* GS = World ? World->GetGameState<ADSGameState>() : nullptr;
	if (!GS || !GS->HasAuthority()) return;
	for (TActorIterator<ADSDiver> It(World); It; ++It)
	{
		UDSGrabComponent* G = It->GetGrab();
		G->Capacity = GS->HasUpgrade(EDSUpgrade::Bag) ? 3 : 1;
		G->Range = GS->HasUpgrade(EDSUpgrade::Claw) ? 220.f * 1.6f : 220.f;
	}
	for (TActorIterator<ADSVacuum> It(World); It; ++It) It->MaxPower = GS->HasUpgrade(EDSUpgrade::Turbo) ? 5 : 3;
	for (TActorIterator<ADSConveyor> It(World); It; ++It) It->Speed = GS->HasUpgrade(EDSUpgrade::FastBelt) ? 600.f : 300.f;
	for (TActorIterator<ADSGiantPickup> It(World); It; ++It) It->GetCarry()->bBalloons = GS->HasUpgrade(EDSUpgrade::Balloons);
}

void ADSUpgradePad::Multicast_Bought_Implementation()
{
	BounceT = 1.f;
	ADSPopupText::Spawn(this, GetActorLocation() + FVector(0, 0, 300.f), LOCTEXT("New", "NOUVEAU !"), FColor(220, 120, 255), 90.f);
}

void ADSUpgradePad::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!GS) return;
	const bool bOwned = GS->HasUpgrade(Upgrade);
	const bool bAfford = GS->GetCredits() >= Cost;
	const int32 State = bOwned ? 2 : bAfford ? 1 : 0;

	// Icone qui flotte, rebondit a l'achat
	BounceT = FMath::Max(0.f, BounceT - DeltaSeconds);
	const float T = GetWorld()->GetTimeSeconds();
	Icon->SetRelativeLocation(FVector(0, 0, 250.f + FMath::Sin(T * 2.f + Cost) * 25.f + FMath::Sin(BounceT * PI) * 150.f));

	if (State == ShownState) return;
	ShownState = State;
	Label->SetText(bOwned ? FText::Format(LOCTEXT("PadLabelOwned", "OK : {0}"), Title)
	                      : FText::Format(LOCTEXT("PadLabel", "{0}\n{1} credits"), Title, Cost));
	Label->SetTextRenderColor(bOwned ? FColor(150, 150, 150) : bAfford ? FColor(255, 230, 120) : FColor(200, 170, 220));
	const FLinearColor DiscCol = bOwned ? FLinearColor(0.25f, 0.25f, 0.27f) : bAfford ? FLinearColor(0.83f, 0.42f, 1.f) : FLinearColor(0.42f, 0.23f, 0.5f);
	for (UStaticMeshComponent* C : { Disc.Get(), Icon.Get() })
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), DiscCol);
			MID->SetScalarParameterValue(TEXT("Emissive"), bAfford && !bOwned ? 0.6f : 0.1f);
		}
}

FText ADSUpgradePad::GetStationLabel() const
{
	const ADSGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADSGameState>() : nullptr;
	return GS && GS->HasUpgrade(Upgrade) ? FText::Format(LOCTEXT("StPadOk", "{0} (achete)"), Title) : FText::Format(LOCTEXT("StPad", "{0} : {1} cr"), Title, Cost);
}

#undef LOCTEXT_NAMESPACE
