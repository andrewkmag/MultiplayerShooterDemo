// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterTrackerAI.h"
#include "ShooterCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Components/ShooterHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "CollisionQueryParams.h"

// Sets default values
AShooterTrackerAI::AShooterTrackerAI()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup Mesh component
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetSimulatePhysics(true);

	// Create Health component and subscribe to it
	HealthComp = CreateDefaultSubobject<UShooterHealthComponent>(TEXT("Health"));
	HealthComp->OnHealthChanged.AddDynamic(this, &AShooterTrackerAI::HandleTakeDamage);

	// Set Movement attributes
	bUseVelocityChange = true;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	// Set Explosion attributes
	ExplosionDamage = 40;
	ExplosionRadius = 200;

	// Setup Sphere component for detecting when near player
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(ExplosionRadius);
	SphereComp->SetupAttachment(RootComponent);
	// Setup collision to only query and not handle physics
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Setup collision response to ignore on every channel
	// except when overlapping Pawn
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Set Max Power Level of potential explosion damage
	MaxPowerLevel = 4;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void AShooterTrackerAI::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// Find initial move to location
		NextPathPoint = GetNextPathPoint();
	
		// Every second we update our power-level based on nearby bots
		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &AShooterTrackerAI::OnCheckNearbyAI, 1.0f, true);
	}
}

// Called every frame
void AShooterTrackerAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		// close to target get path point
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();
			DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!");
		}
		else
		{
			// Apply force to get closer to target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.GetSafeNormal();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			Mesh->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Red, false, 0.0f, 0, 1.0f);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Red, false, 0.0f, 1.0f);
	}
}

void AShooterTrackerAI::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestruct && !bExploded)
	{
		AShooterCharacter* PlayerPawn = Cast<AShooterCharacter>(OtherActor);

		if (PlayerPawn)
		{

			if (HasAuthority())
			{
				// Begin timer for explosion sequence
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &AShooterTrackerAI::DamageSelf, 0.5f, true, 0.0f);
			}

			bStartedSelfDestruct = true;
			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}

}

void AShooterTrackerAI::HandleTakeDamage(UShooterHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// Pulse the material on Hit
	// Create a dynamic instance if not set
	if (MatInst == nullptr)
	{
		MatInst = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mesh->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue(TEXT("LastTimeDamageTaken"), GetWorld()->TimeSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());

	// Explode on health = 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

void AShooterTrackerAI::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}
	
	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	// Hide Mesh visibility and disable collision
	Mesh->SetVisibility(false, true);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority())
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		// Increase Damage based on power level (amount of other TrackerAIs nearby)
		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		// Apply Damage
		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true, ECC_Pawn);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);

		// Dont destroy immediately & give client enough time to spawn effect
		SetLifeSpan(2.0f);
	}
}

FVector AShooterTrackerAI::GetNextPathPoint()
{
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
	GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &AShooterTrackerAI::RefreshPath, 5.0f, false);
	
	if (NavPath->PathPoints.Num() > 1)
	{
		return NavPath->PathPoints[1];
	}

	return GetActorLocation();
}

void AShooterTrackerAI::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void AShooterTrackerAI::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

void AShooterTrackerAI::OnCheckNearbyAI()
{
	// Distance to check for nearby AI
	const float Radius = 600;

	// Create temporary collision shape to handle overlaps
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	// Only find Pawns (eg. Player, other AI)
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);

	// Check amount of overlaps that match queried object type
	int NumAI = 0;
	for (FOverlapResult Result : Overlaps)
	{
		// Check if overlapped with another TrackerAI
		AShooterTrackerAI* TrackerAI = Cast<AShooterTrackerAI>(Result.GetActor());
		
		if (TrackerAI && TrackerAI != this)
		{
			NumAI++;
		}
	}

	// Clamp between min = 0 and MaxPowerLevel
	PowerLevel = FMath::Clamp((float)NumAI, 0.0f, MaxPowerLevel);

	// Update material color
	if (MatInst == nullptr)
	{
		MatInst = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mesh->GetMaterial(0));
	}

	if (MatInst)
	{
		float Alpha = PowerLevel / (float) MaxPowerLevel;
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);

		// Variable to be sent to clients
		AlphaClients = Alpha;

		OnRep_PowerLevel();
	}

	DrawDebugString(GetWorld(), FVector(0,0,0), FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);

}

void AShooterTrackerAI::OnRep_PowerLevel()
{
	MatInst = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mesh->GetMaterial(0));
	if (MatInst)
	{
		MatInst->SetScalarParameterValue("PowerLevelAlpha", AlphaClients);
	}
}

void AShooterTrackerAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterTrackerAI, PowerLevel);
	DOREPLIFETIME(AShooterTrackerAI, AlphaClients);
}