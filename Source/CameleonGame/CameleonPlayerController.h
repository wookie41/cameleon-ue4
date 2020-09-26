// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CameleonPlayerController.generated.h"

UCLASS()
class CAMELEONGAME_API ACameleonPlayerController : public APlayerController
{
    GENERATED_BODY()
  public:
    ACameleonPlayerController();

    // Actor to spawn as controllable character marker //
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class AControllableCharacterMarker> MarkerClass;

    UPROPERTY(EditDefaultsOnly)
    float TransitionTimeSeconds = 1.5;

  protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void SetNextAsActive();

    UFUNCTION()
    void SetPreviousAsActive();

    UFUNCTION()
    void SwitchCharacter();

    UFUNCTION()
    void OnActorBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                             AActor* OtherActor,
                             UPrimitiveComponent* OtherComp,
                             int32 OtherBodyIndex,
                             bool bFromSweep,
                             const FHitResult& SweepResult);

    UFUNCTION()
    void OnActorEndOverlap(UPrimitiveComponent* OverlappedComponent,
                           AActor* OtherActor,
                           UPrimitiveComponent* OtherComp,
                           int32 OtherBodyIndex);

  private:
    // Maximal distance at which we can take control over a character //

    UPROPERTY()
    FVector ScanDistance = { 2500, 1000, 350 };

    // Map which holds controllable character in player's sight along with their markers //

    UPROPERTY()
    TMap<ACharacter*, class AControllableCharacterMarker*> ControllableCharacters;

    // Box used to determine what is in players view frustum and this what can he take control over //

    UPROPERTY()
    class UBoxComponent* CollisionComponent;

    UPROPERTY()
    int ActiveCharacterIndex = -1;

    UPROPERTY()
    TArray<ACharacter*> CharactersInSight;

    // Flag indicating if we can use the switch ability //

    UPROPERTY()
    bool bCanSwitch;

    UPROPERTY()
    bool bInTransition;

    UPROPERTY()
    float TransitionTimer;

    UPROPERTY()
    class UCameraComponent* CurrentCharacterCamera;
};