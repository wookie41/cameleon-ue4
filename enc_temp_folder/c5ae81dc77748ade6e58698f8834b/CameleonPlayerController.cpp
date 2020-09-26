#include "CameleonPlayerController.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "ControllableCharacterMarker.h"
#include "Camera/CameraActor.h"

ACameleonPlayerController::ACameleonPlayerController()
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>("CollisionComponent");
	CollisionComponent->SetRelativeLocation({ScanDistance.X / 2, 0, ScanDistance.Z / 2});

	bAutoManageActiveCameraTarget = false;
	bCanSwitch = true;
}

void ACameleonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	InputComponent->BindAction("NextCharacter", IE_Pressed, this, &ACameleonPlayerController::SetNextAsActive);
	InputComponent->BindAction("PreviousCharacter", IE_Pressed, this, &ACameleonPlayerController::SetPreviousAsActive);
	InputComponent->BindAction("SwitchCharacter", IE_Pressed, this, &ACameleonPlayerController::SwitchCharacter);
}


void ACameleonPlayerController::BeginPlay()
{
	SetActorHiddenInGame(false); //todo remove

	if (const auto character = GetCharacter())
	{
		if (const auto camera = character->FindComponentByClass<UCameraComponent>())
		{
			CurrentCharacterCamera = camera;
			CollisionComponent->AttachToComponent(camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	FScriptDelegate overlapBeginDelegate;
	FScriptDelegate overlapEndDelegate;

	overlapBeginDelegate.BindUFunction(this, FName("OnActorBeginOverlap"));
	overlapEndDelegate.BindUFunction(this, FName("OnActorEndOverlap"));

	CollisionComponent->SetHiddenInGame(false);
	CollisionComponent->SetBoxExtent(ScanDistance);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentBeginOverlap.Add(overlapBeginDelegate);
	CollisionComponent->OnComponentEndOverlap.Add(overlapEndDelegate);
}

void ACameleonPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInTransition)
	{
		TransitionTimer += DeltaSeconds;
		if (TransitionTimer > TransitionTimeSeconds)
		{
			Possess(CharactersInSight[ActiveCharacterIndex]);
			EnableInput(this);

			for (auto it = ControllableCharacters.CreateIterator(); it; ++it)
			{
				it.Value()->Destroy();
			}

			ControllableCharacters.Empty();
			CharactersInSight.Empty();
			ActiveCharacterIndex = -1;

			bInTransition = false;
			bCanSwitch = true;
		}
	}
}

void ACameleonPlayerController::SetNextAsActive()
{
	if (bInTransition || ActiveCharacterIndex < 0)
	{
		return;
	}

	(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(false);

	if (++ActiveCharacterIndex >= CharactersInSight.Num())
	{
		ActiveCharacterIndex = 0;
	}

	(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(true);
}

void ACameleonPlayerController::SetPreviousAsActive()
{
	if (bInTransition || ActiveCharacterIndex < 0)
	{
		return;
	}

	(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(false);

	if (--ActiveCharacterIndex < 0)
	{
		ActiveCharacterIndex = CharactersInSight.Num() - 1;
	}

	(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(true);
}

void ACameleonPlayerController::SwitchCharacter()
{
	if (bCanSwitch && ActiveCharacterIndex > -1)
	{
		auto characterToUse = CharactersInSight[ActiveCharacterIndex];

		if (auto mesh = characterToUse->GetMesh())
		{
			DisableInput(this);
			UnPossess();

			bCanSwitch = false;
			bInTransition = true;
			TransitionTimer = 0;

			CollisionComponent->AttachToComponent(mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SetViewTargetWithBlend(characterToUse, TransitionTimeSeconds);
		}
	}
}

void ACameleonPlayerController::OnActorBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                    AActor* OtherActor,
                                                    UPrimitiveComponent* OtherComp,
                                                    int32 OtherBodyIndex,
                                                    bool bFromSweep,
                                                    const FHitResult& SweepResult)
{
	if (bInTransition)
	{
		return;
	}
	auto overlappedCharacter = Cast<ACharacter>(OtherActor);

	if (!overlappedCharacter)
	{
		return;
	}

	if (overlappedCharacter == GetCharacter())
	{
		return;
	}

	// Send a trace from player's eyes to the overlapped character to check if it's actually visible //

	FHitResult hitResult;
	GetWorld()->LineTraceSingleByChannel(hitResult,
	                                     CurrentCharacterCamera->GetComponentLocation() + CurrentCharacterCamera->
	                                     GetForwardVector() * 100,
	                                     overlappedCharacter->GetActorLocation(), ECC_Camera);
	if (hitResult.Actor == overlappedCharacter)
	{
		// Get the mesh and capsule of the overlapped character //

		auto mesh = overlappedCharacter->GetMesh();
		auto capsule = overlappedCharacter->GetCapsuleComponent();

		if (mesh && capsule)
		{
			// Calculate the offset from the character's head at which we'll place the marker //

			float radius, halfHeight;
			capsule->GetScaledCapsuleSize(radius, halfHeight);

			float aboveActorHead = halfHeight + 10;

			// Spawn the marker //

			auto marker = GetWorld()->SpawnActor<AControllableCharacterMarker>(
				MarkerClass,
				overlappedCharacter->GetActorLocation() + FVector{0, 0, aboveActorHead},
				FRotator::ZeroRotator
			);

			marker->AttachToActor(overlappedCharacter, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			marker->SetActorRelativeLocation({0, 0, aboveActorHead});

			ControllableCharacters.Add(overlappedCharacter, marker);

			// Add the actor to the array holding actors in our sight so we can switch if there are multiple 
			// The array holds the character in order from the closest to the one most far away, so we need
			// to find a place for the newly overlapped character

			float distanceToOverlappedCharacter = (overlappedCharacter->GetActorLocation() - GetCharacter()->
				GetActorLocation()).Size();

			int overlappedCharacterIndex = -1;
			for (int characterIdx = 0; characterIdx < CharactersInSight.Num(); ++characterIdx)
			{
				float distToCharacter = (overlappedCharacter->GetActorLocation() - GetCharacter()->GetActorLocation()).
					Size();
				if (distToCharacter > distanceToOverlappedCharacter)
				{
					overlappedCharacterIndex = characterIdx;
					CharactersInSight.EmplaceAt(overlappedCharacterIndex, overlappedCharacter);
					if (overlappedCharacterIndex == ActiveCharacterIndex)
					{
						++ActiveCharacterIndex;
					}
				}
			}

			// This is the case then there were no characters in sight or the new character is behind all
			// of the currently overlapped characters

			if (overlappedCharacterIndex == -1)
			{
				CharactersInSight.Add(overlappedCharacter);
				if (ActiveCharacterIndex < 0)
				{
					ActiveCharacterIndex = 0;
					marker->SetActive(true);
				}
				else
				{
					marker->SetActive(false);
				}
			}
			else
			{
				marker->SetActive(false);
			}
		}
	}
}

void ACameleonPlayerController::OnActorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (bInTransition)
	{
		return;
	}

	if (auto character = Cast<ACharacter>(OtherActor)) // make sure the overlapped actor is a character
	{
		// Get a marker for the overlapped character to Destroy() it
		auto marker = ControllableCharacters.Find(character);
		if (marker)
		{
			(*marker)->Destroy();

			// Find the index of the overlapped character and remove it //

			int removedCharacterIndex = CharactersInSight.IndexOfByPredicate([character](ACharacter* It)
			{
				return character == It;
			});

			CharactersInSight.RemoveAt(removedCharacterIndex);

			// Check if the removed character was active and if yes, then mark next as active

			if (removedCharacterIndex == ActiveCharacterIndex)
			{
				if (ActiveCharacterIndex >= CharactersInSight.Num())
				{
					--ActiveCharacterIndex;
				}
				if (ActiveCharacterIndex > -1)
				{
					(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(true);
				}
			}
		}
	}
}
