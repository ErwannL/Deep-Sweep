#include "FX/DSPopupText.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

ADSPopupText::ADSPopupText()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Text;
}

ADSPopupText* ADSPopupText::Spawn(const UObject* WorldContext, const FVector& Location, const FText& InText, FColor Color, float Size)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
	ADSPopupText* P = World->SpawnActor<ADSPopupText>(Location, FRotator::ZeroRotator);
	if (P)
	{
		P->Text->SetText(InText);
		P->Text->SetTextRenderColor(Color);
		P->BaseSize = Size;
		P->Text->SetWorldSize(Size * 0.5f);
	}
	return P;
}

void ADSPopupText::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Age >= Life) { Destroy(); return; }
	const float T = Age / Life;
	AddActorWorldOffset(FVector(0, 0, 160.f * DeltaSeconds));
	// "pop" : grossit vite puis retrecit en fin de vie
	const float Pop = T < 0.15f ? FMath::Lerp(0.5f, 1.2f, T / 0.15f) : FMath::Lerp(1.2f, 0.8f, (T - 0.15f) / 0.85f);
	Text->SetWorldSize(BaseSize * Pop * (T > 0.8f ? (1.f - T) / 0.2f : 1.f));
	// toujours face a la camera locale
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector To = Cam->GetCameraLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}
}
