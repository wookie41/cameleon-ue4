#include "SwitchCharacterComponent.h"

USwitchCharacterComponent::USwitchCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void USwitchCharacterComponent::BeginPlay()
{
	Super::BeginPlay();
}


void USwitchCharacterComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}