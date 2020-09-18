// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterWeapon.h"
#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "MPShooter.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AShooterWeapon::AShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup Scene and Mesh components
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(Root);

	// Initialize FNames
	MuzzleSocketName = "MuzzleFlashSocket";
	TracerTargetName = "BeamEnd";

	// Initialize base damage and rate of fire
	BaseDamage = 8.66f;
	RateOfFire = 600;

	// Initialize Ammo counts for loaded and reserve
	MaxReserveAmmo = 210;
	MaxLoadedAmmo = 30;
	LoadedAmmo = MaxLoadedAmmo;
	ReserveAmmo = MaxReserveAmmo;

	// Replicates spawning of gun to clients attached to server
	SetReplicates(true);

	// Update tick rate	for network updates
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

// Called when the game starts or when spawned
void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	TimeBetweenShots = 60 / RateOfFire;
}

// Called every frame
void AShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AShooterWeapon::PullTrigger()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerPullTrigger();
	}

	AActor* WeaponOwner = GetOwner();
	if (WeaponOwner && LoadedAmmo-- > 0)
	{
		FHitResult Hit;
		FVector ShotDirection;
		FVector TraceEnd;
		FVector TracerEndPoint;
		EPhysicalSurface SurfaceType = SurfaceType_Default;
		
		bool HitSuccess = LineTrace(Hit, ShotDirection, TraceEnd, TracerEndPoint);
		if (HitSuccess)
		{
			// Apply damage to Actor receiving hit
			AActor* HitActor = Hit.GetActor();
			if (HitActor)
			{
				
				// Get SurfaceType on Hit
				SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

				// Check if Hit was headshot and apply damage multiplier if true
				float ActualDamage = BaseDamage;
				if (SurfaceType == SURFACE_FLESHVULNERABLE)
				{
					ActualDamage *= 2.5f;
				}

				UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, WeaponOwner->GetInstigatorController(), this, DamageType);
			
				SpawnImpactEffect(SurfaceType, Hit.ImpactPoint);
			}
		}

		SpawnTracerEffect(TracerEndPoint);
		CamShakeOnFire();

		// Update struct when shot on server for clients to spawn tracer effects
		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceEnd = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
			HitScanTrace.ReplicationCount++;
		}

		LastFireTime = GetWorld()->TimeSeconds;
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
		QueryParams.bReturnPhysicalMaterial = true;

		bool HitResult = GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams);
		if (HitResult)
		{
			TracerEndPoint = Hit.ImpactPoint;
		}

		return HitResult;
	}

	return false;
}

void AShooterWeapon::SpawnTracerEffect(FVector TracerEndPoint)
{
	// Spawn MuzzleFlash if set
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
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


void AShooterWeapon::CamShakeOnFire()
{
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void AShooterWeapon::SpawnImpactEffect(EPhysicalSurface SurfaceType, FVector& ImpactPoint) 
{

	UParticleSystem* ResultEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		ResultEffect = FleshImpactEffect;
		break;
	default:
		ResultEffect = DefaultImpactEffect;
		break;
	}

	if (ResultEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ResultEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void AShooterWeapon::BeginFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AShooterWeapon::PullTrigger, TimeBetweenShots, true, FirstDelay);

}


void AShooterWeapon::EndFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void AShooterWeapon::BeginReload()
{

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerBeginReload();
	}

	if (ReserveAmmo <= 0 || LoadedAmmo == MaxLoadedAmmo) return;

	if (ReserveAmmo < (MaxLoadedAmmo - LoadedAmmo))
	{
		LoadedAmmo += ReserveAmmo;
		ReserveAmmo = 0;
	}
	else
	{
		ReserveAmmo -= (MaxLoadedAmmo - LoadedAmmo);
		LoadedAmmo = MaxLoadedAmmo;
	}

}

void AShooterWeapon::ServerPullTrigger_Implementation()
{
	PullTrigger();
}

bool AShooterWeapon::ServerPullTrigger_Validate()
{
	return true;
}

void AShooterWeapon::ServerBeginReload_Implementation()
{
	BeginReload();
}

bool AShooterWeapon::ServerBeginReload_Validate()
{
	return true;
}

void AShooterWeapon::OnRep_HitScanTrace()
{
	// Play Visual Effects
	SpawnTracerEffect(HitScanTrace.TraceEnd);
	SpawnImpactEffect(HitScanTrace.SurfaceType, HitScanTrace.TraceEnd);
}

void AShooterWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate to everyone except owner to avoid playing effects twice
	DOREPLIFETIME_CONDITION(AShooterWeapon, HitScanTrace, COND_SkipOwner);
}