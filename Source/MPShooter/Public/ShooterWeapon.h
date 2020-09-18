// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeapon.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;

// Contains hitscan info from a line trace 
// to be sent from client-to-client for spawning of effects
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;
	
	// Vector packing with 0 decimal point precision
	UPROPERTY()
	FVector_NetQuantize TraceEnd;

	UPROPERTY()
	uint8 ReplicationCount;
};

UCLASS()
class MPSHOOTER_API AShooterWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShooterWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	UParticleSystem* TracerEffect;
	
	bool LineTrace(FHitResult& Hit, FVector& ShotDirection, FVector& TraceEnd, FVector& TracerEndPoint);

	void SpawnTracerEffect(FVector TracerEndPoint);

	void SpawnImpactEffect(EPhysicalSurface SurfaceType, FVector& ImpactPoint);

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	float BaseDamage;

	virtual void PullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginReload();

	void CamShakeOnFire();

	FTimerHandle TimerHandle_TimeBetweenShots;

	float LastFireTime;

	// Amount of bullets fired per minute by weapon (RPM)
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	float RateOfFire;

	// Derived from RateOfFire
	float TimeBetweenShots;
	
	// @TODO: Ammo Implementation & link with animations

	UPROPERTY(EditDefaultsOnly, Category = "Weapons", Meta = (ClampMin = 0, ClampMax = 500))
	int MaxLoadedAmmo;

	int LoadedAmmo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons", Meta = (ClampMin = 0, ClampMax = 500))
	int MaxReserveAmmo;

	int ReserveAmmo;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
	void OnRep_HitScanTrace();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void BeginFire();

	void EndFire();
	
	void BeginReload();

};
