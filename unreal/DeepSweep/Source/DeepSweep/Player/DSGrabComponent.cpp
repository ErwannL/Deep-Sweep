#include "Player/DSGrabComponent.h"
#include "Player/DSDiver.h"
#include "Pickups/DSPickup.h"
#include "Pickups/DSPickupData.h"
#include "DeepSweep.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

UDSGrabComponent::UDSGrabComponent()
{
	SetIsReplicatedByDefault(true);
}

void UDSGrabComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDSGrabComponent, Held);
	DOREPLIFETIME(UDSGrabComponent, Capacity);
	DOREPLIFETIME(UDSGrabComponent, Range);
}

void UDSGrabComponent::RequestGrab(ADSPickup* Target) { if (Target) Server_Grab(Target); }
void UDSGrabComponent::RequestDrop() { if (Held.Num()) Server_Drop(); }
void UDSGrabComponent::RequestThrow(const FVector& Aim) { if (Held.Num()) Server_Throw(Aim.GetSafeNormal()); }

void UDSGrabComponent::Server_Grab_Implementation(ADSPickup* Target)
{
	AActor* Owner = GetOwner();
	if (!Owner || !IsValid(Target) || Target->IsHeld() || IsFull() || !Target->GetType()) return;
	// Validation serveur : portee + marge de latence.
	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) - Target->GetType()->Radius;
	if (Dist > Range + 200.f) return;

	Target->SetHolder(Owner);
	Target->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
	Held.Add(Target);
	UE_LOG(LogDeepSweep, Log, TEXT("%s attrape %s (%d/%d)"), *Owner->GetName(), *Target->GetType()->TypeId.ToString(), Held.Num(), Capacity);
	OnHeldChanged();
}

void UDSGrabComponent::Release(ADSPickup* P, const FVector& Velocity)
{
	if (!IsValid(P)) return;
	P->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	P->SetHolder(nullptr);
	if (UStaticMeshComponent* M = P->GetMesh())
	{
		M->SetPhysicsLinearVelocity(Velocity);
		M->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f)));
	}
}

void UDSGrabComponent::Server_Drop_Implementation()
{
	AActor* Owner = GetOwner();
	const FVector Fwd = Owner->GetActorForwardVector();
	for (ADSPickup* P : Held)
	{
		if (!IsValid(P)) continue;
		P->SetActorLocation(Owner->GetActorLocation() + Fwd * 120.f + FVector(0, 0, 40.f));
		Release(P, Owner->GetVelocity() + Fwd * 150.f);
	}
	Held.Reset();
	OnHeldChanged();
}

void UDSGrabComponent::Server_Throw_Implementation(FVector_NetQuantizeNormal Aim)
{
	AActor* Owner = GetOwner();
	if (Held.Num() == 0) return;
	ADSPickup* P = Held.Pop();
	if (IsValid(P))
	{
		const float MassK = P->GetType() ? FMath::Clamp(20.f / P->GetType()->MassKg, 0.45f, 1.f) : 1.f; // lourd = vole moins loin
		const FVector Dir = (FVector(Aim) + FVector(0, 0, 0.15f)).GetSafeNormal();
		P->SetActorLocation(Owner->GetActorLocation() + Dir * 110.f + FVector(0, 0, 60.f));
		Release(P, Owner->GetVelocity() + Dir * ThrowSpeed * MassK);
		P->SetLastThrower(Owner);
		UE_LOG(LogDeepSweep, Log, TEXT("%s lance %s"), *Owner->GetName(), P->GetType() ? *P->GetType()->TypeId.ToString() : TEXT("?"));
	}
	OnHeldChanged();
}

void UDSGrabComponent::ServerForget(ADSPickup* Pickup)
{
	if (Held.Remove(Pickup) > 0) OnHeldChanged();
}

void UDSGrabComponent::OnRep_Held() { OnHeldChanged(); }

void UDSGrabComponent::OnHeldChanged()
{
	Held.RemoveAll([](const TObjectPtr<ADSPickup>& P) { return !IsValid(P); });
	LayoutHeld();
	if (ADSDiver* Diver = Cast<ADSDiver>(GetOwner()))
		Diver->SetCarrySpeedFactor(FMath::Max(0.4f, 1.f - SlowPerItem * Held.Num()));
}

void UDSGrabComponent::LayoutHeld()
{
	// Pile au-dessus du casque (comme le prototype), en coordonnees relatives.
	float Z = 105.f;
	for (ADSPickup* P : Held)
	{
		if (!IsValid(P) || !P->GetType()) continue;
		const float R = P->GetType()->Radius;
		Z += R;
		P->SetActorRelativeLocation(FVector(0.f, 0.f, Z));
		P->SetActorRelativeRotation(FRotator(0.f, 30.f, 0.f));
		Z += R + 5.f;
	}
}

void UDSGrabComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
		for (ADSPickup* P : Held) Release(P, FVector::ZeroVector);
	Held.Reset();
	Super::EndPlay(Reason);
}
