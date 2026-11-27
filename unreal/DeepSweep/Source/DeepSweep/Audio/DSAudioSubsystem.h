#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Sound/SoundWaveProcedural.h"
#include <atomic>
#include "DSAudioSubsystem.generated.h"

class UAudioComponent;

/** Effets sonores du jeu (generes par synthese, aucun fichier audio). */
UENUM()
enum class EDSSfx : uint8
{
	Grab, Throw, Sale, Slurp, Clog, Bonk, Unclog, Break, Recycled, Jingle, BigJingle, OctoArrive, Splash, Jam, Alarm, Buy, Denied
};

/** Une note de synthese (comme le prototype WebAudio). */
struct FDSTone
{
	enum EWave : uint8 { Sine, Square, Saw, Triangle, Noise };
	float Freq = 440.f;
	float Duration = 0.1f;
	EWave Wave = Sine;
	float Volume = 0.12f;
	float Slide = 0.f;   // variation de frequence (Hz) sur la duree
	float Delay = 0.f;
};

/**
 * Musique dynamique : boucle synthetisee en continu (thread audio). Une couche percussive
 * "industrielle" monte quand la chaine tourne (GDD 13).
 */
UCLASS()
class DEEPSWEEP_API UDSMusicWave : public USoundWaveProcedural
{
	GENERATED_BODY()
public:
	UDSMusicWave(const FObjectInitializer& OI);
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;

	std::atomic<float> Intensity{ 0.2f };
	std::atomic<float> Brightness{ 0.f }; // 0 sale -> 1 propre : accords plus lumineux
	std::atomic<bool> bVictory{ false };

private:
	int64 SampleIndex = 0;
	uint32 Noise = 0x1234567u;
	float SmoothedPerc = 0.f;
};

/** Joue les sons localement (chaque machine) et pilote la musique. */
UCLASS()
class DEEPSWEEP_API UDSAudioSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	void PlaySfx(EDSSfx Sfx, const FVector& Location);
	void PlayTones(const TArray<FDSTone>& Tones, float Gain = 1.f);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDSAudioSubsystem, STATGROUP_Tickables); }
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
	struct FLive { TObjectPtr<UAudioComponent> Comp; float EndTime; };
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> KeepAlive;
	TArray<FLive> Live;
	UPROPERTY(Transient) TObjectPtr<UDSMusicWave> Music;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicComp;
	int32 Combo = 0;
	float ComboTimer = 0.f;
	float LastSlurp = -1.f;
	bool CanPlay() const;
};
