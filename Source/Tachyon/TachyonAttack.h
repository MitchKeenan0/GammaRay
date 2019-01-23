// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TachyonAttack.generated.h"

class ATachyonCharacter;

UCLASS()
class TACHYON_API ATachyonAttack : public AActor
{
	GENERATED_BODY()

	void UpdateLifeTime(float DeltaT);
	void SpawnBurst();

	UPROPERTY()
	bool bGameEnder = false;

	
public:	
	// Sets default values for this actor's properties
	ATachyonAttack();

	void StartFire();

	void EndFire();

	void RemoteHit(AActor* Target, float Damage);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void InitAttack();

	UFUNCTION()
	void ActivateSound();

	UFUNCTION()
	UParticleSystemComponent* ActivateParticles();

	UFUNCTION()
	void SetShooterInputEnabled(bool bEnabled);

	UFUNCTION()
	bool IsInitialized() { return bInitialized; }

	UFUNCTION()
	bool IsLockedEmitPoint() { return LockedEmitPoint; }

	UFUNCTION()
	void CallForTimescale(AActor* TargetActor, bool bGlobal, float NewTimescale);

	UFUNCTION()
	void Neutralize();

	UFUNCTION()
	void ReceiveTimescale(float InTimescale);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	float LastFireTime = 0.0f;
	FTimerHandle TimerHandle_TimeBetweenShots;
	float TimeBetweenShots = 0.0f;
	float TimeAtInit = 0.0f;

	FTimerHandle TimerHandle_DeliveryTime;

	FTimerHandle TimerHandle_Neutralize;

	FTimerHandle TimerHandle_Raycast;

	UPROPERTY()
	AActor* CurrentBurstObject = nullptr;

	UFUNCTION()
	void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UPROPERTY(BlueprintReadWrite)
	float LifeTimer = 0.0f;

	UFUNCTION(NetMulticast, Reliable) /// 
	void ActivateEffects();

	UPROPERTY()
	float ActualDeliveryTime = 0.0f;

	UPROPERTY()
	float ActualHitsPerSecond = 0.0f;

	UPROPERTY()
	float ActualAttackDamage = 0.0f;

	UPROPERTY()
	float ActualLethalTime = 0.0f;

	UPROPERTY()
	float ActualDurationTime = 0.0f;

	/////////////////////////////////////////////////////////////////////////
	// Attack functions
	UFUNCTION()
	void RaycastForHit();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRaycastForHit();

	UFUNCTION()
	void RedirectAttack();

	UFUNCTION()
	void Lethalize();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLethalize();

	UFUNCTION()
	void SetInitVelocities();

	void MainHit(AActor* HitActor, FVector HitLocation);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMainHit(AActor* HitActor, FVector HitLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNeutralize();

	UFUNCTION()
	void ReportHitToMatch(AActor* Shooter, AActor* Mark);

	UFUNCTION()
	void SpawnHit(AActor* HitActor, FVector HitLocation);

	UFUNCTION()
	void ApplyKnockForce(AActor* HitActor, FVector HitLocation, float HitScalar);

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


	/////////////////////////////////////////////////////////////////////////
	// Attributes
	UPROPERTY(EditDefaultsOnly)
	bool bSecondary = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool LockedEmitPoint = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector WindupPullDirection = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DeliveryTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RedirectionTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DurationTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float LethalTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float AttackDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TimescaleImpact = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float GivenMagnitude = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TimeExtendOnHit = 0.015f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float HitsPerSecond = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float HitsPerSecondDecay = 0.333f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ShooterSlow = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float HitSlow = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAbsoluteHitForce = true;

	UPROPERTY(EditDefaultsOnly)
	float ShootingAngle = 21.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ProjectileSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float ProjectileDrag = 0.0f;

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
	float RaycastHitRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bRaycastOnMesh = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float KineticForce = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RecoilForce = 1000.0f;


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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class URadialForceComponent* AttackRadial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UAudioComponent* AttackSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* AttackEffect;

	/////////////////////////////////////////////////////////////////////////
	// Spawned Resources
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> BurstClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> DamageClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UCameraShake> FireShake = nullptr;

	/*

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGDamage> BlockedClass = nullptr;
	
	*/


	/////////////////////////////////////////////////////////////////////////
	// Replicated Variables
	UPROPERTY(BlueprintReadWrite)
	bool bInitialized = false;
	
	UPROPERTY(BlueprintReadWrite)
	class AActor* OwningShooter = nullptr;
	UPROPERTY(BlueprintReadWrite)
	class AActor* HitActor = nullptr;
	UPROPERTY(BlueprintReadWrite)
	float DynamicLifetime = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	class AActor* KilledActor = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	float HitTimer = 0.0f;
	UPROPERTY(BlueprintReadWrite)
	bool bHit = false;
	UPROPERTY(BlueprintReadWrite)
	int NumHits = 0;
	UPROPERTY(BlueprintReadWrite)
	float AttackMagnitude = 0.0f;
	UPROPERTY(BlueprintReadWrite)
	float AttackDirection = 0.0f;
	
	UPROPERTY(BlueprintReadWrite)
	bool bLethal = false;
	UPROPERTY(BlueprintReadWrite)
	bool bDoneLethal = false;
	UPROPERTY(BlueprintReadWrite)
	bool bFirstHitReported = false;
	UPROPERTY(BlueprintReadWrite)
	bool bNeutralized = false;

	/*UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float NetDeliveryTime = 0.1f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadWrite)
	float NetHitRate = 25.0f;*/

	


	
	
};
