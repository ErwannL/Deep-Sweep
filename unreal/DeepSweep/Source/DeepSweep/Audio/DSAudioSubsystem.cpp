#include "Audio/DSAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/DSGameState.h"
#include "Machines/DSConveyor.h"
#include "Machines/DSVacuumNozzle.h"
#include "Giants/DSGiantPickup.h"
#include "Giants/DSCarryComponent.h"
#include "Events/DSOctopus.h"

static constexpr int32 GRate = 22050;

// ------------------------------------------------------------------ Synthese de notes

static float Osc(FDSTone::EWave W, float Phase, uint32& Rng)
{
	const float P = Phase - FMath::FloorToFloat(Phase);
	switch (W)
	{
	case FDSTone::Square: return P < 0.5f ? 0.6f : -0.6f;
	case FDSTone::Saw: return (2.f * P - 1.f) * 0.7f;
	case FDSTone::Triangle: return 4.f * FMath::Abs(P - 0.5f) - 1.f;
	case FDSTone::Noise: Rng = Rng * 1664525u + 1013904223u; return ((Rng >> 9) / float(1 << 23)) * 2.f - 1.f;
	default: return FMath::Sin(P * UE_TWO_PI);
	}
}

static void RenderTones(const TArray<FDSTone>& Tones, float Gain, TArray<int16>& Out)
{
	float Len = 0.f;
	for (const FDSTone& T : Tones) Len = FMath::Max(Len, T.Delay + T.Duration + 0.02f);
	TArray<float> Mix;
	Mix.SetNumZeroed(FMath::CeilToInt(Len * GRate));
	uint32 Rng = 0xC0FFEEu;
	for (const FDSTone& T : Tones)
	{
		const int32 Start = FMath::FloorToInt(T.Delay * GRate), N = FMath::CeilToInt(T.Duration * GRate);
		double Phase = 0.0;
		for (int32 i = 0; i < N && Start + i < Mix.Num(); ++i)
		{
			const float k = float(i) / N;
			// glissando exponentiel (comme exponentialRampToValueAtTime)
			const float F = FMath::Max(30.f, T.Freq * FMath::Pow(FMath::Max(30.f, T.Freq + T.Slide) / T.Freq, k));
			Phase += F / GRate;
			const float Env = FMath::Min(1.f, i / (0.004f * GRate)) * FMath::Pow(0.001f, k); // attaque courte, decroissance expo
			Mix[Start + i] += Osc(T.Wave, float(Phase), Rng) * T.Volume * Env;
		}
	}
	Out.SetNumUninitialized(Mix.Num());
	for (int32 i = 0; i < Mix.Num(); ++i) Out[i] = int16(FMath::Clamp(Mix[i] * Gain * 1.6f, -1.f, 1.f) * 32000.f);
}

bool UDSAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

bool UDSAudioSubsystem::CanPlay() const
{
	const UWorld* W = GetWorld();
	return W && W->GetNetMode() != NM_DedicatedServer && W->GetAudioDeviceRaw();
}

void UDSAudioSubsystem::PlayTones(const TArray<FDSTone>& Tones, float Gain)
{
	if (!CanPlay() || Tones.Num() == 0) return;
	TArray<int16> Pcm;
	RenderTones(Tones, Gain, Pcm);
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(GRate);
	Wave->NumChannels = 1;
	Wave->Duration = Pcm.Num() / float(GRate);
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	if (UAudioComponent* C = UGameplayStatics::SpawnSound2D(GetWorld(), Wave, 1.f, 1.f, 0.f, nullptr, false, false))
	{
		KeepAlive.Add(Wave);
		KeepAlive.Add(C);
		Live.Add({ C, float(GetWorld()->GetTimeSeconds() + Wave->Duration + 0.1f) });
	}
}

void UDSAudioSubsystem::PlaySfx(EDSSfx Sfx, const FVector& Location)
{
	if (!CanPlay()) return;
	// Attenuation simple selon la distance a la camera locale
	float Gain = 1.f;
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		if (!Location.IsZero()) Gain = FMath::Clamp(1.2f - float(FVector::Dist(Cam->GetCameraLocation(), Location)) / 5000.f, 0.15f, 1.f);

	using T = FDSTone;
	TArray<FDSTone> S;
	switch (Sfx)
	{
	case EDSSfx::Grab: S = { { 660.f, 0.06f, T::Triangle, 0.12f, 300.f } }; break;
	case EDSSfx::Throw: S = { { 300.f, 0.15f, T::Noise, 0.06f, 0.f }, { 500.f, 0.12f, T::Sine, 0.08f, -250.f } }; break;
	case EDSSfx::Sale:
		// pop au pitch croissant en combo (GDD 13)
		++Combo; ComboTimer = 1.5f;
		S = { { 440.f + float(FMath::Min(Combo, 40)) * 22.f, 0.08f, T::Triangle, 0.1f, 0.f } };
		break;
	case EDSSfx::Slurp:
		if (GetWorld()->GetTimeSeconds() - LastSlurp < 0.06f) return;
		LastSlurp = GetWorld()->GetTimeSeconds();
		S = { { 180.f + float(FMath::FRand()) * 80.f, 0.08f, T::Sine, 0.08f, 400.f } };
		break;
	case EDSSfx::Clog: S = { { 110.f, 0.35f, T::Square, 0.1f, 0.f } }; break;
	case EDSSfx::Bonk: S = { { 160.f, 0.12f, T::Square, 0.09f, 0.f }, { 90.f, 0.08f, T::Noise, 0.05f, 0.f } }; break;
	case EDSSfx::Unclog: S = { { 600.f, 0.12f, T::Square, 0.08f, 300.f } }; break;
	case EDSSfx::Break: S = { { 1200.f, 0.25f, T::Saw, 0.07f, -1000.f }, { 2000.f, 0.2f, T::Noise, 0.05f, 0.f } }; break;
	case EDSSfx::Recycled: S = { { 500.f, 0.7f, T::Sine, 0.2f, -420.f } }; break;
	case EDSSfx::Buy:
	case EDSSfx::Jingle: S = { { 523.f, 0.15f, T::Triangle, 0.12f, 0, 0.f }, { 659.f, 0.15f, T::Triangle, 0.12f, 0, 0.08f }, { 784.f, 0.15f, T::Triangle, 0.12f, 0, 0.16f }, { 1047.f, 0.2f, T::Triangle, 0.12f, 0, 0.24f } }; break;
	case EDSSfx::BigJingle:
		for (int32 i = 0; i < 8; ++i) S.Add({ float(523.f * FMath::Pow(2.f, (i % 4) * 4 / 12.f) * (i < 4 ? 1.f : 2.f)), 0.25f, T::Triangle, 0.12f, 0, i * 0.11f });
		break;
	case EDSSfx::OctoArrive: S = { { 300.f, 0.4f, T::Saw, 0.08f, 300.f } }; break;
	case EDSSfx::Splash: S = { { 700.f, 0.1f, T::Sine, 0.08f, -300.f }, { 800.f, 0.3f, T::Noise, 0.06f, 0.f } }; break;
	case EDSSfx::Jam: S = { { 140.f, 0.4f, T::Saw, 0.1f, -60.f } }; break;
	case EDSSfx::Alarm: S = { { 880.f, 0.12f, T::Square, 0.07f, 0, 0.f }, { 660.f, 0.12f, T::Square, 0.07f, 0, 0.15f } }; break;
	case EDSSfx::Denied: S = { { 200.f, 0.15f, T::Square, 0.07f, -60.f } }; break;
	}
	PlayTones(S, Gain);
}

void UDSAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!CanPlay()) return;
	Music = NewObject<UDSMusicWave>(this);
	MusicComp = UGameplayStatics::SpawnSound2D(&InWorld, Music, 0.55f, 1.f, 0.f, nullptr, true, false);
}

void UDSAudioSubsystem::Tick(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;
	ComboTimer -= DeltaTime;
	if (ComboTimer <= 0.f) Combo = 0;
	const float Now = W->GetTimeSeconds();
	for (int32 i = Live.Num() - 1; i >= 0; --i)
		if (Now > Live[i].EndTime)
		{
			if (Live[i].Comp) Live[i].Comp->Stop();
			Live.RemoveAtSwap(i);
		}
	if (Live.Num() == 0) KeepAlive.Reset();

	if (!Music) return;
	// Intensite : la chaine tourne-t-elle ? (aspiration, tapis, objets geants, poulpe)
	float I = 0.2f;
	for (TActorIterator<ADSVacuumNozzle> It(W); It; ++It) if (It->IsSucking()) I += 0.3f;
	for (TActorIterator<ADSConveyor> It(W); It; ++It) if (It->NumItems() > 0 && !It->bJammed) I += 0.3f;
	for (TActorIterator<ADSGiantPickup> It(W); It; ++It) if (It->GetCarry()->Carriers.Num()) I += 0.2f;
	for (TActorIterator<ADSOctopus> It(W); It; ++It) if (It->State != EDSOctoState::Idle) I += 0.3f;
	Music->Intensity.store(FMath::Clamp(I, 0.f, 1.f));
	if (const ADSGameState* GS = W->GetGameState<ADSGameState>())
	{
		Music->Brightness.store(GS->GetCleanliness());
		Music->bVictory.store(GS->IsMatchOver());
	}
}

// ------------------------------------------------------------------ Musique

UDSMusicWave::UDSMusicWave(const FObjectInitializer& OI) : Super(OI)
{
	SetSampleRate(GRate);
	NumChannels = 1;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = true;
	bProcedural = true;
}

int32 UDSMusicWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	// 104 BPM, double-croches ; Am - F - C - G ; pentatonique de La mineur.
	static const float Roots[4] = { 110.f, 87.31f, 130.81f, 98.f };
	static const int32 Arp[8] = { 0, 7, 12, 7, 3, 7, 10, 7 };
	const float Step = 60.f / 104.f / 4.f;
	const float Inten = Intensity.load();
	const float Bright = Brightness.load();
	const bool bWin = bVictory.load();

	OutAudio.SetNumUninitialized(NumSamples * sizeof(int16));
	int16* Out = reinterpret_cast<int16*>(OutAudio.GetData());
	for (int32 n = 0; n < NumSamples; ++n, ++SampleIndex)
	{
		const float t = SampleIndex / float(GRate);
		const int32 S16 = int32(t / Step);            // index de double-croche
		const float InStep = t - S16 * Step;
		const int32 Bar = (S16 / 16) % 4;
		const float Root = Roots[Bar] * (bWin ? 2.f : 1.f);
		SmoothedPerc += ((Inten - 0.3f) / 0.7f - SmoothedPerc) * 0.00005f;
		const float Perc = FMath::Clamp(SmoothedPerc, 0.f, 1.f);

		float s = 0.f;
		// Nappe douce (plus brillante quand la crique est propre)
		for (int32 k : { 0, 3 + int32(Bright > 0.5f), 7 })
			s += FMath::Sin(UE_TWO_PI * Root * 2.f * FMath::Pow(2.f, k / 12.f) * t) * 0.035f;
		// Basse arpegee (croches)
		if ((S16 % 2) == 0 || bWin)
		{
			const float F = Root * FMath::Pow(2.f, Arp[(S16 / 2) % 8] / 12.f);
			const float Env = FMath::Exp(-InStep * 9.f);
			const float Ph = F * t - FMath::FloorToFloat(F * t);
			s += (4.f * FMath::Abs(Ph - 0.5f) - 1.f) * 0.16f * Env;
		}
		// Couche percussive "industrielle" (monte avec l'activite)
		if (Perc > 0.01f)
		{
			if (S16 % 4 == 0) { const float e = FMath::Exp(-InStep * 30.f); s += FMath::Sin(UE_TWO_PI * (45.f + 90.f * e) * InStep) * 0.35f * e * Perc; } // kick
			if (S16 % 2 == 1) { Noise = Noise * 1664525u + 1013904223u; s += (((Noise >> 9) / float(1 << 23)) * 2.f - 1.f) * 0.05f * FMath::Exp(-InStep * 60.f) * Perc; } // hat
			if (S16 % 8 == 4 && Perc > 0.5f) { const float Ph = 900.f * InStep; s += ((Ph - FMath::FloorToFloat(Ph)) < 0.5f ? 0.08f : -0.08f) * FMath::Exp(-InStep * 40.f) * (Perc - 0.5f) * 2.f; } // clank metallique
		}
		Out[n] = int16(FMath::Clamp(s, -1.f, 1.f) * 30000.f);
	}
	return NumSamples;
}
