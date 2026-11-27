#include "Net/DSSessionSubsystem.h"
#include "Core/DSUserSettings.h"
#include "DeepSweep.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

static const FName GDSSession = NAME_GameSession;

void UDSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UDSUserSettings>();
	Super::Initialize(Collection);
	if (GEngine)
	{
		NetFailHandle = GEngine->OnNetworkFailure().AddUObject(this, &UDSSessionSubsystem::OnNetworkFailure);
		TravelFailHandle = GEngine->OnTravelFailure().AddUObject(this, &UDSSessionSubsystem::OnTravelFailure);
	}
}

void UDSSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetFailHandle);
		GEngine->OnTravelFailure().Remove(TravelFailHandle);
	}
	Super::Deinitialize();
}

FString UDSSessionSubsystem::GetPlayerName() const
{
	const UDSUserSettings* S = GetGameInstance()->GetSubsystem<UDSUserSettings>();
	return S ? S->PlayerName : FString(TEXT("Plongeur"));
}

void UDSSessionSubsystem::SetPlayerName(const FString& Name)
{
	FString N = Name.TrimStartAndEnd().Left(16);
	if (N.IsEmpty()) return;
	if (UDSUserSettings* S = GetGameInstance()->GetSubsystem<UDSUserSettings>()) { S->PlayerName = N; S->Save(); }
}

void UDSSessionSubsystem::SetStatus(const FString& S, bool bError)
{
	Status = S;
	bStatusIsError = bError;
	OnChanged.Broadcast();
}

IOnlineSessionPtr UDSSessionSubsystem::Sessions() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

FString UDSSessionSubsystem::NameOption() const
{
	FString Clean = GetPlayerName().Left(16).Replace(TEXT("?"), TEXT("")).Replace(TEXT("="), TEXT("")).Replace(TEXT(" "), TEXT("_"));
	return TEXT("?Name=") + (Clean.IsEmpty() ? FString(TEXT("Plongeur")) : Clean);
}

void UDSSessionSubsystem::PlaySolo()
{
	bLeaving = false;
	SetStatus(TEXT("Plongee en solo..."));
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GameMap), true, NameOption().Mid(1));
}

void UDSSessionSubsystem::HostLAN()
{
	bLeaving = false;
	IOnlineSessionPtr S = Sessions();
	if (!S.IsValid())
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName(*GameMap), true, TEXT("listen") + NameOption());
		return;
	}
	if (S->GetNamedSession(GDSSession))
	{
		DestroyHandle = S->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnDestroyThenHost));
		S->DestroySession(GDSSession);
		return;
	}
	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = true;
	Settings.NumPublicConnections = 4;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = false;
	Settings.bUseLobbiesIfAvailable = false;
	Settings.Set(FName(TEXT("HOSTNAME")), GetPlayerName(), EOnlineDataAdvertisementType::ViaOnlineService);
	CreateHandle = S->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnCreateComplete));
	bBusy = true;
	SetStatus(TEXT("Creation de la partie..."));
	S->CreateSession(0, GDSSession, Settings);
}

void UDSSessionSubsystem::OnDestroyThenHost(FName, bool)
{
	if (IOnlineSessionPtr S = Sessions()) S->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	HostLAN();
}

void UDSSessionSubsystem::OnCreateComplete(FName, bool bOk)
{
	if (IOnlineSessionPtr S = Sessions()) S->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	bBusy = false;
	UE_LOG(LogDeepSweep, Log, TEXT("Session LAN creee : %d"), bOk);
	if (!bOk) { SetStatus(TEXT("Impossible de creer la partie."), true); return; }
	SetStatus(TEXT("Partie creee, ouverture de la crique..."));
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GameMap), true, TEXT("listen") + NameOption());
}

void UDSSessionSubsystem::FindLAN()
{
	IOnlineSessionPtr S = Sessions();
	if (!S.IsValid() || bSearching) return;
	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = true;
	Search->MaxSearchResults = 20;
	Search->PingBucketSize = 50;
	FindHandle = S->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnFindComplete));
	bSearching = true;
	Found.Reset();
	SetStatus(TEXT("Recherche des parties sur le reseau..."));
	S->FindSessions(0, Search.ToSharedRef());
}

void UDSSessionSubsystem::OnFindComplete(bool bOk)
{
	if (IOnlineSessionPtr S = Sessions()) S->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	bSearching = false;
	Found.Reset();
	if (Search.IsValid())
		for (const FOnlineSessionSearchResult& R : Search->SearchResults)
		{
			FDSFoundSession F;
			R.Session.SessionSettings.Get(FName(TEXT("HOSTNAME")), F.HostName);
			if (F.HostName.IsEmpty()) F.HostName = TEXT("Partie");
			F.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			F.OpenSlots = R.Session.NumOpenPublicConnections;
			F.PingMs = R.PingInMs;
			Found.Add(F);
		}
	UE_LOG(LogDeepSweep, Log, TEXT("Recherche LAN : %d resultat(s)"), Found.Num());
	SetStatus(Found.Num() ? FString::Printf(TEXT("%d partie(s) trouvee(s)"), Found.Num())
	                      : TEXT("Aucune partie trouvee. Avec Hamachi, rejoins plutot par IP (25.x.x.x)."));
}

void UDSSessionSubsystem::JoinFound(int32 Index)
{
	IOnlineSessionPtr S = Sessions();
	if (!S.IsValid() || !Search.IsValid() || !Search->SearchResults.IsValidIndex(Index)) return;
	bLeaving = false;
	JoinHandle = S->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnJoinComplete));
	bBusy = true;
	SetStatus(TEXT("Connexion..."));
	S->JoinSession(0, GDSSession, Search->SearchResults[Index]);
}

void UDSSessionSubsystem::OnJoinComplete(FName Session, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr S = Sessions();
	if (S.IsValid()) S->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	bBusy = false;
	FString Url;
	if (Result != EOnJoinSessionCompleteResult::Success || !S.IsValid() || !S->GetResolvedConnectString(Session, Url))
	{
		SetStatus(TEXT("Impossible de rejoindre cette partie."), true);
		return;
	}
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		PC->ClientTravel(Url + NameOption(), TRAVEL_Absolute);
}

void UDSSessionSubsystem::JoinIP(const FString& Address)
{
	FString A = Address.TrimStartAndEnd();
	if (A.IsEmpty()) { SetStatus(TEXT("Saisis l'IP de l'hote (ex. 25.12.34.56 avec Hamachi)."), true); return; }
	if (UDSUserSettings* Set = GetGameInstance()->GetSubsystem<UDSUserSettings>()) { Set->LastJoinAddress = A; Set->Save(); }
	bLeaving = false;
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		SetStatus(FString::Printf(TEXT("Connexion a %s..."), *A));
		PC->ClientTravel(A + NameOption(), TRAVEL_Absolute);
	}
}

void UDSSessionSubsystem::LeaveToMenu()
{
	bLeaving = true;
	if (IOnlineSessionPtr S = Sessions()) S->DestroySession(GDSSession);
	SetStatus(TEXT(""));
	UGameplayStatics::OpenLevel(GetWorld(), FName(*MenuMap));
}

void UDSSessionSubsystem::OnNetworkFailure(UWorld*, UNetDriver*, ENetworkFailure::Type Type, const FString& Error)
{
	UE_LOG(LogDeepSweep, Warning, TEXT("Echec reseau %s : %s"), ENetworkFailure::ToString(Type), *Error);
	if (bLeaving) return;
	bBusy = false;
	if (IOnlineSessionPtr S = Sessions()) S->DestroySession(GDSSession);
	FString Msg;
	switch (Type)
	{
	case ENetworkFailure::ConnectionLost:
	case ENetworkFailure::ConnectionTimeout: Msg = TEXT("Connexion perdue avec l'hote (partie terminee ou reseau coupe)."); break;
	case ENetworkFailure::PendingConnectionFailure: Msg = TEXT("Impossible de se connecter : verifie l'IP, Hamachi et le pare-feu de l'hote (port UDP 7777)."); break;
	case ENetworkFailure::OutdatedClient:
	case ENetworkFailure::OutdatedServer: Msg = TEXT("Versions differentes : tout le monde doit avoir le meme zip."); break;
	default: Msg = FString::Printf(TEXT("Erreur reseau : %s"), *Error); break;
	}
	SetStatus(Msg, true);
}

void UDSSessionSubsystem::OnTravelFailure(UWorld*, ETravelFailure::Type Type, const FString& Error)
{
	UE_LOG(LogDeepSweep, Warning, TEXT("Echec de voyage %s : %s"), ETravelFailure::ToString(Type), *Error);
	bBusy = false;
	SetStatus(TEXT("Impossible de rejoindre : verifie l'IP de l'hote et que sa partie est ouverte."), true);
}
