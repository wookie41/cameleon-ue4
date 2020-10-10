// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class IInteractable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	FVector GetInteractableLocation() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Interact();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void SetInteractableActive(bool Active);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    bool IsUseable() const;
};
