#include "UI/DSHUD.h"
#include "Core/DSGameState.h"
#include "Player/DSDiver.h"
#include "World/DSInteractable.h"
#include "Machines/DSVacuum.h"
#include "Core/DSPlayerState.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

void ADSHUD::Pill(float X, float Y, float W, float H, const FLinearColor& Col)
{
	DrawRect(Col, X, Y, W, H);
}

void ADSHUD::Label(const FString& S, float X, float Y, const FLinearColor& Col, float Scale, bool bCenter)
{
	UFont* Font = GEngine->GetLargeFont();
	float W = 0, H = 0;
	GetTextSize(S, W, H, Font, Scale);
	const float PX = bCenter ? X - W * 0.5f : X;
	DrawText(S, FLinearColor(0, 0, 0, 0.6f), PX + 2, Y + 2, Font, Scale);
	DrawText(S, Col, PX, Y, Font, Scale);
}

void ADSHUD::DrawEndScreen(float S)
{
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawRect(FLinearColor(0.01f, 0.12f, 0.23f, 0.82f), 0, 0, W, H);
	float Y = H * 0.16f;
	Label(TEXT("Crique nettoyee !"), W * 0.5f, Y, FLinearColor(0.5f, 1.f, 0.75f), 3.2f * S, true);
	Y += 110 * S;
	const int32 T = FMath::FloorToInt(GS->GetFinalTime());
	Label(FString::Printf(TEXT("Temps : %d min %02d s   -   %d credits en banque"), T / 60, T % 60, GS->GetCredits()), W * 0.5f, Y, FLinearColor::White, 1.6f * S, true);
	Y += 80 * S;

	// Tableau par joueur
	TArray<const ADSPlayerState*> Players;
	for (APlayerState* PS : GS->PlayerArray) if (const ADSPlayerState* D = Cast<ADSPlayerState>(PS)) Players.Add(D);
	const float C0 = W * 0.5f - 430 * S;
	Label(TEXT("Plongeur"), C0, Y, FLinearColor(1, 1, 1, 0.6f), 1.3f * S);
	Label(TEXT("Rapporte"), C0 + 300 * S, Y, FLinearColor(1, 1, 1, 0.6f), 1.3f * S);
	Label(TEXT("Collegues aspires"), C0 + 480 * S, Y, FLinearColor(1, 1, 1, 0.6f), 1.3f * S);
	Label(TEXT("Vases casses"), C0 + 720 * S, Y, FLinearColor(1, 1, 1, 0.6f), 1.3f * S);
	Y += 50 * S;
	for (int32 i = 0; i < Players.Num(); ++i)
	{
		const ADSPlayerState* P = Players[i];
		Label(FString::Printf(TEXT("J%d  %s"), i + 1, *P->GetPlayerName().Left(14)), C0, Y, ADSDiver::GetPlayerColor(i), 1.4f * S);
		Label(FString::Printf(TEXT("%d"), P->StatSold), C0 + 300 * S, Y, FLinearColor(1.f, 0.85f, 0.25f), 1.4f * S);
		Label(FString::Printf(TEXT("%d"), P->StatSucked), C0 + 480 * S, Y, FLinearColor::White, 1.4f * S);
		Label(FString::Printf(TEXT("%d"), P->StatBroken), C0 + 720 * S, Y, FLinearColor::White, 1.4f * S);
		Y += 46 * S;
	}
	Y += 30 * S;

	// Titres droles (GDD 6.4)
	auto Best = [&](TFunctionRef<int32(const ADSPlayerState*)> F) -> int32
	{
		int32 B = INDEX_NONE, BV = 0;
		for (int32 i = 0; i < Players.Num(); ++i) if (F(Players[i]) > BV) { BV = F(Players[i]); B = i; }
		return B;
	};
	TArray<FString> Titles;
	if (Players.Num() > 1)
		if (const int32 B = Best([](const ADSPlayerState* P) { return P->StatSold; }); B != INDEX_NONE) Titles.Add(FString::Printf(TEXT("Employe du mois : J%d"), B + 1));
	if (const int32 B = Best([](const ADSPlayerState* P) { return P->StatSucked; }); B != INDEX_NONE) Titles.Add(FString::Printf(TEXT("Recycleur en chef : J%d"), B + 1));
	if (const int32 B = Best([](const ADSPlayerState* P) { return P->StatBroken; }); B != INDEX_NONE) Titles.Add(FString::Printf(TEXT("Main de velours (pas vraiment) : J%d"), B + 1));
	if (Titles.Num() == 0) Titles.Add(TEXT("M. Abysso est... presque satisfait."));
	for (const FString& Tt : Titles) { Label(Tt, W * 0.5f, Y, FLinearColor(1.f, 0.9f, 0.4f), 1.5f * S, true); Y += 50 * S; }

	Label(TEXT("Attraper (F / A) : replonger"), W * 0.5f, H - 110 * S, FLinearColor(1, 1, 1, 0.8f), 1.3f * S, true);
}

void ADSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	const float S = FMath::Max(0.75f, Canvas->ClipY / 1080.f);
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!GS) return;

	if (GS->IsMatchOver()) { DrawEndScreen(S); return; }
	const FLinearColor Bg(0, 0, 0, 0.45f);
	float X = 20 * S;
	const float Y = 18 * S, H = 44 * S;

	// Credits : le compteur "roule" vers la vraie valeur (GDD 13)
	ShownCredits = FMath::FInterpTo(ShownCredits, static_cast<float>(GS->GetCredits()), GetWorld()->GetDeltaSeconds(), 8.f);
	if (FMath::Abs(ShownCredits - GS->GetCredits()) < 0.5f) ShownCredits = GS->GetCredits();
	Pill(X, Y, 200 * S, H, Bg);
	Label(FString::Printf(TEXT("%d credits"), FMath::RoundToInt(ShownCredits)), X + 14 * S, Y + 8 * S, FLinearColor(1.f, 0.85f, 0.25f), 1.3f * S);
	X += 215 * S;

	// Proprete
	const float Clean = GS->GetCleanliness();
	Pill(X, Y, 360 * S, H, Bg);
	Label(TEXT("Proprete"), X + 14 * S, Y + 8 * S, FLinearColor::White, 1.2f * S);
	const float BX = X + 135 * S, BW = 150 * S;
	DrawRect(FLinearColor(1, 1, 1, 0.2f), BX, Y + 16 * S, BW, 12 * S);
	DrawRect(FLinearColor(0.37f, 0.94f, 0.63f), BX, Y + 16 * S, BW * Clean, 12 * S);
	Label(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Clean * 100.f)), BX + BW + 10 * S, Y + 8 * S, FLinearColor::White, 1.2f * S);
	X += 375 * S;

	// Chrono
	const int32 T = FMath::FloorToInt(GS->GetElapsedTime());
	Pill(X, Y, 110 * S, H, Bg);
	Label(FString::Printf(TEXT("%d:%02d"), T / 60, T % 60), X + 16 * S, Y + 8 * S, FLinearColor::White, 1.3f * S);

	X += 125 * S;
	// Aspirateur
	for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It)
	{
		const ADSVacuum* V = *It;
		Pill(X, Y, 330 * S, H, Bg);
		const FLinearColor C = V->Clog ? FLinearColor(1.f, 0.5f, 0.2f) : V->IsFull() ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor::White;
		Label(FString::Printf(TEXT("Aspirateur P%d/%d  Cuve %d/%d%s"), V->Power, V->MaxPower, V->TankCount, V->Capacity, V->Clog ? TEXT(" BOUCHE") : TEXT("")),
			X + 14 * S, Y + 8 * S, C, 1.2f * S);
		break;
	}

	// Message (toast)
	float Age = 0.f;
	const FText Toast = GS->GetToast(Age);
	if (!Toast.IsEmpty() && Age < 2.8f)
		Label(Toast.ToString(), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.2f, FLinearColor(1.f, 1.f, 1.f, FMath::Clamp((2.8f - Age) * 2.f, 0.f, 1.f)), 1.9f * S, true);

	// Rappel permanent des touches + aide plus grande les 15 premieres secondes
	Label(TEXT("Echap : menu    F : attraper / utiliser    G : lancer / action    Espace / Ctrl : monter / descendre"), 20 * S, Canvas->ClipY - 40 * S, FLinearColor(1, 1, 1, 0.55f), 0.95f * S);
	if (GS->GetElapsedTime() < 15.f)
	{
		const float CX = Canvas->ClipX - 480 * S, CY = 90 * S;
		DrawRect(FLinearColor(0.f, 0.05f, 0.1f, 0.7f), CX, CY, 450 * S, 250 * S);
		Label(TEXT("Nettoyez la crique a 100 % !"), CX + 20 * S, CY + 14 * S, FLinearColor(0.4f, 0.95f, 1.f), 1.3f * S);
		const TCHAR* Lines[] = { TEXT("ZQSD/WASD : nager   souris : regarder"), TEXT("F : attraper un dechet, utiliser une machine"),
			TEXT("G : lancer (ou action de l'aspirateur)"), TEXT("Vendez au COMPTOIR vert, achetez sur les pads violets"), TEXT("Echap : menu (options, quitter)") };
		for (int32 i = 0; i < 5; ++i) Label(Lines[i], CX + 20 * S, CY + (62 + i * 36) * S, FLinearColor::White, 1.f * S);
	}

	// Aide contextuelle
	if (const ADSDiver* Diver = Cast<ADSDiver>(GetOwningPawn()))
	{
		FText Prompt;
		if (const ADSInteractable* I = ADSInteractable::FindBest(Diver)) Prompt = I->GetPrompt(Diver);
		if (!Prompt.IsEmpty())
			Label(Prompt.ToString(), Canvas->ClipX * 0.5f, Canvas->ClipY - 90 * S, FLinearColor(1.f, 1.f, 0.6f), 1.5f * S, true);
	}
}
