#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "DSSessionSubsystem.generated.h"

class FOnlineSessionSearch;

/** Une partie trouvee sur le reseau local. */
struct FDSFoundSession
{
	FString HostName;
	int32 OpenSlots = 0;
	int32 MaxPlayers = 4;
	int32 PingMs = 0;
};

/**
 * Sessions listen server (GDD 14) via l'Online Subsystem configure (Null = LAN par defaut ;
 * Steam possible plus tard sans changer ce code). Heberger, chercher, rejoindre, rejoindre par IP.
 */
UCLASS()
class DEEPSWEEP_API UDSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/** Pseudo du joueur (passe dans l'URL : ?Name=...). */
	FString PlayerName = TEXT("Plongeur");
	FString GameMap = TEXT("/Game/Maps/L_Cove");
	FString MenuMap = TEXT("/Game/Maps/L_Menu");

	void HostLAN();
	void FindLAN();
	void JoinFound(int32 Index);
	void JoinIP(const FString& Address);
	void LeaveToMenu();

	const TArray<FDSFoundSession>& GetFound() const { return Found; }
	bool IsSearching() const { return bSearching; }
	/** Message d'etat pour le menu. */
	FString Status;

	DECLARE_MULTICAST_DELEGATE(FOnChanged);
	FOnChanged OnChanged;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	IOnlineSessionPtr Sessions() const;
	FString NameOption() const;
	void OnCreateComplete(FName Session, bool bOk);
	void OnDestroyThenHost(FName Session, bool bOk);
	void OnFindComplete(bool bOk);
	void OnJoinComplete(FName Session, EOnJoinSessionCompleteResult::Type Result);

	TSharedPtr<FOnlineSessionSearch> Search;
	TArray<FDSFoundSession> Found;
	bool bSearching = false;
	FDelegateHandle CreateHandle, FindHandle, JoinHandle, DestroyHandle;
};
