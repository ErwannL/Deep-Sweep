#include "World/DSCounter.h"
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "FX/DSPopupText.h"
#include "Audio/DSAudioSubsystem.h"
#include "FX/DSFX.h"
#include "DeepSweep.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSCounter::ADSCounter()
{
	InteractRadius = 450.f;
	LabelHeight = 480.f;
	LabelColor = FLinearColor(0.05f, 0.55f, 0.25f);
	Priority = 5;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetStaticMesh(Cube.Object);
	Body->SetRelativeScale3D(FVector(4.f, 4.f, 2.5f));
	Body->SetRelativeLocation(FVector(0, 0, 125.f));
	RootComponent = Body;

	Rim = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rim"));
	Rim->SetupAttachment(Body);
	Rim->SetStaticMesh(Cube.Object);
	Rim->SetRelativeLocation(FVector(0, 0, 52.f));
	Rim->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.06f));
	Rim->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Hopper = CreateDefaultSubobject<UBoxComponent>(TEXT("Hopper"));
	Hopper->SetupAttachment(RootComponent);
	Hopper->SetAbsolute(false, false, true);
	Hopper->SetBoxExtent(FVector(240.f, 240.f, 130.f));
	Hopper->SetRelativeLocation(FVector(0, 0, 70.f)); // au-dessus du plateau (coord. relatives a l'echelle du corps)
	Hopper->SetCollisionProfileName(TEXT("OverlapAll"));
	Hopper->SetGenerateOverlapEvents(true);

	Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
	Sign->SetupAttachment(RootComponent);
	Sign->SetAbsolute(false, false, true);
	Sign->SetText(LOCTEXT("CounterSign", "COMPTOIR"));
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetWorldSize(70.f);
	Sign->SetTextRenderColor(FColor(255, 240, 120));
	Sign->SetRelativeLocation(FVector(0, 0, 110.f));
	Sign->SetRelativeRotation(FRotator(0, 180.f, 0));
	SignBack = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignBack"));
	SignBack->SetupAttachment(RootComponent);
	SignBack->SetAbsolute(false, false, true);
	SignBack->SetText(LOCTEXT("CounterSign", "COMPTOIR"));
	SignBack->SetHorizontalAlignment(EHTA_Center);
	SignBack->SetWorldSize(70.f);
	SignBack->SetTextRenderColor(FColor(255, 240, 120));
	SignBack->SetRelativeLocation(FVector(0, 0, 110.f));
	SignBack->SetRelativeRotation(FRotator(0, 0.f, 0));
}

void ADSCounter::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), 0.15f);
		}
	};
	Tint(Body, FLinearColor(0.1f, 0.75f, 0.35f));
	Tint(Rim, FLinearColor(0.02f, 0.2f, 0.1f));
	if (HasAuthority()) Hopper->OnComponentBeginOverlap.AddDynamic(this, &ADSCounter::OnHopperOverlap);
}

FVector ADSCounter::GetHopperLocation() const { return Hopper->GetComponentLocation(); }

bool ADSCounter::CanInteract(const ADSDiver* Diver) const
{
	return Diver && Diver->GetGrab() && Diver->GetGrab()->NumHeld() > 0;
}

FText ADSCounter::GetPrompt(const ADSDiver* Diver) const
{
	return LOCTEXT("CounterPrompt", "Attraper : vendre");
}

void ADSCounter::Interact(ADSDiver* Diver)
{
	if (!HasAuthority() || !Diver) return;
	TArray<ADSPickup*> ToSell;
	for (ADSPickup* P : Diver->GetGrab()->GetHeld()) ToSell.Add(P);
	for (ADSPickup* P : ToSell)
	{
		Diver->GetGrab()->ServerForget(P);
		Sell(P, Diver);
	}
}

void ADSCounter::OnHopperOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ADSPickup* P = Cast<ADSPickup>(Other);
	if (P && !P->IsHeld()) Sell(P, P->GetLastThrower());
}

void ADSCounter::Sell(ADSPickup* Pickup, AActor* Seller)
{
	if (!HasAuthority() || !IsValid(Pickup) || !Pickup->GetType() || Pickup->IsActorBeingDestroyed()) return;
	const int32 Value = Pickup->GetType()->Value;
	if (UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
		if (ADSPickupManager* M = Sub->GetManager()) M->NotifyCollected(1);
	Pickup->Destroy();
	SellValue(Value, Seller, GetHopperLocation() + FVector(0, 0, 150.f));
}

void ADSCounter::SellValue(int32 Value, AActor* Seller, const FVector& PopupLocation)
{
	if (!HasAuthority()) return;
	if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>()) GS->AddCredits(Value);
	if (const APawn* Pawn = Cast<APawn>(Seller))
		if (ADSPlayerState* PS = Pawn->GetPlayerState<ADSPlayerState>()) PS->StatSold += Value;
	Multicast_SaleFX(Value, PopupLocation);
}

void ADSCounter::Multicast_SaleFX_Implementation(int32 Value, FVector_NetQuantize Location)
{
	if (UDSAudioSubsystem* A = GetWorld()->GetSubsystem<UDSAudioSubsystem>()) A->PlaySfx(EDSSfx::Sale, Location);
	ADSFXBurst::Play(this, EDSFX::Coins, Location, FMath::Clamp(Value / 10.f, 0.6f, 2.f));
	ADSPopupText::Spawn(this, Location + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0.f),
		FText::Format(LOCTEXT("SalePopup", "+{0}"), Value), FColor(255, 230, 60));
}

FText ADSCounter::GetStationLabel() const
{
	return LOCTEXT("StCounter", "COMPTOIR : vends tes dechets ici");
}

#undef LOCTEXT_NAMESPACE
