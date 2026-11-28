#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DSHUD.generated.h"

class ADSDiver;
class ADSGameState;

/**
 * HUD de plongee dessine au Canvas (aucun asset) :
 * barre d'etat, panneaux des stations dans le monde, invites "[touche] action" au-dessus de ce
 * qu'on vise, objectifs guides avec fleches, bandeaux quand quelque chose bloque, ecran de fin.
 */
UCLASS()
class DEEPSWEEP_API ADSHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

private:
	float ShownCredits = 0.f;
	float S = 1.f;
	bool bPad = false;

	// Progression du tutoriel (locale, pour la partie en cours)
	bool bDoneGrab = false, bDoneSell = false, bDoneNozzle = false, bDoneSuck = false, bDoneDump = false, bDoneBuy = false;

	void Pill(float X, float Y, float W, float H, const FLinearColor& Col);
	void Label(const FString& Str, float X, float Y, const FLinearColor& Col, float Scale = 1.f, bool bCenter = false);
	float TextWidth(const FString& Str, float Scale);
	bool ToScreen(const FVector& World, FVector2D& Out) const;
	FString KeyName(TCHAR Key) const;
	/** "[F] texte" : badge de touche + texte, centre sur X. Renvoie la largeur. */
	float KeyPrompt(TCHAR Key, const FString& Text, float CX, float Y, float Scale, bool bCenter = true);
	/** Invite multi-segments ("Attraper : x  -  Action : y") convertie en badges. */
	void DrawPromptText(const FString& Prompt, float CX, float Y, float Scale);
	/** Repere au-dessus d'un lieu (ou fleche au bord de l'ecran s'il est hors champ). */
	void Marker(const FVector& World, const FString& Text, const FLinearColor& Col);

	void DrawTopBar(const ADSGameState* GS);
	void DrawStationLabels(const ADSDiver* Me);
	void DrawTargetPrompt(const ADSDiver* Me);
	void DrawObjectives(const ADSDiver* Me, const ADSGameState* GS);
	void DrawBlockers(const ADSDiver* Me);
	void DrawEndScreen();
};
