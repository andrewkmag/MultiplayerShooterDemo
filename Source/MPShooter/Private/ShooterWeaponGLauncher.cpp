// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterWeaponGLauncher.h"
#include "Components/SkeletalMeshComponent.h"

void AShooterWeaponGLauncher::PullTrigger()
{
	AActor* WeaponOwner = GetOwner();
	if (WeaponOwner && ProjectileClass)
	{
		// Use Viewpoint of player camera
		FVector EyeLocation;
		FRotator EyeRotation;
		WeaponOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
	}
}