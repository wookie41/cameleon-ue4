// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameleonGameProjectile.generated.h"

UCLASS(config=Game)
class ACameleonGameProjectile : public AActor
{
    GENERATED_BODY()

    /** Sphere collision component */
    UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
    class USphereComponent* CollisionComp;

public:
	ACameleonGameProjectile();
};

