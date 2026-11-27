// Scenarios de test reseau automatiques (hote + clients), lances par ligne de commande.
// Hote :   ds.TestNet <nb joueurs attendus>   Client : ds.TestNetClient 1
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "Giants/DSGiantPickup.h"
#include "Giants/DSCarryComponent.h"
#include "Machines/DSVacuum.h"
#include "Machines/DSVacuumNozzle.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "World/DSCounter.h"
#include "DeepSweep.h"
#include "EngineUtils.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "GameFramework/CharacterMovementComponent.h"

static void LogBandwidth(UWorld* W)
{
	if (UNetDriver* ND = W->GetNetDriver())
		for (UNetConnection* C : ND->ClientConnections)
			if (C) UE_LOG(LogDeepSweep, Log, TEXT("NET debit vers client : %.1f Ko/s (sortant), %.1f Ko/s (entrant)"), C->OutBytesPerSecond / 1024.f, C->InBytesPerSecond / 1024.f);
}

void ADSDiver::RunNetHostScenario(int32 Expected)
{
	static int32 Step = 0;
	static float StepT = 0.f, BwT = 0.f;
	UWorld* W = GetWorld();
	const float T = W->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	ADSGameState* GS = W->GetGameState<ADSGameState>();
	if (!PC || !GS || Step >= 99) return;

	TArray<ADSDiver*> Divers;
	for (TActorIterator<ADSDiver> It(W); It; ++It) Divers.Add(*It);
	ADSGiantPickup* Chest = nullptr;
	for (TActorIterator<ADSGiantPickup> It(W); It; ++It) if (It->GetCarry()->DesignCarriers == 2) Chest = *It;
	ADSCounter* Counter = nullptr;
	for (TActorIterator<ADSCounter> It(W); It; ++It) Counter = *It;
	ADSVacuum* Vac = nullptr;
	for (TActorIterator<ADSVacuum> It(W); It; ++It) Vac = *It;

	if (T > BwT && Step >= 1) { BwT = T + 3.f; LogBandwidth(W); }

	if (Step == 0)
	{
		if (GS->PlayerArray.Num() >= Expected && Divers.Num() >= Expected)
		{
			UE_LOG(LogDeepSweep, Log, TEXT("NET %d joueurs connectes"), GS->PlayerArray.Num());
			for (APlayerState* P : GS->PlayerArray) UE_LOG(LogDeepSweep, Log, TEXT("NET   - %s"), *P->GetPlayerName());
			StepT = T; Step = 1;
		}
		else if (T > 120.f) { UE_LOG(LogDeepSweep, Log, TEXT("NET timeout : %d/%d joueurs"), GS->PlayerArray.Num(), Expected); Step = 99; PC->ConsoleCommand(TEXT("quit")); }
		return;
	}
	// 1) Tous les plongeurs portent le coffre ensemble vers le comptoir
	if (Step == 1 && T > StepT + 3.f && Chest)
	{
		int32 k = 0;
		for (ADSDiver* D : Divers)
		{
			D->SetActorLocation(Chest->GetActorLocation() + FVector(-300.f, -150.f + 150.f * k++, 80.f));
			Chest->Interact(D); // le serveur fait autorite : equivalent a "attraper" de chacun
		}
		UE_LOG(LogDeepSweep, Log, TEXT("NET coffre : %d porteurs / %d requis, facteur vitesse %.2f"), Chest->GetCarry()->Carriers.Num(), Chest->GetCarry()->GetRequiredCarriers(), Chest->GetCarry()->GetSpeedFactor());
		StepT = T; Step = 2;
	}
	else if (Step == 2)
	{
		if (CarryingGiant && Counter)
		{
			const FVector To = (Counter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			PC->SetControlRotation(FRotator(-25.f, To.Rotation().Yaw + 35.f, 0.f));
			PendingIntent = To;
			bIntentThisFrame = true;
		}
		if (T > StepT + 4.f && T < StepT + 4.1f) PC->ConsoleCommand(TEXT("shot showui"));
		if (!Chest || T > StepT + 45.f)
		{
			UE_LOG(LogDeepSweep, Log, TEXT("NET coffre %s en %.1f s, credits=%d"), Chest ? TEXT("NON livre") : TEXT("livre"), T - StepT, GS->GetCredits());
			StepT = T; Step = 3;
		}
	}
	// 2) Aspiration d'un collegue a puissance 4
	else if (Step == 3 && T > StepT + 1.f && Vac && Vac->Nozzle)
	{
		ADSDiver* Victim = nullptr;
		for (ADSDiver* D : Divers) if (D != this) Victim = D;
		if (!Victim) { Step = 5; return; }
		SetActorLocation(FVector(0.f, 0.f, 300.f));
		if (Vac->Nozzle->Holder != this) Vac->Nozzle->Interact(this);
		Vac->MaxPower = 5;
		Vac->Power = 4;
		Victim->SetActorLocation(GetActorLocation() + GetActorForwardVector() * 450.f);
		Vac->Nozzle->bOn = true;
		StepT = T; Step = 4;
	}
	else if (Step == 4)
	{
		int32 Sucked = 0;
		if (const ADSPlayerState* PS = GetPlayerState<ADSPlayerState>()) Sucked = PS->StatSucked;
		if (Sucked > 0 || T > StepT + 8.f)
		{
			UE_LOG(LogDeepSweep, Log, TEXT("NET collegue aspire : %d"), Sucked);
			Vac->Nozzle->bOn = false;
			StepT = T; Step = 5;
		}
	}
	else if (Step == 5 && T > StepT + 3.f)
	{
		LogBandwidth(W);
		const UDSPickupSubsystem* Sub = W->GetSubsystem<UDSPickupSubsystem>();
		UE_LOG(LogDeepSweep, Log, TEXT("NET fin hote : credits=%d proprete=%.3f actifs=%d"), GS->GetCredits(), GS->GetCleanliness(), Sub ? Sub->NumActive() : -1);
		for (APlayerState* P : GS->PlayerArray) UE_LOG(LogDeepSweep, Log, TEXT("NET   pseudo final : %s"), *P->GetPlayerName());
		StepT = T; Step = 6;
	}
	else if (Step == 6 && T > StepT + 4.f) { Step = 99; PC->ConsoleCommand(TEXT("quit")); }
}

void ADSDiver::RunNetClientScenario()
{
	static int32 Step = 0;
	static bool bShot = false, bWasStunned = false;
	static float CarryT = -1.f, LogT = 0.f;
	UWorld* W = GetWorld();
	const float T = W->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	const ADSGameState* GS = W->GetGameState<ADSGameState>();
	if (!PC || !GS) return;
	const UDSPickupSubsystem* Sub = W->GetSubsystem<UDSPickupSubsystem>();
	const ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;

	if (Step == 0 && M && M->TotalItems > 0)
	{
		UE_LOG(LogDeepSweep, Log, TEXT("CLIENT connecte : %d objets dans la zone, %d dormants recus, %d joueurs"), M->TotalItems, M->NumDormant(), GS->PlayerArray.Num());
		Step = 1;
	}
	if (T > LogT) { LogT = T + 5.f; UE_LOG(LogDeepSweep, Log, TEXT("CLIENT etat : credits=%d proprete=%.3f actifs=%d"), GS->GetCredits(), GS->GetCleanliness(), Sub ? Sub->NumActive() : -1); }

	// Porte le coffre vers le comptoir des que le serveur l'a mis porteur (etat replique)
	if (CarryingGiant)
	{
		if (CarryT < 0.f) { CarryT = T; UE_LOG(LogDeepSweep, Log, TEXT("CLIENT porte le coffre")); }
		for (TActorIterator<ADSCounter> It(W); It; ++It)
		{
			const FVector To = (It->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			PC->SetControlRotation(FRotator(-25.f, To.Rotation().Yaw - 35.f, 0.f));
			PendingIntent = To;
			bIntentThisFrame = true;
		}
		if (!bShot && T > CarryT + 4.f) { PC->ConsoleCommand(TEXT("shot showui")); bShot = true; }
	}
	if (IsStunned() && !bWasStunned) { bWasStunned = true; UE_LOG(LogDeepSweep, Log, TEXT("CLIENT aspire et sonne !")); }
}
