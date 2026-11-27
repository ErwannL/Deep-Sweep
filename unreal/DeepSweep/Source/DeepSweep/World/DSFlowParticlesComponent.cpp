#include "World/DSFlowParticlesComponent.h"

UDSFlowParticlesComponent::UDSFlowParticlesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetMobility(EComponentMobility::Movable);
}

FVector UDSFlowParticlesComponent::RandomPoint(bool bAtSource) const
{
	FVector P(FMath::FRandRange(-Extent.X, Extent.X), FMath::FRandRange(-Extent.Y, Extent.Y), FMath::FRandRange(-Extent.Z, Extent.Z));
	if (bColumn)
	{
		const float R = FMath::Sqrt(FMath::FRand()) * Extent.X;
		const float A = FMath::FRand() * UE_TWO_PI;
		P.X = FMath::Cos(A) * R;
		P.Y = FMath::Sin(A) * R;
	}
	if (bAtSource && !Velocity.IsNearlyZero())
	{
		// Repart du cote amont de la boite
		const FVector Dir = Velocity.GetSafeNormal();
		for (int32 Axis = 0; Axis < 3; ++Axis)
			if (FMath::Abs(Dir[Axis]) > 0.5f) P[Axis] = -FMath::Sign(Dir[Axis]) * Extent[Axis];
	}
	return P;
}

void UDSFlowParticlesComponent::BeginPlay()
{
	Super::BeginPlay();
	ClearInstances();
	Particles.SetNum(Count);
	Scratch.SetNum(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		FParticle& P = Particles[i];
		P.Pos = RandomPoint(false);
		P.Scale = FMath::FRandRange(ScaleRange.X, ScaleRange.Y);
		P.Phase = FMath::FRand() * 10.f;
		P.SpeedMul = FMath::FRandRange(0.7f, 1.3f);
		Scratch[i] = FTransform(P.Pos);
	}
	AddInstances(Scratch, false, false);
}

void UDSFlowParticlesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Particles.Num() == 0 || GetNetMode() == NM_DedicatedServer) return;

	const FVector Dir = Velocity.GetSafeNormal();
	const FQuat Align = Dir.IsNearlyZero() ? FQuat::Identity : FRotationMatrix::MakeFromZ(Dir).ToQuat();
	const float T = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Particles.Num(); ++i)
	{
		FParticle& P = Particles[i];
		P.Pos += Velocity * P.SpeedMul * DeltaTime;
		if (FMath::Abs(P.Pos.X) > Extent.X * (bColumn ? 1.5f : 1.f) || FMath::Abs(P.Pos.Y) > Extent.Y * (bColumn ? 1.5f : 1.f) || FMath::Abs(P.Pos.Z) > Extent.Z)
			P.Pos = RandomPoint(true);
		const FVector Wob(FMath::Sin(T * 3.f + P.Phase) * Wobble, FMath::Cos(T * 2.3f + P.Phase) * Wobble, 0.f);
		const FVector S(P.Scale, P.Scale, P.Scale * Stretch);
		Scratch[i] = FTransform(Align, P.Pos + Wob, S);
	}
	BatchUpdateInstancesTransforms(0, Scratch, false, true, true);
}
