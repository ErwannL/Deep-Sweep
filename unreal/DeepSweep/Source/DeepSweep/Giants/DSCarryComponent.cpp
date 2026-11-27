#include "Giants/DSCarryComponent.h"
#include "Player/DSDiver.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

UDSCarryComponent::UDSCarryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UDSCarryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDSCarryComponent, Carriers);
	DOREPLIFETIME(UDSCarryComponent, bBalloons);
}

int32 UDSCarryComponent::GetRequiredCarriers() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const int32 Players = GS ? FMath::Max(1, GS->PlayerArray.Num()) : 1;
	return FMath::Clamp(DesignCarriers, 1, Players);
}

float UDSCarryComponent::GetSpeedFactor() const
{
	const int32 C = Carriers.Num();
	if (C == 0) return 0.f;
	// Prototype : min(1, c/requis) * (0.35 + 0.65 * min(1, c/design)) ; ballons x1,6.
	float F = FMath::Min(1.f, float(C) / GetRequiredCarriers()) * (0.35f + 0.65f * FMath::Min(1.f, float(C) / DesignCarriers));
	if (bBalloons) F = FMath::Min(1.f, F * 1.6f);
	return F;
}

bool UDSCarryComponent::AddCarrier(ADSDiver* Diver)
{
	if (!Diver || Carriers.Contains(Diver) || !HasFreeSlot()) return false;
	Carriers.Add(Diver);
	OnRep_Carriers();
	return true;
}

void UDSCarryComponent::RemoveCarrier(ADSDiver* Diver)
{
	if (Carriers.Remove(Diver) > 0) OnRep_Carriers();
}

void UDSCarryComponent::ReleaseAll()
{
	Carriers.Reset();
	OnRep_Carriers();
}

void UDSCarryComponent::ComputeMotion(FVector& OutVelocity, float& OutYawRate) const
{
	OutVelocity = FVector::ZeroVector;
	OutYawRate = 0.f;
	if (Carriers.Num() == 0) return;
	FVector Sum = FVector::ZeroVector;
	for (const ADSDiver* D : Carriers) if (D) Sum += D->GetCarryIntent();
	const FVector Avg = Sum / Carriers.Num();
	OutVelocity = Avg * FullSpeed * GetSpeedFactor();
	// Desaccord : plus les intentions divergent, plus l'objet tourne sur lui-meme.
	const FVector Dir = Avg.GetSafeNormal();
	float Disagree = 0.f;
	for (const ADSDiver* D : Carriers)
		if (D && !D->GetCarryIntent().IsNearlyZero()) Disagree += 1.f - FVector::DotProduct(D->GetCarryIntent(), Dir);
	OutYawRate = Disagree * 45.f;
}

void UDSCarryComponent::OnRep_Carriers()
{
	Carriers.RemoveAll([](const TObjectPtr<ADSDiver>& D) { return !IsValid(D); });
	OnCarriersChanged.Broadcast();
}
