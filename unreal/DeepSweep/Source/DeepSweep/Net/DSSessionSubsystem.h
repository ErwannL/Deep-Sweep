#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/EngineBaseTypes.h"
#include "DSSessionSubsystem.generated.h"

class FOnlineSessionSearch;
class UNetDriver;

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
 * Steam possible plus tard sans changer ce code). Solo, heberger, chercher, rejoindre, rejoindre par IP.
 * Les erreurs reseau ramenent au menu avec un message lisible.
 */
UCLASS()
class DEEPSWEEP_API UDSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	FString GameMap = TEXT("/Game/Maps/L_Cove");
	FString MenuMap = TEXT("/Game/Maps/L_Menu");

	FString GetPlayerName() const;
	void SetPlayerName(const FString& Name);

	void PlaySolo();
	void HostLAN();
	void FindLAN();
	void JoinFound(int32 Index);
	void JoinIP(const FString& Address);
	void LeaveToMenu();

	const TArray<FDSFoundSession>& GetFound() const { return Found; }
	bool IsSearching() const { return bSearching; }
	bool IsBusy() const { return bBusy; }

	/** Message d'etat pour le menu (bStatusIsError = rouge). */
	FString Status;
	bool bStatusIsError = false;
	void SetStatus(const FString& S, bool bError = false);

	DECLARE_MULTICAST_DELEGATE(FOnChanged);
	FOnChanged OnChanged;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	IOnlineSessionPtr Sessions() const;
	FString NameOption() const;
	void OnCreateComplete(FName Session, bool bOk);
	void OnDestroyThenHost(FName Session, bool bOk);
	void OnFindComplete(bool bOk);
	void OnJoinComplete(FName Session, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);

	TSharedPtr<FOnlineSessionSearch> Search;
	TArray<FDSFoundSession> Found;
	bool bSearching = false;
	bool bBusy = false;
	bool bLeaving = false;
	FDelegateHandle CreateHandle, FindHandle, JoinHandle, DestroyHandle, NetFailHandle, TravelFailHandle;
};
