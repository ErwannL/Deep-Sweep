#include "World/DSInteractable.h"
#include "Player/DSDiver.h"
#include "EngineUtils.h"

ADSInteractable::ADSInteractable()
{
	bReplicates = true;
}

bool ADSInteractable::IsInRange(const AActor* Diver) const
{
	if (!Diver) return false;
	const FVector D = Diver->GetActorLocation() - GetActorLocation();
	return FVector2D(D.X, D.Y).SizeSquared() < FMath::Square(InteractRadius) && FMath::Abs(D.Z) < 600.f;
}

ADSInteractable* ADSInteractable::FindBest(const ADSDiver* Diver)
{
	if (!Diver) return nullptr;
	ADSInteractable* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<ADSInteractable> It(Diver->GetWorld()); It; ++It)
	{
		ADSInteractable* I = *It;
		if (!I->IsInRange(Diver) || !I->CanInteract(Diver)) continue;
		const float Score = FVector::Dist(I->GetActorLocation(), Diver->GetActorLocation()) - I->Priority * 10000.f;
		if (Score < BestScore) { BestScore = Score; Best = I; }
	}
	return Best;
}
