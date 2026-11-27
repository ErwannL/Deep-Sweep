#include "Core/DSGameState.h"
#include "Net/UnrealNetwork.h"
#include "DeepSweep.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

static FAutoConsoleCommandWithWorld GCmdEndMatch(TEXT("ds.EndMatch"), TEXT("Debug : termine la plongee (ecran de fin)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W) { if (ADSGameState* GS = W ? W->GetGameState<ADSGameState>() : nullptr) GS->EndMatch(); }));

ADSGameState::ADSGameState() {}

void ADSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSGameState, Credits);
	DOREPLIFETIME(ADSGameState, Cleanliness);
	DOREPLIFETIME(ADSGameState, MatchStartTime);
	DOREPLIFETIME(ADSGameState, OwnedUpgrades);
	DOREPLIFETIME(ADSGameState, bMatchOver);
	DOREPLIFETIME(ADSGameState, FinalTime);
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
	if (!HasAuthority()) return;
	Cleanliness = FMath::Clamp(Value, 0.f, 1.f);
	if (Cleanliness >= 1.f) EndMatch();
}

void ADSGameState::EndMatch()
{
	if (!HasAuthority() || bMatchOver) return;
	bMatchOver = true;
	FinalTime = GetElapsedTime();
	UE_LOG(LogDeepSweep, Log, TEXT("Crique nettoyee en %.0f s, %d credits"), FinalTime, Credits);
}
