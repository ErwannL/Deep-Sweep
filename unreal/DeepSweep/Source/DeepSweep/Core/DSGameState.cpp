#include "Core/DSGameState.h"
#include "Net/UnrealNetwork.h"

ADSGameState::ADSGameState() {}

void ADSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSGameState, Credits);
	DOREPLIFETIME(ADSGameState, Cleanliness);
	DOREPLIFETIME(ADSGameState, MatchStartTime);
}

void ADSGameState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) MatchStartTime = GetServerWorldTimeSeconds();
}

float ADSGameState::GetElapsedTime() const
{
	return FMath::Max(0.f, static_cast<float>(GetServerWorldTimeSeconds()) - MatchStartTime);
}

void ADSGameState::AddCredits(int32 Amount)
{
	if (HasAuthority()) Credits += Amount;
}

bool ADSGameState::SpendCredits(int32 Amount)
{
	if (!HasAuthority() || Credits < Amount) return false;
	Credits -= Amount;
	return true;
}

void ADSGameState::Multicast_Toast_Implementation(const FText& Message)
{
	Toast = Message;
	ToastTime = GetWorld()->GetTimeSeconds();
}

void ADSGameState::SetCleanliness(float Value)
{
	if (HasAuthority()) Cleanliness = FMath::Clamp(Value, 0.f, 1.f);
}
