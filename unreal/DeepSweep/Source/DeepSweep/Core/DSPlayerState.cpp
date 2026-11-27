#include "Core/DSPlayerState.h"
#include "Net/UnrealNetwork.h"

void ADSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSPlayerState, StatSold);
	DOREPLIFETIME(ADSPlayerState, StatSucked);
	DOREPLIFETIME(ADSPlayerState, StatBroken);
}
