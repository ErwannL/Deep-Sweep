#include "UI/DSHUD.h"
#include "Core/DSGameState.h"
#include "Player/DSDiver.h"
#include "World/DSInteractable.h"
#include "Machines/DSVacuum.h"
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

void ADSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	const float S = FMath::Max(0.75f, Canvas->ClipY / 1080.f);
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!GS) return;

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

	// Aide contextuelle
	if (const ADSDiver* Diver = Cast<ADSDiver>(GetOwningPawn()))
	{
		FText Prompt;
		if (const ADSInteractable* I = ADSInteractable::FindBest(Diver)) Prompt = I->GetPrompt(Diver);
		if (!Prompt.IsEmpty())
			Label(Prompt.ToString(), Canvas->ClipX * 0.5f, Canvas->ClipY - 90 * S, FLinearColor(1.f, 1.f, 0.6f), 1.5f * S, true);
	}
}
