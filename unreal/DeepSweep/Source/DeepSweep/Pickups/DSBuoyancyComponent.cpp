#include "Pickups/DSBuoyancyComponent.h"
#include "Components/PrimitiveComponent.h"

UDSBuoyancyComponent::UDSBuoyancyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UDSBuoyancyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Applique partout ou le corps est simule (serveur + prediction locale client).
	if (!GetOwner()) return;
	UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	if (!Prim || !Prim->IsSimulatingPhysics()) return;

	const float Net = FMath::Clamp(Sink - Lift, -1.5f, 1.5f);
	Prim->AddForce(FVector(0.f, 0.f, -Net * BaseAcceleration), NAME_None, /*bAccelChange*/ true);
}
