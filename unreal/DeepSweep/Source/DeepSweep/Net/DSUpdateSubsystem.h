#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DSUpdateSubsystem.generated.h"

UENUM()
enum class EDSUpdateState : uint8 { Checking, UpToDate, UpdateRequired, Offline };

/**
 * Verifie au lancement la derniere version publiee sur GitHub (releases/latest).
 * Si elle est plus recente que celle du jeu, le menu bloque les parties et propose le telechargement.
 */
UCLASS()
class DEEPSWEEP_API UDSUpdateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static constexpr const TCHAR* LatestApi = TEXT("https://api.github.com/repos/ErwannL/Deep-Sweep/releases/latest");

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	EDSUpdateState GetState() const { return State; }
	bool IsPlayBlocked() const { return State == EDSUpdateState::UpdateRequired; }
	FString GetLocalVersion() const;
	const FString& GetLatestVersion() const { return LatestVersion; }
	/** Lien direct vers l'installateur (ou la page de la release). */
	const FString& GetDownloadUrl() const { return DownloadUrl; }
	void OpenDownload() const;
	void Recheck();

	DECLARE_MULTICAST_DELEGATE(FOnChanged);
	FOnChanged OnChanged;

	/** "0.3.1" > "0.3.0" ? (tolere un "v" devant). */
	static bool IsNewer(const FString& Remote, const FString& Local);

private:
	EDSUpdateState State = EDSUpdateState::Checking;
	FString LatestVersion;
	FString DownloadUrl;
};
