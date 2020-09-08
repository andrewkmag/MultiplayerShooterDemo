// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "ShooterWeaponGLauncher.generated.h"

/**
 * 
 */
UCLASS()
class MPSHOOTER_API AShooterWeaponGLauncher : public AShooterWeapon
{
	GENERATED_BODY()
	
protected:
	virtual void PullTrigger() override;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectileWeapons")
	TSubclassOf<AActor> ProjectileClass;
};
