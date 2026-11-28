#include "Core/DSGameState.h"
#include "Net/UnrealNetwork.h"
#include "DeepSweep.h"
#include "Player/DSDiver.h"
#include "Kismet/GameplayStatics.h"
#include "FX/DSFX.h"
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

void ADSGameState::Multicast_Sfx_Implementation(EDSSfx Sfx, FVector_NetQuantize Location, float Shake)
{
	if (UDSAudioSubsystem* A = GetWorld()->GetSubsystem<UDSAudioSubsystem>()) A->PlaySfx(Sfx, Location);
	if (!Location.IsZero())
		switch (Sfx)
		{
		case EDSSfx::Slurp: ADSFXBurst::Play(this, EDSFX::Bubbles, Location, 0.6f); break;
		case EDSSfx::Clog: case EDSSfx::Bonk: case EDSSfx::Jam: ADSFXBurst::Play(this, EDSFX::Sparks, Location); break;
		case EDSSfx::Unclog: ADSFXBurst::Play(this, EDSFX::BigBubbles, Location, 0.7f); break;
		case EDSSfx::Break: ADSFXBurst::Play(this, EDSFX::Shards, Location); break;
		case EDSSfx::Recycled: ADSFXBurst::Play(this, EDSFX::BigBubbles, Location); break;
		case EDSSfx::Jingle: ADSFXBurst::Play(this, EDSFX::Confetti, Location + FVector(0, 0, 200.f), 1.3f); ADSFXBurst::Play(this, EDSFX::Coins, Location, 1.5f); break;
		case EDSSfx::Buy: ADSFXBurst::Play(this, EDSFX::Confetti, Location + FVector(0, 0, 150.f)); break;
		case EDSSfx::Splash: ADSFXBurst::Play(this, EDSFX::Ink, Location, 1.2f); break;
		default: break;
		}
	if (Shake > 0.f)
		if (ADSDiver* Me = Cast<ADSDiver>(UGameplayStatics::GetPlayerPawn(this, 0)))
			Me->AddShake(Shake * FMath::Clamp(1.3f - FVector::Dist(Me->GetActorLocation(), Location) / 4000.f, 0.f, 1.f));
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
	Multicast_Sfx(EDSSfx::BigJingle, FVector::ZeroVector, 0.f);
	UE_LOG(LogDeepSweep, Log, TEXT("Crique nettoyee en %.0f s, %d credits"), FinalTime, Credits);
}
