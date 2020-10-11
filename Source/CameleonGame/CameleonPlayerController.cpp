#include "CameleonPlayerController.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "ControllableCharacterMarker.h"
#include "Interactable.h"
#include "GameplayTagContainer.h"
#include "CameleonGameCharacter.h"

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
	InputComponent->BindAction("UseInteractable", IE_Pressed, this, &ACameleonPlayerController::UseInteractable);
	InputComponent->BindAction("ToggleScan", IE_Pressed, this, &ACameleonPlayerController::ToggleScanAbility);
}

void ACameleonPlayerController::BeginPlay()
{
    Super::BeginPlay();

	auto queryExpression = FGameplayTagQueryExpression()
		.AllTagsMatch()
		.AddTag(FGameplayTag::RequestGameplayTag("Controllable"));

	ControllableCharacterQuery.Build(queryExpression);
	
	CollisionComponent->SetHiddenInGame(false);

	if (const auto character = GetCharacter())
	{
		if (const auto camera = character->FindComponentByClass<UCameraComponent>())
		{
			CurrentCharacterCamera = camera;
			CollisionComponent->AttachToComponent(camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SetViewTarget(character);
		}
	}

	FScriptDelegate overlapBeginDelegate;
	FScriptDelegate overlapEndDelegate;

	overlapBeginDelegate.BindUFunction(this, FName("OnActorBeginOverlap"));
	overlapEndDelegate.BindUFunction(this, FName("OnActorEndOverlap"));

	//@TODO Remove once we have some nice effect to show the scan region
	SetActorHiddenInGame(false);
	CollisionComponent->SetHiddenInGame(true);

	CollisionComponent->SetBoxExtent(ScanDistance);
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->OnComponentBeginOverlap.Add(overlapBeginDelegate);
	CollisionComponent->OnComponentEndOverlap.Add(overlapEndDelegate);
}

void ACameleonPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Camera transition
	if (bInTransition)
	{
		TransitionTimer += DeltaSeconds;

		if (TransitionTimer > TransitionTimeSeconds)
		{
			Possess(CharactersInSight[ActiveCharacterIndex]);
			EnableInput(this);

			ClearControllableCharacters();
			ActiveAInteractable = nullptr;

			bInTransition = false;
			bCanSwitch = true;

			Interactables.Empty();
			if (auto activeInteractable = Cast<IInteractable>(ActiveAInteractable))
			{
				activeInteractable->Execute_SetInteractableActive(ActiveAInteractable, false);
			}

			CollisionComponent->SetGenerateOverlapEvents(true);
		}
	}

	// Interactables

	auto playerCharacter = GetCharacter();

	if (!bInTransition && playerCharacter)
	{
		float largestDot = 0;
		AActor* activeInteractableActor = nullptr;
		AActor* lastActiveInteractableActor = ActiveAInteractable;

		// Find the interactable that the player is facing by calculating the dot product
		// of the eye vector and the direction to the interactable

		FVector eyesPos;
		FRotator viewRotation;
		playerCharacter->GetActorEyesViewPoint(eyesPos, viewRotation);

		auto eyeVector = CurrentCharacterCamera->GetForwardVector();

		for (auto interactableActor : Interactables)
		{
			auto interactable = Cast<IInteractable>(interactableActor);
			auto interactablePos = interactable->Execute_GetInteractableLocation(interactableActor);
			auto toInteractable = (interactablePos - eyesPos).GetUnsafeNormal();
			auto currentDot = FVector::DotProduct(eyeVector, toInteractable);

			if (activeInteractableActor == nullptr || currentDot > largestDot)
			{
				largestDot = currentDot;
				activeInteractableActor = interactableActor;
			}
		}

		if (activeInteractableActor)
		{
			// if the active interactable has changed
			if (activeInteractableActor != lastActiveInteractableActor)
			{
				// disable the last interactable if we had one
				if (lastActiveInteractableActor)
				{
					Cast<IInteractable>(lastActiveInteractableActor)->Execute_SetInteractableActive(
						lastActiveInteractableActor, false);
				}

				// enable the newly picked one
				Cast<IInteractable>(activeInteractableActor)->Execute_SetInteractableActive(
					activeInteractableActor, true);
			}

			ActiveAInteractable = activeInteractableActor;
		}
	}
}

void ACameleonPlayerController::AddInteractable(AActor* aInteractable)
{
	if (aInteractable->Implements<UInteractable>() &&
		Cast<IInteractable>(aInteractable)->Execute_IsUseable(aInteractable))
	{
		Interactables.Add(aInteractable);
	}
}

void ACameleonPlayerController::RemoveInteractable(AActor* aInteractable)
{
	if (aInteractable == ActiveAInteractable)
	{
		ActiveAInteractable = nullptr;
	}
	Cast<IInteractable>(aInteractable)->Execute_SetInteractableActive(aInteractable, false);
	Interactables.Remove(aInteractable);
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
		const auto characterToUse = CharactersInSight[ActiveCharacterIndex];
		// sanity check if characters is behind us

		const auto playerLocation = GetCharacter()->GetActorLocation();
		const auto characterLocation = characterToUse->GetActorLocation();
		const auto toCharacterDir = (characterLocation - playerLocation).GetUnsafeNormal();
		const auto dot = FVector::DotProduct(GetCharacter()->GetActorForwardVector(), toCharacterDir);

		if (dot < 0)
		{
			// character is behin
			return;
		}

		// check if something is not in the way

		if (!CanWeSee(characterToUse))
		{
			return;
		}

		auto mesh = characterToUse->GetMesh();
		auto camera = characterToUse->FindComponentByClass<UCameraComponent>();

		if (mesh && camera)
		{
			DisableInput(this);
			UnPossess();

			bCanSwitch = false;
			bInTransition = true;
			TransitionTimer = 0;

			CollisionComponent->AttachToComponent(camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			CollisionComponent->SetGenerateOverlapEvents(false);

			SetViewTargetWithBlend(characterToUse, TransitionTimeSeconds);
			CurrentCharacterCamera = camera;
		}
	}
}

void ACameleonPlayerController::UseInteractable()
{
	if (ActiveAInteractable)
	{
		auto interactable = Cast<IInteractable>(ActiveAInteractable);
		interactable->Execute_Interact(ActiveAInteractable);

		// Remove the interactable if it can't be used anymore
		if (!interactable->Execute_IsUseable(ActiveAInteractable))
		{
			interactable->Execute_SetInteractableActive(ActiveAInteractable, false);
			Interactables.Remove(ActiveAInteractable);
			ActiveAInteractable = nullptr;
		}
	}
}

void ACameleonPlayerController::ToggleScanAbility()
{
	// Don't allow to toggle the ability while in transition
	if (bInTransition)
	{
		return;
	}

	// Reset the ability if it's currently active
	if (CollisionComponent->GetGenerateOverlapEvents())
	{
		bCanSwitch = false;
		//@TODO Remove once we have some nice effect to show the scan region
		CollisionComponent->SetHiddenInGame(true);
		CollisionComponent->SetGenerateOverlapEvents(false);
		ClearControllableCharacters();
	}
	else
	{
		bCanSwitch = true;
		//@TODO Remove once we have some nice effect to show the scan region
		CollisionComponent->SetHiddenInGame(false);
		CollisionComponent->SetGenerateOverlapEvents(true);
	}
}

void ACameleonPlayerController::OnActorBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                    AActor* OtherActor,
                                                    UPrimitiveComponent* OtherComp,
                                                    int32 OtherBodyIndex,
                                                    bool bFromSweep,
                                                    const FHitResult& SweepResult)
{
	// Ignore overlaps when changing characters

	if (bInTransition)
	{
		return;
	}
	auto overlappedCharacter = Cast<ACameleonGameCharacter>(OtherActor);

	if (!overlappedCharacter)
	{
		return;
	}

	//sanity check

	if (overlappedCharacter == GetCharacter())
	{
		return;
	}

	FGameplayTagContainer characterTags;
	overlappedCharacter->GetOwnedGameplayTags(characterTags);
	if (!CanWeSee(overlappedCharacter) || !ControllableCharacterQuery.Matches(characterTags))
	{
		return;
	}

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
		marker->SetActive(false);

		ControllableCharacters.Add(overlappedCharacter, marker);

		// Add the actor to the array holding actors in our sight so we can switch if there are multiple 
		// The array holds the character in order from the closest to the one most far away, so we need
		// to find a place for the newly overlapped character

		float distanceToOverlappedCharacter = (overlappedCharacter->GetActorLocation() - GetCharacter()->
			GetActorLocation()).Size();

		int overlappedCharacterIndex = -1;
		for (int characterIdx = 0; characterIdx < CharactersInSight.Num(); ++characterIdx)
		{
			float distToCharacter = (CharactersInSight[characterIdx]->GetActorLocation() - GetCharacter()->
				GetActorLocation()).Size();
			if (distToCharacter > distanceToOverlappedCharacter)
			{
				overlappedCharacterIndex = characterIdx;
				CharactersInSight.EmplaceAt(overlappedCharacterIndex, overlappedCharacter);
				if (overlappedCharacterIndex == ActiveCharacterIndex)
				{
					++ActiveCharacterIndex;
				}

				break;
			}
		}

		// This is the case when there are no characters in sight
		// or the new character is behind all of the currently in sight characters

		if (overlappedCharacterIndex == -1)
		{
			CharactersInSight.Add(overlappedCharacter);

			// Make the character active if it is the only one
			if (ActiveCharacterIndex < 0)
			{
				ActiveCharacterIndex = 0;
				marker->SetActive(true);
			}
		}
	}
}

void ACameleonPlayerController::OnActorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Ignore overlaps in collision

	if (bInTransition)
	{
		return;
	}

	if (auto character = Cast<ACameleonGameCharacter>(OtherActor)) // make sure the overlapped actor is a character
	{
		// Sanity check
		if (character == GetCharacter())
		{
			return;
		}

		// Get a marker for the overlapped character to Destroy() it
		auto marker = ControllableCharacters.Find(character);
		if (marker)
		{
			// Find the index of the overlapped character and remove it //

			int removedCharacterIndex = CharactersInSight.IndexOfByPredicate([character](ACharacter* It)
			{
				return character == It;
			});

			CharactersInSight.RemoveAt(removedCharacterIndex);

			// Adjust the active character index if the character that we've lost sight of
			// was closer to us than the active character
			if (ActiveCharacterIndex > removedCharacterIndex)
			{
				--ActiveCharacterIndex;
			}
			else if (removedCharacterIndex == ActiveCharacterIndex)
			{
				// if the removed character was active and if yes, then mark next as active

				if (ActiveCharacterIndex >= CharactersInSight.Num())
				{
					--ActiveCharacterIndex;
				}
				if (ActiveCharacterIndex > -1)
				{
					(*ControllableCharacters.Find(CharactersInSight[ActiveCharacterIndex]))->SetActive(true);
				}
			}

			(*marker)->Destroy();
			ControllableCharacters.Remove(character);
		}
	}
}

void ACameleonPlayerController::ClearControllableCharacters()
{
	// Destroy all of the markers

	for (auto it = ControllableCharacters.CreateIterator(); it; ++it)
	{
		it.Value()->Destroy();
	}

	ControllableCharacters.Empty();
	CharactersInSight.Empty();
	ActiveCharacterIndex = -1;
}

bool ACameleonPlayerController::CanWeSee(const ACharacter* OtherCharacter) const
{
	FHitResult hitResult;
	GetWorld()->LineTraceSingleByChannel(hitResult,
	                                     CurrentCharacterCamera->GetComponentLocation() +
	                                     CurrentCharacterCamera->GetForwardVector() * 100,
	                                     OtherCharacter->GetActorLocation(), ECC_Camera);

	return OtherCharacter == hitResult.Actor;
}
