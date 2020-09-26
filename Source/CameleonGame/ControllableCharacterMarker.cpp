#include "ControllableCharacterMarker.h"
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"

// Sets default values for this component's properties
AControllableCharacterMarker::AControllableCharacterMarker()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(5.0f);
	// CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	SphereComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	SphereComponent->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = SphereComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
}

void AControllableCharacterMarker::SetActive(const bool& Active)
{
	MeshComponent->SetMaterial(0, Active ? ActiveMaterial : DefaultMaterial);
}

// Called when the game starts
void AControllableCharacterMarker::BeginPlay()
{
	Super::BeginPlay();
}
