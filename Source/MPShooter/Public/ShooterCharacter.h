// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AShooterWeapon;
class USHealthComponent;

UCLASS()
class MPSHOOTER_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void BeginCrouch();

	void EndCrouch();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UShooterHealthComponent* HealthComp;

	bool bWantstoADS;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;

	float DefaultFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", Meta = (ClampMin = 0.1, ClampMax = 100))
	float ADSInterpSpeed;

	void BeginADS();

	void EndADS();

	AShooterWeapon* CurrentWeapon;

	void PullTrigger();

	void ReleaseTrigger();

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<AShooterWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

	void BeginReload();

	UFUNCTION()
	void OnHealthChanged(UShooterHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* DamageType,  AController* InstigatedBy, AActor* DamageCauser);

	// Flag if character died previously
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bDied;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;
};




