#include "Net/DSSessionSubsystem.h"
#include "DeepSweep.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformMisc.h"

static const FName GDSSession = NAME_GameSession;

void UDSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Pseudo par defaut neutre (pas le nom du PC) ; modifiable dans le menu ou via -Name=...
	PlayerName = FString::Printf(TEXT("Plongeur%d"), FMath::RandRange(10, 99));
	FString Arg;
	if (FParse::Value(FCommandLine::Get(), TEXT("DSName="), Arg)) PlayerName = Arg;
}

IOnlineSessionPtr UDSSessionSubsystem::Sessions() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

FString UDSSessionSubsystem::NameOption() const
{
	FString Clean = PlayerName.Left(16).Replace(TEXT("?"), TEXT("")).Replace(TEXT("="), TEXT("")).Replace(TEXT(" "), TEXT("_"));
	return TEXT("?Name=") + (Clean.IsEmpty() ? FString(TEXT("Plongeur")) : Clean);
}

void UDSSessionSubsystem::HostLAN()
{
	IOnlineSessionPtr S = Sessions();
	if (!S.IsValid())
	{
		// Sans Online Subsystem : on heberge quand meme (rejoindre par IP)
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
	Settings.Set(FName(TEXT("HOSTNAME")), PlayerName, EOnlineDataAdvertisementType::ViaOnlineService);
	CreateHandle = S->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnCreateComplete));
	Status = TEXT("Creation de la partie...");
	OnChanged.Broadcast();
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
	UE_LOG(LogDeepSweep, Log, TEXT("Session LAN creee : %d"), bOk);
	if (!bOk) { Status = TEXT("Echec de creation de la partie"); OnChanged.Broadcast(); return; }
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
	Status = TEXT("Recherche sur le reseau local...");
	OnChanged.Broadcast();
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
			if (F.HostName.IsEmpty()) F.HostName = R.Session.OwningUserName;
			F.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			F.OpenSlots = R.Session.NumOpenPublicConnections;
			F.PingMs = R.PingInMs;
			Found.Add(F);
		}
	Status = Found.Num() ? FString::Printf(TEXT("%d partie(s) trouvee(s)"), Found.Num()) : TEXT("Aucune partie trouvee (essayez par IP)");
	UE_LOG(LogDeepSweep, Log, TEXT("Recherche LAN : %d resultat(s)"), Found.Num());
	OnChanged.Broadcast();
}

void UDSSessionSubsystem::JoinFound(int32 Index)
{
	IOnlineSessionPtr S = Sessions();
	if (!S.IsValid() || !Search.IsValid() || !Search->SearchResults.IsValidIndex(Index)) return;
	JoinHandle = S->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UDSSessionSubsystem::OnJoinComplete));
	Status = TEXT("Connexion...");
	OnChanged.Broadcast();
	S->JoinSession(0, GDSSession, Search->SearchResults[Index]);
}

void UDSSessionSubsystem::OnJoinComplete(FName Session, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr S = Sessions();
	if (S.IsValid()) S->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	FString Url;
	if (Result != EOnJoinSessionCompleteResult::Success || !S.IsValid() || !S->GetResolvedConnectString(Session, Url))
	{
		Status = TEXT("Impossible de rejoindre cette partie");
		OnChanged.Broadcast();
		return;
	}
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		PC->ClientTravel(Url + NameOption(), TRAVEL_Absolute);
}

void UDSSessionSubsystem::JoinIP(const FString& Address)
{
	FString A = Address.TrimStartAndEnd();
	if (A.IsEmpty()) A = TEXT("127.0.0.1");
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		Status = FString::Printf(TEXT("Connexion a %s..."), *A);
		OnChanged.Broadcast();
		PC->ClientTravel(A + NameOption(), TRAVEL_Absolute);
	}
}

void UDSSessionSubsystem::LeaveToMenu()
{
	if (IOnlineSessionPtr S = Sessions()) S->DestroySession(GDSSession);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*MenuMap));
}
