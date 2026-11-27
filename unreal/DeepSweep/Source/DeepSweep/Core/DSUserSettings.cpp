#include "Core/DSUserSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UDSUserSettings::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();
	FString Arg;
	if (FParse::Value(FCommandLine::Get(), TEXT("DSName="), Arg)) PlayerName = Arg;
	if (PlayerName.IsEmpty()) PlayerName = FString::Printf(TEXT("Plongeur%d"), FMath::RandRange(10, 99));
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.2f, 3.f);
	MusicVolume = FMath::Clamp(MusicVolume, 0.f, 1.f);
	SfxVolume = FMath::Clamp(SfxVolume, 0.f, 1.f);
}

UDSUserSettings* UDSUserSettings::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	return W && W->GetGameInstance() ? W->GetGameInstance()->GetSubsystem<UDSUserSettings>() : nullptr;
}
