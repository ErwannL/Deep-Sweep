#include "Net/DSUpdateSubsystem.h"
#include "DeepSweep.h"
#include "GeneralProjectSettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UDSUpdateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Recheck();
}

FString UDSUpdateSubsystem::GetLocalVersion() const
{
	return GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

bool UDSUpdateSubsystem::IsNewer(const FString& Remote, const FString& Local)
{
	auto Parse = [](FString V)
	{
		V.RemoveFromStart(TEXT("v"));
		V.RemoveFromStart(TEXT("V"));
		TArray<FString> P;
		V.ParseIntoArray(P, TEXT("."));
		TArray<int32> N;
		for (int32 i = 0; i < 3; ++i) N.Add(P.IsValidIndex(i) ? FCString::Atoi(*P[i]) : 0);
		return N;
	};
	const TArray<int32> R = Parse(Remote), L = Parse(Local);
	for (int32 i = 0; i < 3; ++i)
	{
		if (R[i] != L[i]) return R[i] > L[i];
	}
	return false;
}

void UDSUpdateSubsystem::Recheck()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("NoUpdateCheck")) || GIsEditor) { State = EDSUpdateState::UpToDate; return; }
	State = EDSUpdateState::Checking;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(LatestApi);
	Req->SetVerb(TEXT("GET"));
	Req->SetHeader(TEXT("Accept"), TEXT("application/vnd.github+json"));
	Req->SetHeader(TEXT("User-Agent"), TEXT("DeepSweep-Game"));
	Req->SetTimeout(8.f);
	TWeakObjectPtr<UDSUpdateSubsystem> Self(this);
	Req->OnProcessRequestComplete().BindLambda([Self](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		if (!Self.IsValid()) return;
		UDSUpdateSubsystem* S = Self.Get();
		TSharedPtr<FJsonObject> Json;
		if (!bOk || !Resp.IsValid() || Resp->GetResponseCode() != 200
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Resp->GetContentAsString()), Json) || !Json.IsValid())
		{
			S->State = EDSUpdateState::Offline; // pas d'Internet : on laisse jouer
			UE_LOG(LogDeepSweep, Log, TEXT("Mise a jour : verification impossible (hors ligne ?)"));
			S->OnChanged.Broadcast();
			return;
		}
		S->LatestVersion = Json->GetStringField(TEXT("tag_name"));
		S->DownloadUrl = Json->GetStringField(TEXT("html_url"));
		const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
		if (Json->TryGetArrayField(TEXT("assets"), Assets))
			for (const TSharedPtr<FJsonValue>& A : *Assets)
			{
				const TSharedPtr<FJsonObject> O = A->AsObject();
				FString Name, Url;
				if (O.IsValid() && O->TryGetStringField(TEXT("name"), Name) && Name.EndsWith(TEXT(".exe")) && O->TryGetStringField(TEXT("browser_download_url"), Url))
				{
					S->DownloadUrl = Url; // l'installateur en priorite
					break;
				}
			}
		const bool bNewer = IsNewer(S->LatestVersion, S->GetLocalVersion());
		S->State = bNewer ? EDSUpdateState::UpdateRequired : EDSUpdateState::UpToDate;
		UE_LOG(LogDeepSweep, Log, TEXT("Mise a jour : locale %s, derniere %s -> %s"), *S->GetLocalVersion(), *S->LatestVersion, bNewer ? TEXT("MISE A JOUR REQUISE") : TEXT("a jour"));
		S->OnChanged.Broadcast();
	});
	Req->ProcessRequest();
}

void UDSUpdateSubsystem::OpenDownload() const
{
	const FString Url = DownloadUrl.IsEmpty() ? FString(TEXT("https://github.com/ErwannL/Deep-Sweep/releases/latest")) : DownloadUrl;
	FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
}
