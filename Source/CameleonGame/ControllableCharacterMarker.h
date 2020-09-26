// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControllableCharacterMarker.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAMELEONGAME_API AControllableCharacterMarker : public AActor
{
	GENERATED_BODY()

public:
	AControllableCharacterMarker();

	// RootComponent //
	UPROPERTY(VisibleDefaultsOnly, Category = Marker)
	class USphereComponent* SphereComponent;

	// Mesh used to represent the marker //
	UPROPERTY(EditDefaultsOnly, Category = Marker)
	class UStaticMeshComponent* MeshComponent;

	// Material to use then the marker is active//
	UPROPERTY(EditDefaultsOnly, Category = Marker)
	class UMaterialInterface* ActiveMaterial;

	// Default material to use for the marker //
	UPROPERTY(EditDefaultsOnly, Category = Marker)
	class UMaterialInterface* DefaultMaterial;

	void SetActive(const bool& Active);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
