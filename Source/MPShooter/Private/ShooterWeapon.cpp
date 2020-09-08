// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterWeapon.h"
#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AShooterWeapon::AShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(Root);

	MuzzleSocketName = "MuzzleFlashSocket";
	TracerTargetName = "BeamEnd";
}

// Called when the game starts or when spawned
void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AShooterWeapon::PullTrigger()
{
	AActor* WeaponOwner = GetOwner();
	if (WeaponOwner)
	{
		// Spawn MuzzleFlash if set
		if (MuzzleEffect)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		}

		FHitResult Hit;
		FVector ShotDirection;
		FVector TraceEnd;
		FVector TracerEndPoint;
		
		bool HitSuccess = LineTrace(Hit, ShotDirection, TraceEnd, TracerEndPoint);
		if (HitSuccess)
		{
			// Apply damage to Actor receiving hit
			AActor* HitActor = Hit.GetActor();
			if (HitActor)
			{
				UGameplayStatics::ApplyPointDamage(HitActor, 20.0f, ShotDirection, Hit, WeaponOwner->GetInstigatorController(), this, DamageType);

				// Spawn ImpactEffect if set
				if (ImpactEffect)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				}
			}
		}

		// Spawn Tracer effect if set
		if (TracerEffect)
		{
			FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

			UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
			if (TracerComp)
			{
				TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
			}
		}
	}
}

bool AShooterWeapon::LineTrace(FHitResult& Hit, FVector& ShotDirection, FVector& TraceEnd, FVector& TracerEndPoint)
{
	AActor* WeaponOwner = GetOwner();
	if (WeaponOwner)
	{
		// Use Viewpoint of player camera
		FVector EyeLocation;
		FRotator EyeRotation;
		WeaponOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		ShotDirection = EyeRotation.Vector();
		TraceEnd = EyeLocation + (ShotDirection * 10000);
		TracerEndPoint = TraceEnd;

		// Set collision parameters
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(WeaponOwner); // Ignore owning actor of weapon
		QueryParams.AddIgnoredActor(this); // Ignore mesh of weapon
		QueryParams.bTraceComplex = true; // Trace against each individual triangle of mesh hit


		bool HitResult = GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECollisionChannel::ECC_Visibility, QueryParams);
		if (HitResult)
		{
			TracerEndPoint = Hit.ImpactPoint;
		}

		return HitResult;
	}

	return false;
}

// Called every frame
void AShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

