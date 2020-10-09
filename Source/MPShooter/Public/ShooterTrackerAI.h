// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShooterTrackerAI.generated.h"

class UShooterHealthComponent;
class USphereComponent;
class USoundCue;

UCLASS()
class MPSHOOTER_API AShooterTrackerAI : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AShooterTrackerAI();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UShooterHealthComponent* HealthComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USphereComponent* SphereComp;

	UFUNCTION()
	void HandleTakeDamage(UShooterHealthComponent* HealthComponent, float Health, float HealthDelta, 
							const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
	
	FVector GetNextPathPoint();

	// Next point in navigation path
	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	float RequiredDistanceToTarget;

	// Dynamic material to pulse on damage
	UMaterialInstanceDynamic* MatInst;

	void SelfDestruct();

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	UParticleSystem* ExplosionEffect;

	bool bExploded;

	bool bStartedSelfDestruct;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	float ExplosionDamage;
	
	FTimerHandle TimerHandle_SelfDamage;

	void DamageSelf();

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	USoundCue* ExplodeSound;
	void RefreshPath();

	FTimerHandle TimerHandle_RefreshPath;

	void OnCheckNearbyAI();

	UPROPERTY(EditDefaultsOnly, Category = "TrackerAI")
	float MaxPowerLevel;

	UPROPERTY(ReplicatedUsing=OnRep_PowerLevel, EditDefaultsOnly, Category = "TrackerAI")
	float PowerLevel;

	UFUNCTION()
	void OnRep_PowerLevel();

	UPROPERTY(Replicated)
	float AlphaClients;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

};
