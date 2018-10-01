// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
#include "GameFramework/Actor.h"
#include "TachyonAttack.generated.h"

class ATachyonCharacter;

UCLASS()
class TACHYON_API ATachyonAttack : public AActor
{
	GENERATED_BODY()

	void UpdateLifeTime(float DeltaT);
	void SpawnBurst();
	
public:	
	// Sets default values for this actor's properties
	ATachyonAttack();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	bool IsInitialized() { return bInitialized; }
	UFUNCTION()
	bool IsLockedEmitPoint() { return LockedEmitPoint; }


	/////////////////////////////////////////////////////////////////////////
	// Attack functions
	UFUNCTION()
	void InitAttack(AActor* Shooter, float Magnitude, float YScale);

	UFUNCTION()
	void RedirectAttack();

	UFUNCTION()
	void Lethalize();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnAttackBeginOverlap
	(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void RaycastForHit(FVector RaycastVector);


	/////////////////////////////////////////////////////////////////////////
	// Attributes
	UPROPERTY(EditDefaultsOnly)
	bool bSecondary = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool LockedEmitPoint = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DeliveryTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DurationTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MagnitudeTimeScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float HitsPerSecond = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bLethal = false;

	UPROPERTY(EditDefaultsOnly)
	float ShootingAngle = 21.0f;

	UPROPERTY(EditDefaultsOnly)
	float ProjectileSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bScaleProjectileSpeed = false;

	UPROPERTY(EditDefaultsOnly)
	float ProjectileMaxSpeed = 12000.0f;

	UPROPERTY(EditDefaultsOnly)
	float AngleSweep = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float RefireTime = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	float ChargeCost = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bUpdateAttack = true;

	UPROPERTY(EditDefaultsOnly)
	float FireDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float RaycastHitRange = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bRaycastOnMesh = false;

	UPROPERTY(EditDefaultsOnly)
	float KineticForce = 30000.0f;


	/////////////////////////////////////////////////////////////////////////
	// Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USceneComponent* AttackScene = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UCapsuleComponent* CapsuleComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UParticleSystemComponent* AttackParticles = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UPaperSpriteComponent* AttackSprite = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UProjectileMovementComponent* ProjectileComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> BurstClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> DamageClass = nullptr;

	/*UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UAudioComponent* AttackSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGDamage> DamageClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGDamage> BlockedClass = nullptr;*/


	/////////////////////////////////////////////////////////////////////////
	// Local Variables
	UPROPERTY(EditDefaultsOnly)
	float LifeTimer = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float HitTimer = 0.0f;


	/////////////////////////////////////////////////////////////////////////
	// Replicated Variables
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	bool bInitialized = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated)
	class AActor* OwningShooter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated)
	class AActor* HitActor = nullptr;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float DynamicLifetime = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float LethalTime = 0.2f;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	bool bHit = false;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	int NumHits = 1;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float AttackMagnitude = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float AttackDirection = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float AttackDamage = 1.0f;

	


	
	
};
