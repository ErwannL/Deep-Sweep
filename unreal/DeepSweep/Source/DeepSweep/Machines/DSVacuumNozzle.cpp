#include "Machines/DSVacuumNozzle.h"
#include "Machines/DSVacuum.h"
#include "Player/DSDiver.h"
#include "Player/DSGrabComponent.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupSubsystem.h"
#include "Core/DSGameState.h"
#include "Core/DSPlayerState.h"
#include "FX/DSPopupText.h"
#include "FX/DSFX.h"
#include "DeepSweep.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DeepSweep"

ADSVacuumNozzle::ADSVacuumNozzle()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetReplicatingMovement(true);
	InteractRadius = 260.f;
	LabelHeight = 130.f;
	LabelColor = FLinearColor(0.75f, 0.12f, 0.1f);
	Priority = 8;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ray(TEXT("/Game/Materials/M_DSLightRay.M_DSLightRay"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Embouchure orientee vers +X (l'avant), poignee derriere.
	Mouth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mouth"));
	Mouth->SetupAttachment(RootComponent);
	Mouth->SetStaticMesh(Cone.Object);
	Mouth->SetRelativeRotation(FRotator(90.f, 0.f, 0.f)); // pointe vers l arriere, ouverture vers l avant
	Mouth->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.9f));
	Mouth->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
	Handle->SetupAttachment(RootComponent);
	Handle->SetStaticMesh(Cyl.Object);
	Handle->SetRelativeLocation(FVector(-60.f, 0.f, 0.f));
	Handle->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Handle->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.6f));
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Cone d'aspiration (visuel local) : pointe a la buse, s'ouvre vers l'avant.
	SuckCone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SuckCone"));
	SuckCone->SetupAttachment(RootComponent);
	SuckCone->SetStaticMesh(Cone.Object);
	if (Ray.Succeeded()) SuckCone->SetMaterial(0, Ray.Object);
	SuckCone->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	SuckCone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SuckCone->SetCastShadow(false);
	SuckCone->SetVisibility(false);
}

void ADSVacuumNozzle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSVacuumNozzle, Holder);
	DOREPLIFETIME(ADSVacuumNozzle, bOn);
}

void ADSVacuumNozzle::BeginPlay()
{
	Super::BeginPlay();
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col)
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, UDSPickupSubsystem::GetColorMaterial()))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetScalarParameterValue(TEXT("Emissive"), 0.2f);
		}
	};
	Tint(Mouth, FLinearColor(1.f, 0.5f, 0.05f));
	Tint(Handle, FLinearColor(0.2f, 0.2f, 0.25f));
}

bool ADSVacuumNozzle::IsSucking() const
{
	return Holder && bOn && Vacuum && Vacuum->Clog == 0 && !Vacuum->IsFull();
}

float ADSVacuumNozzle::GetActivationRadius() const
{
	return IsSucking() ? Vacuum->GetSuctionRadius() + 150.f : 0.f;
}

bool ADSVacuumNozzle::CanInteract(const ADSDiver* Diver) const
{
	if (!Diver) return false;
	if (Holder) return Holder == Diver; // le porteur peut la lacher
	return Diver->GetGrab() && Diver->GetGrab()->NumHeld() == 0 && !Diver->IsStunned();
}

FText ADSVacuumNozzle::GetPrompt(const ADSDiver* Diver) const
{
	if (Holder == Diver)
	{
		if (Vacuum && Vacuum->Clog > 0) return FText::Format(LOCTEXT("NozzleClog", "BOUCHE ! Action : taper ({0})  -  Attraper : lacher"), Vacuum->Clog);
		return bOn ? LOCTEXT("NozzleOff", "Action : eteindre  -  Attraper : lacher la buse")
		           : LOCTEXT("NozzleOn", "Action : aspirer  -  Attraper : lacher la buse");
	}
	return LOCTEXT("NozzleTake", "Attraper : prendre la buse");
}

void ADSVacuumNozzle::Interact(ADSDiver* Diver)
{
	if (!HasAuthority() || !Diver) return;
	if (Holder == Diver) { Release(); return; }
	if (Holder) return;
	Holder = Diver;
	LastHolder = Diver;
	ApplyHolder();
}

void ADSVacuumNozzle::Release()
{
	if (!HasAuthority()) return;
	const FVector Loc = GetActorLocation();
	Holder = nullptr;
	bOn = false;
	ApplyHolder();
	// Retombe au fond
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Loc, Loc - FVector(0, 0, 3000.f), ECC_WorldStatic))
		SetActorLocation(Hit.Location + FVector(0, 0, 45.f));
}

void ADSVacuumNozzle::PrimaryAction(ADSDiver* Diver)
{
	if (!HasAuthority() || Holder != Diver || !Vacuum) return;
	if (Vacuum->Clog > 0)
	{
		--Vacuum->Clog;
		if (ADSGameState* G = GetWorld()->GetGameState<ADSGameState>()) G->Multicast_Sfx(Vacuum->Clog ? EDSSfx::Bonk : EDSSfx::Unclog, GetActorLocation(), 0.15f);
		Multicast_Popup(GetActorLocation() + FVector(0, 0, 120.f), Vacuum->Clog ? LOCTEXT("Tap", "BONK !") : LOCTEXT("Unclogged", "Debouche !"),
			Vacuum->Clog ? FColor(255, 160, 60) : FColor(120, 255, 140));
		return;
	}
	bOn = !bOn;
}

void ADSVacuumNozzle::OnRep_Holder() { ApplyHolder(); }

void ADSVacuumNozzle::ApplyHolder()
{
	if (ADSDiver* Old = LastHolder.Get()) if (Old != Holder) Old->SetHeldNozzle(nullptr);
	if (Holder)
	{
		AttachToActor(Holder, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(130.f, 30.f, -20.f));
		SetActorRelativeRotation(FRotator::ZeroRotator);
		Holder->SetHeldNozzle(this);
		LastHolder = Holder;
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	}
}

void ADSVacuumNozzle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Visuel local : cone d'aspiration + tremblement de la buse selon la puissance.
	const bool bSucking = IsSucking();
	SuckCone->SetVisibility(bSucking);
	if (bSucking && Vacuum)
	{
		const float R = Vacuum->GetSuctionRadius();
		SuckCone->SetRelativeScale3D(FVector(R * 0.011f, R * 0.011f, R * 0.01f));
		SuckCone->SetRelativeLocation(FVector(R * 0.5f + 40.f, 0.f, 0.f));
		const float T = GetWorld()->GetTimeSeconds();
		Mouth->SetRelativeLocation(FVector(FMath::Sin(T * 40.f) * 1.5f * Vacuum->Power, 0.f, FMath::Cos(T * 33.f) * Vacuum->Power));
	}
	else Mouth->SetRelativeLocation(FVector::ZeroVector);

	if (bSucking)
	{
		SwirlTimer -= DeltaSeconds;
		if (SwirlTimer <= 0.f)
		{
			SwirlTimer = 0.12f;
			const float R = Vacuum->GetSuctionRadius();
			ADSFXBurst::Play(this, EDSFX::Bubbles, GetActorLocation() + GetActorForwardVector() * FMath::FRandRange(0.3f, 0.9f) * R + FMath::VRand() * R * 0.25f, 0.3f);
		}
	}
	if (HasAuthority() && bSucking) ServerSuck(DeltaSeconds);

	// Limite du tuyau : le porteur est retenu au-dela de la longueur.
	if (Holder && Vacuum && (Holder->HasAuthority() || Holder->IsLocallyControlled()))
	{
		const FVector Out = Vacuum->GetOutletLocation();
		const FVector D = Holder->GetActorLocation() - Out;
		const float Over = D.Size() - Vacuum->HoseLength;
		if (Over > 0.f) Holder->GetCharacterMovement()->AddImpulse(-D.GetSafeNormal() * Over * 8.f * DeltaSeconds, true);
	}
}

void ADSVacuumNozzle::ServerSuck(float DeltaSeconds)
{
	UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	if (!Sub || !Vacuum) return;
	const FVector Mouthpos = GetActorLocation();
	const FVector Fwd = GetActorForwardVector();
	const float R = Vacuum->GetSuctionRadius();
	const int32 P = Vacuum->Power;

	TArray<ADSPickup*> ToAbsorb;
	for (const TWeakObjectPtr<ADSPickup>& W : Sub->GetActives())
	{
		ADSPickup* Pk = W.Get();
		if (!Pk || Pk->IsHeld() || !Pk->GetType()) continue;
		const FVector D = Pk->GetActorLocation() - Mouthpos;
		const float Dist = D.Size();
		if (Dist > R + Pk->GetType()->Radius) continue;
		if (FVector::DotProduct(D / FMath::Max(Dist, 1.f), Fwd) < 0.35f && Dist > 120.f) continue; // cone avant
		if (Dist < 110.f + Pk->GetType()->Radius * 0.5f) { ToAbsorb.Add(Pk); continue; }
		// Attire vers l'embouchure, plus fort de pres
		const float Strength = (P * 650.f + 900.f) * (1.4f - Dist / (R + 100.f));
		Pk->GetMesh()->AddForce(-D / Dist * Strength, NAME_None, true);
	}

	for (ADSPickup* Pk : ToAbsorb)
	{
		if (Vacuum->Clog > 0 || Vacuum->IsFull()) break;
		UDSPickupData* T = Pk->GetType();
		ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
		if (T->bClogs && FMath::FRand() < 0.6f)
		{
			// Caddie coince dans le tuyau : 3 tapes pour deboucher.
			Vacuum->Clog = 3;
			Pk->GetMesh()->SetPhysicsLinearVelocity(Fwd * 300.f);
			if (GS) { GS->Multicast_Toast(LOCTEXT("Clogged", "Caddie coince ! Action x3 pour taper dessus")); GS->Multicast_Sfx(EDSSfx::Clog, Mouthpos, 0.35f); }
			break;
		}
		if (T->bFragile && P >= 4)
		{
			// Trop puissant : le vase se brise (sort du jeu, sans credits).
			Multicast_Popup(Pk->GetActorLocation(), LOCTEXT("Crack", "CRAC !"), FColor(255, 90, 60));
			if (ADSPlayerState* PS = Holder ? Holder->GetPlayerState<ADSPlayerState>() : nullptr) PS->StatBroken++;
			if (ADSPickupManager* M = Sub->GetManager()) M->NotifyCollected(1);
			if (GS) { GS->Multicast_Toast(LOCTEXT("VaseBroken", "Vase brise... (puissance trop forte)")); GS->Multicast_Sfx(EDSSfx::Break, Pk->GetActorLocation(), 0.25f); }
			Pk->Destroy();
			continue;
		}
		Vacuum->StoreItem(T);
		Pk->Destroy();
	}

	if (P >= 4) SuckDivers(DeltaSeconds, R);
}

void ADSVacuumNozzle::SuckDivers(float DeltaSeconds, float R)
{
	const FVector Mouthpos = GetActorLocation();
	const FVector Fwd = GetActorForwardVector();
	for (TActorIterator<ADSDiver> It(GetWorld()); It; ++It)
	{
		ADSDiver* D = *It;
		if (D == Holder || D->IsStunned()) continue;
		const FVector V = D->GetActorLocation() - Mouthpos;
		const float Dist = V.Size();
		if (Dist > R || FVector::DotProduct(V, Fwd) <= 0.f) continue;
		D->GetCharacterMovement()->AddImpulse(-V / FMath::Max(Dist, 1.f) * (Vacuum->Power - 3) * 1400.f * DeltaSeconds, true);
		if (Dist < 150.f)
		{
			// Collegue recycle : ressort par la cuve, sonne 1,5 s.
			D->TeleportTo(Vacuum->GetRecycleLocation() + FVector(0, 0, 220.f), D->GetActorRotation());
			D->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			D->Stun(1.5f);
			if (ADSPlayerState* PS = Holder->GetPlayerState<ADSPlayerState>()) PS->StatSucked++;
			const APlayerState* Victim = D->GetPlayerState();
			if (ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>())
			{
				GS->Multicast_Toast(FText::Format(LOCTEXT("Recycled", "{0} a ete recycle(e) !"), FText::FromString(Victim ? Victim->GetPlayerName() : TEXT("Un collegue"))));
				GS->Multicast_Sfx(EDSSfx::Recycled, D->GetActorLocation(), 0.6f);
			}
		}
	}
}

void ADSVacuumNozzle::Multicast_Popup_Implementation(FVector_NetQuantize Location, const FText& Text, FColor Color)
{
	ADSPopupText::Spawn(this, Location, Text, Color, 80.f);
}

FText ADSVacuumNozzle::GetStationLabel() const
{
	return Holder ? FText::GetEmpty() : LOCTEXT("StNozzle", "BUSE DE L'ASPIRATEUR");
}

#undef LOCTEXT_NAMESPACE
