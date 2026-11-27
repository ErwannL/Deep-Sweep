#include "Pickups/DSPickupSubsystem.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Materials/MaterialInterface.h"

void UDSPickupSubsystem::SetManager(ADSPickupManager* InManager) { Manager = InManager; }
ADSPickupManager* UDSPickupSubsystem::GetManager() const { return Manager.Get(); }
void UDSPickupSubsystem::RegisterActive(ADSPickup* P) { Actives.AddUnique(P); }
void UDSPickupSubsystem::UnregisterActive(ADSPickup* P) { Actives.Remove(P); }

bool UDSPickupSubsystem::TrySleep(ADSPickup* P)
{
	return Manager.IsValid() && Manager->TrySleep(P);
}

ADSPickup* UDSPickupSubsystem::FindBestPickup(const FVector& From, const FVector& Forward, float Range) const
{
	ADSPickup* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector Fwd = Forward.GetSafeNormal();
	for (const TWeakObjectPtr<ADSPickup>& W : Actives)
	{
		ADSPickup* P = W.Get();
		if (!P || !P->GetType() || P->IsHeld()) continue;
		const FVector To = P->GetActorLocation() - From;
		const float Dist = FMath::Max(0.f, To.Size() - P->GetType()->Radius);
		if (Dist > Range) continue;
		const float Dot = FVector::DotProduct(To.GetSafeNormal(), Fwd);
		if (Dot < -0.2f && Dist > 60.f) continue; // derriere soi : seulement au contact
		const float Score = Dist * (1.6f - Dot);
		if (Score < BestScore) { BestScore = Score; Best = P; }
	}
	return Best;
}

UMaterialInterface* UDSPickupSubsystem::GetColorMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
		Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_DSColor.M_DSColor"));
	return Cached.Get();
}
