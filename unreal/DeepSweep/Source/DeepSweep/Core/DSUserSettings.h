#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DSUserSettings.generated.h"

/** Reglages du joueur, sauvegardes entre les sessions (GameUserSettings.ini). */
UCLASS(config = GameUserSettings)
class DEEPSWEEP_API UDSUserSettings : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(config) FString PlayerName;
	UPROPERTY(config) FString LastJoinAddress;
	UPROPERTY(config) float MouseSensitivity = 1.f;
	UPROPERTY(config) bool bInvertY = false;
	UPROPERTY(config) float MusicVolume = 0.6f;
	UPROPERTY(config) float SfxVolume = 0.9f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void Save() { SaveConfig(); }

	static UDSUserSettings* Get(const UObject* WorldContext);
};
