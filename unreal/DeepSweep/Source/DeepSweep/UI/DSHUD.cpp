#include "UI/DSHUD.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "World/DSInteractable.h"
#include "World/DSCounter.h"
#include "Machines/DSVacuum.h"
#include "Machines/DSVacuumNozzle.h"
#include "Machines/DSVacuumControls.h"
#include "Machines/DSConveyor.h"
#include "Upgrades/DSUpgradePad.h"
#include "Events/DSOctopus.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Framework/Application/SlateApplication.h"

// ------------------------------------------------------------------ primitives

void ADSHUD::Pill(float X, float Y, float W, float H, const FLinearColor& Col) { DrawRect(Col, X, Y, W, H); }

float ADSHUD::TextWidth(const FString& Str, float Scale)
{
	float W = 0, H = 0;
	GetTextSize(Str, W, H, GEngine->GetLargeFont(), Scale);
	return W;
}

void ADSHUD::Label(const FString& Str, float X, float Y, const FLinearColor& Col, float Scale, bool bCenter)
{
	UFont* Font = GEngine->GetLargeFont();
	const float PX = bCenter ? X - TextWidth(Str, Scale) * 0.5f : X;
	DrawText(Str, FLinearColor(0, 0, 0, 0.65f * Col.A), PX + 2, Y + 2, Font, Scale);
	DrawText(Str, Col, PX, Y, Font, Scale);
}

bool ADSHUD::ToScreen(const FVector& World, FVector2D& Out) const
{
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->PlayerCameraManager) return false;
	const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
	if (FVector::DotProduct(World - Cam, PC->PlayerCameraManager->GetCameraRotation().Vector()) <= 10.f) return false;
	const FVector P = Canvas->Project(World);
	Out = FVector2D(P.X, P.Y);
	return Out.X > 0 && Out.Y > 0 && Out.X < Canvas->ClipX && Out.Y < Canvas->ClipY;
}

FString ADSHUD::KeyName(TCHAR Key) const
{
	if (!bPad) return FString::Chr(Key);
	return Key == 'F' ? TEXT("A") : Key == 'G' ? TEXT("X") : FString::Chr(Key);
}

float ADSHUD::KeyPrompt(TCHAR Key, const FString& Text, float CX, float Y, float Scale, bool bCenter)
{
	const FString K = KeyName(Key);
	const float KW = FMath::Max(34.f * Scale, TextWidth(K, 1.3f * Scale) + 16.f * Scale), KH = 34.f * Scale;
	const float TW = TextWidth(Text, 1.2f * Scale);
	const float Total = KW + 10.f * Scale + TW;
	float X = bCenter ? CX - Total * 0.5f : CX;
	Pill(X - 8 * Scale, Y - 6 * Scale, Total + 16 * Scale, KH + 12 * Scale, FLinearColor(0.f, 0.04f, 0.09f, 0.72f));
	Pill(X, Y, KW, KH, FLinearColor(1.f, 0.9f, 0.35f, 1.f));
	DrawText(K, FLinearColor(0.05f, 0.05f, 0.1f), X + (KW - TextWidth(K, 1.3f * Scale)) * 0.5f, Y + 2 * Scale, GEngine->GetLargeFont(), 1.3f * Scale);
	Label(Text, X + KW + 10.f * Scale, Y + 4 * Scale, FLinearColor::White, 1.2f * Scale);
	return Total;
}

void ADSHUD::DrawPromptText(const FString& Prompt, float CX, float Y, float Scale)
{
	// "Attraper : x" -> [F] x ; "Action : y" -> [G] y ; segments separes par "  -  "
	TArray<FString> Parts;
	Prompt.ParseIntoArray(Parts, TEXT("  -  "));
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		FString P = Parts[i].TrimStartAndEnd();
		TCHAR Key = 'F';
		if (P.RemoveFromStart(TEXT("Attraper : "))) Key = 'F';
		else if (P.RemoveFromStart(TEXT("Action : "))) Key = 'G';
		else if (P.StartsWith(TEXT("BOUCHE"))) { Key = 'G'; P = P.Replace(TEXT("BOUCHE ! Action : "), TEXT("Tuyau bouche ! ")); }
		KeyPrompt(Key, P, CX, Y + i * 50.f * Scale, Scale);
	}
}

void ADSHUD::Marker(const FVector& World, const FString& Text, const FLinearColor& Col)
{
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->PlayerCameraManager) return;
	FVector2D P;
	const float T = GetWorld()->GetTimeSeconds();
	if (ToScreen(World, P))
	{
		const float Bob = FMath::Sin(T * 5.f) * 6.f * S;
		// chevron vers le bas au-dessus du lieu
		for (int32 k = 0; k < 3; ++k)
		{
			DrawLine(P.X - 16 * S + k, P.Y - 30 * S + Bob, P.X, P.Y - 12 * S + Bob, Col, 3.f);
			DrawLine(P.X + 16 * S - k, P.Y - 30 * S + Bob, P.X, P.Y - 12 * S + Bob, Col, 3.f);
		}
		Label(Text, P.X, P.Y - 64 * S + Bob, Col, 1.15f * S, true);
		return;
	}
	// Hors champ : fleche au bord de l'ecran, orientee vers le lieu
	const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CR = PC->PlayerCameraManager->GetCameraRotation();
	const FVector Local = CR.UnrotateVector(World - Cam);
	FVector2D Dir(Local.Y, -Local.Z);
	if (Dir.IsNearlyZero()) Dir = FVector2D(0, 1);
	Dir.Normalize();
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float M = 70.f * S;
	const float Kx = (C.X - M) / FMath::Max(0.001f, FMath::Abs(Dir.X)), Ky = (C.Y - M) / FMath::Max(0.001f, FMath::Abs(Dir.Y));
	const FVector2D E = C + Dir * FMath::Min(Kx, Ky);
	const FVector2D Side(-Dir.Y, Dir.X);
	const FVector2D Tip = E + Dir * 26.f * S, B1 = E - Dir * 10.f * S + Side * 18.f * S, B2 = E - Dir * 10.f * S - Side * 18.f * S;
	for (int32 k = 0; k < 4; ++k)
	{
		DrawLine(Tip.X, Tip.Y, B1.X - Dir.X * k, B1.Y - Dir.Y * k, Col, 3.f);
		DrawLine(Tip.X, Tip.Y, B2.X - Dir.X * k, B2.Y - Dir.Y * k, Col, 3.f);
	}
	Label(Text, FMath::Clamp(E.X - Dir.X * 40.f * S, 140.f * S, Canvas->ClipX - 140.f * S), FMath::Clamp(E.Y - Dir.Y * 40.f * S, 80.f * S, Canvas->ClipY - 80.f * S), Col, 1.f * S, true);
}

// ------------------------------------------------------------------ elements

void ADSHUD::DrawTopBar(const ADSGameState* GS)
{
	const FLinearColor Bg(0, 0, 0, 0.45f);
	float X = 20 * S;
	const float Y = 18 * S, H = 44 * S;
	ShownCredits = FMath::FInterpTo(ShownCredits, static_cast<float>(GS->GetCredits()), GetWorld()->GetDeltaSeconds(), 8.f);
	if (FMath::Abs(ShownCredits - GS->GetCredits()) < 0.5f) ShownCredits = GS->GetCredits();
	Pill(X, Y, 200 * S, H, Bg);
	Label(FString::Printf(TEXT("%d credits"), FMath::RoundToInt(ShownCredits)), X + 14 * S, Y + 8 * S, FLinearColor(1.f, 0.85f, 0.25f), 1.3f * S);
	X += 215 * S;
	const float Clean = GS->GetCleanliness();
	Pill(X, Y, 360 * S, H, Bg);
	Label(TEXT("Proprete"), X + 14 * S, Y + 8 * S, FLinearColor::White, 1.2f * S);
	const float BX = X + 135 * S, BW = 150 * S;
	DrawRect(FLinearColor(1, 1, 1, 0.2f), BX, Y + 16 * S, BW, 12 * S);
	DrawRect(FLinearColor(0.37f, 0.94f, 0.63f), BX, Y + 16 * S, BW * Clean, 12 * S);
	Label(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Clean * 100.f)), BX + BW + 10 * S, Y + 8 * S, FLinearColor::White, 1.2f * S);
	X += 375 * S;
	const int32 T = FMath::FloorToInt(GS->GetElapsedTime());
	Pill(X, Y, 110 * S, H, Bg);
	Label(FString::Printf(TEXT("%d:%02d"), T / 60, T % 60), X + 16 * S, Y + 8 * S, FLinearColor::White, 1.3f * S);
	X += 125 * S;
	for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It)
	{
		const ADSVacuum* V = *It;
		Pill(X, Y, 330 * S, H, Bg);
		const FLinearColor C = V->Clog ? FLinearColor(1.f, 0.5f, 0.2f) : V->IsFull() ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor::White;
		Label(FString::Printf(TEXT("Aspirateur P%d/%d  Cuve %d/%d%s"), V->Power, V->MaxPower, V->TankCount, V->Capacity, V->Clog ? TEXT(" BOUCHE") : TEXT("")), X + 14 * S, Y + 8 * S, C, 1.2f * S);
		break;
	}
	float Age = 0.f;
	const FText Toast = GS->GetToast(Age);
	if (!Toast.IsEmpty() && Age < 3.f)
		Label(Toast.ToString(), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.17f, FLinearColor(1.f, 1.f, 1.f, FMath::Clamp((3.f - Age) * 2.f, 0.f, 1.f)), 1.9f * S, true);
	Label(FString::Printf(TEXT("Echap : menu    %s : attraper / utiliser    %s : lancer / action    Espace / Ctrl : monter / descendre"), *KeyName('F'), *KeyName('G')),
		20 * S, Canvas->ClipY - 40 * S, FLinearColor(1, 1, 1, 0.55f), 0.95f * S);
}

void ADSHUD::DrawStationLabels(const ADSDiver* Me)
{
	// Panneaux flottants des stations (comptoir, buse, console, levier, boutique, objets geants...)
	for (TActorIterator<ADSInteractable> It(GetWorld()); It; ++It)
	{
		const FString Txt = It->GetStationLabel().ToString();
		if (Txt.IsEmpty()) continue;
		const float Dist = FVector::Dist(Me->GetActorLocation(), It->GetActorLocation());
		if (Dist > 4500.f) continue;
		FVector2D P;
		if (!ToScreen(It->GetActorLocation() + FVector(0, 0, It->LabelHeight), P)) continue;
		const float K = FMath::Clamp(1.5f - Dist / 4000.f, 0.85f, 1.3f) * S;
		const float W = TextWidth(Txt, 1.1f * K) + 24 * K;
		Pill(P.X - W * 0.5f, P.Y - 6 * K, W, 34 * K, It->LabelColor.CopyWithNewOpacity(0.75f));
		Label(Txt, P.X, P.Y, FLinearColor::White, 1.1f * K, true);
	}
}

void ADSHUD::DrawTargetPrompt(const ADSDiver* Me)
{
	// 1) Element de la base ou machine a portee
	if (const ADSInteractable* I = ADSInteractable::FindBest(Me))
	{
		const FString Prompt = I->GetPrompt(Me).ToString();
		if (!Prompt.IsEmpty())
		{
			FVector2D P;
			const FVector Anchor = I->IsA<ADSConveyor>() ? Me->GetActorLocation() + FVector(0, 0, 120.f) : I->GetActorLocation() + FVector(0, 0, I->LabelHeight * 0.6f);
			if (ToScreen(Anchor, P)) DrawPromptText(Prompt, P.X, P.Y + 30 * S, 1.f * S);
			else DrawPromptText(Prompt, Canvas->ClipX * 0.5f, Canvas->ClipY - 140 * S, 1.f * S);
			return;
		}
	}
	// 2) Dechet vise : "[F] Ramasser Canette (+1 cr)"
	if (const ADSPickup* P = Me->GetHoverTarget())
	{
		FVector2D SP;
		if (P->GetType() && ToScreen(P->GetActorLocation() + FVector(0, 0, P->GetType()->Radius + 40.f), SP))
			KeyPrompt('F', FString::Printf(TEXT("Ramasser %s  (+%d cr)"), *P->GetType()->DisplayName.ToString(), P->GetType()->Value), SP.X, SP.Y - 50 * S, 1.f * S);
		return;
	}
	// 3) Objets en main : quoi faire ensuite
	if (Me->GetGrab() && Me->GetGrab()->NumHeld() > 0)
	{
		const float Y = Canvas->ClipY - 150 * S;
		KeyPrompt('G', TEXT("Lancer"), Canvas->ClipX * 0.5f - 230 * S, Y, 1.f * S, false);
		KeyPrompt('F', Me->GetGrab()->IsFull() ? TEXT("Lacher (sac plein !)") : TEXT("Lacher / ramasser"), Canvas->ClipX * 0.5f + 20 * S, Y, 1.f * S, false);
		for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It) Marker(It->GetActorLocation() + FVector(0, 0, 320.f), TEXT("Vends ici"), FLinearColor(0.3f, 1.f, 0.5f));
	}
	else if (Me->GetHeldNozzle())
	{
		KeyPrompt('G', TEXT("Aspirer / arreter"), Canvas->ClipX * 0.5f - 200 * S, Canvas->ClipY - 150 * S, 1.f * S, false);
		KeyPrompt('F', TEXT("Poser la buse"), Canvas->ClipX * 0.5f + 60 * S, Canvas->ClipY - 150 * S, 1.f * S, false);
	}
}

void ADSHUD::DrawObjectives(const ADSDiver* Me, const ADSGameState* GS)
{
	ADSVacuum* Vac = nullptr; ADSCounter* Counter = nullptr; ADSConveyor* Belt = nullptr; ADSVacuumLever* Lever = nullptr; ADSUpgradePad* Pad = nullptr;
	for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It) Vac = *It;
	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It) Counter = *It;
	for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It) Belt = *It;
	for (TActorIterator<ADSVacuumLever> It(GetWorld()); It; ++It) Lever = *It;
	for (TActorIterator<ADSUpgradePad> It(GetWorld()); It; ++It) if (!Pad || It->Cost < Pad->Cost) Pad = *It;

	bDoneGrab |= Me->GetGrab() && Me->GetGrab()->NumHeld() > 0;
	bDoneSell |= GS->GetCredits() > 0;
	bDoneNozzle |= Me->GetHeldNozzle() != nullptr;
	bDoneSuck |= Vac && Vac->TankCount > 0;
	bDoneDump |= Belt && Belt->NumItems() > 0;
	bDoneBuy |= GS->HasUpgrade(EDSUpgrade::Bag) || GS->HasUpgrade(EDSUpgrade::Claw) || GS->HasUpgrade(EDSUpgrade::Turbo) || GS->HasUpgrade(EDSUpgrade::FastBelt) || GS->HasUpgrade(EDSUpgrade::Balloons);

	struct FObj { bool bDone; FString Text; AActor* Target; FString Mark; };
	const FObj Objs[] = {
		{ bDoneGrab, FString::Printf(TEXT("Ramasse un dechet  [%s]"), *KeyName('F')), nullptr, TEXT("") },
		{ bDoneSell, TEXT("Vends-le au COMPTOIR vert"), Counter, TEXT("Comptoir") },
		{ bDoneNozzle, FString::Printf(TEXT("Prends la buse de l'aspirateur rouge  [%s]"), *KeyName('F')), Vac ? Vac->Nozzle.Get() : nullptr, TEXT("Buse") },
		{ bDoneSuck, FString::Printf(TEXT("Aspire des dechets  [%s]"), *KeyName('G')), nullptr, TEXT("") },
		{ bDoneDump, FString::Printf(TEXT("Vide la cuve au levier jaune  [%s]"), *KeyName('F')), Lever, TEXT("Levier") },
		{ bDoneBuy, TEXT("Achete une amelioration (pads violets)"), Pad, TEXT("Boutique") },
	};
	const float X = 20 * S;
	float Y = 78 * S;
	int32 Shown = 0;
	bool bMarked = false;
	Pill(X, Y, 470 * S, 36 * S, FLinearColor(0.f, 0.05f, 0.1f, 0.7f));
	Label(TEXT("OBJECTIFS"), X + 12 * S, Y + 6 * S, FLinearColor(0.4f, 0.95f, 1.f), 1.15f * S);
	Y += 40 * S;
	for (const FObj& O : Objs)
	{
		if (O.bDone || Shown >= 3) continue;
		Pill(X, Y, 470 * S, 32 * S, FLinearColor(0.f, 0.05f, 0.1f, Shown == 0 ? 0.75f : 0.5f));
		Label((Shown == 0 ? TEXT("> ") : TEXT("   ")) + O.Text, X + 12 * S, Y + 5 * S, Shown == 0 ? FLinearColor(1.f, 0.95f, 0.55f) : FLinearColor(1, 1, 1, 0.75f), 1.05f * S);
		if (Shown == 0 && O.Target && !bMarked) { Marker(O.Target->GetActorLocation() + FVector(0, 0, 260.f), O.Mark, FLinearColor(1.f, 0.9f, 0.35f)); bMarked = true; }
		Y += 36 * S;
		++Shown;
	}
	if (Shown == 0)
	{
		Pill(X, Y, 470 * S, 32 * S, FLinearColor(0.f, 0.05f, 0.1f, 0.75f));
		Label(FString::Printf(TEXT("> Nettoie la crique a 100 %%  (%d%%)"), FMath::FloorToInt(GS->GetCleanliness() * 100.f)), X + 12 * S, Y + 5 * S, FLinearColor(1.f, 0.95f, 0.55f), 1.05f * S);
	}
}

void ADSHUD::DrawBlockers(const ADSDiver* Me)
{
	// Bandeaux quand quelque chose bloque, avec la marche a suivre et une fleche vers l'endroit.
	float Y = Canvas->ClipY * 0.26f;
	auto Banner = [&](const FString& Txt, const FLinearColor& Col)
	{
		const float W = TextWidth(Txt, 1.35f * S) + 40 * S;
		Pill(Canvas->ClipX * 0.5f - W * 0.5f, Y - 6 * S, W, 44 * S, Col.CopyWithNewOpacity(0.85f));
		Label(Txt, Canvas->ClipX * 0.5f, Y, FLinearColor::White, 1.35f * S, true);
		Y += 54 * S;
	};
	const float Blink = 0.75f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f);
	for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It)
	{
		ADSVacuum* V = *It;
		if (V->Clog > 0)
		{
			const bool bMine = V->Nozzle && V->Nozzle->Holder == Me;
			Banner(bMine ? FString::Printf(TEXT("TUYAU BOUCHE par un caddie : appuie %d fois sur [%s] pour taper dessus"), V->Clog, *KeyName('G'))
			             : TEXT("TUYAU BOUCHE : celui qui tient la buse doit taper dessus [G]"), FLinearColor(0.9f * Blink, 0.35f, 0.05f));
		}
		if (V->IsFull())
		{
			Banner(FString::Printf(TEXT("CUVE PLEINE : tire le LEVIER jaune [%s] (ou attends Bob)"), *KeyName('F')), FLinearColor(0.85f * Blink, 0.15f, 0.15f));
			if (V->LeverActor) Marker(V->LeverActor->GetActorLocation() + FVector(0, 0, 250.f), TEXT("Levier"), FLinearColor(1.f, 0.4f, 0.3f));
		}
	}
	for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It)
		if (It->bJammed)
		{
			Banner(FString::Printf(TEXT("TAPIS BLOQUE par un caddie : va sur le tapis et appuie sur [%s]"), *KeyName('F')), FLinearColor(0.85f * Blink, 0.4f, 0.05f));
			Marker(It->GetJamWorldLocation() + FVector(0, 0, 200.f), TEXT("Debloquer ici"), FLinearColor(1.f, 0.6f, 0.2f));
		}
	for (TActorIterator<ADSOctopus> It(GetWorld()); It; ++It)
		if (It->State != EDSOctoState::Idle)
		{
			Banner(FString::Printf(TEXT("POULPE VOLEUR : rattrape-le et appuie sur [%s] !"), *KeyName('F')), FLinearColor(0.55f * Blink, 0.2f, 0.7f));
			Marker(It->GetActorLocation() + FVector(0, 0, 250.f), TEXT("Poulpe"), FLinearColor(0.85f, 0.5f, 1.f));
		}
}

void ADSHUD::DrawEndScreen()
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
	KeyPrompt('F', TEXT("Replonger"), W * 0.5f, H - 120 * S, 1.2f * S);
}

// ------------------------------------------------------------------

void ADSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	S = FMath::Max(0.75f, Canvas->ClipY / 1080.f);
	bPad = FSlateApplication::IsInitialized() && FSlateApplication::Get().IsGamepadAttached();
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!GS) return;
	if (GS->IsMatchOver()) { DrawEndScreen(); return; }

	const ADSDiver* Me = Cast<ADSDiver>(GetOwningPawn());
	if (Me) DrawStationLabels(Me);
	DrawTopBar(GS);
	if (!Me) return;
	DrawObjectives(Me, GS);
	DrawBlockers(Me);
	DrawTargetPrompt(Me);
}
