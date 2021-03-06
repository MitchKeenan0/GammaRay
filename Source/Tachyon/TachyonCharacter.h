// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
#include "TachyonAttack.h"
#include "TachyonPlayerState.h"
#include "TApparel.h"
#include "GameFramework/Character.h"
#include "TachyonCharacter.generated.h"


UCLASS()
class TACHYON_API ATachyonCharacter : public ACharacter
{
	GENERATED_BODY()


	// CAMERA
	// Side view camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	class UCameraComponent* SideViewCameraComponent;

	// Camera boom positioning the camera beside the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY()
	UParticleSystemComponent* Aimer = nullptr;

	// PRIVATE VARIABLES
	UPROPERTY()
	float APM = 0.0f;
	/*UPROPERTY()
	float X = 0.0f;
	UPROPERTY()
	float Z = 0.0f;*/
	UPROPERTY()
	float MoveTimer = 0.0f;
	UPROPERTY()
	bool bSpawnedDeath = false;
	UPROPERTY()
	FVector PositionOne = FVector::ZeroVector;
	UPROPERTY()
	FVector PositionTwo = FVector::ZeroVector;
	UPROPERTY()
	FVector Midpoint = FVector::ZeroVector;
	UPROPERTY()
	FVector PrevMoveInput = FVector::ZeroVector;
	UPROPERTY()
	AActor* Actor1 = nullptr;
	UPROPERTY()
	AActor* Actor2 = nullptr;

public:
	// Sets default values for this character's properties
	ATachyonCharacter(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	USceneComponent* GetAttackScene() { return AttackScene; }

	UFUNCTION()
	float GetX() { return InputX; }

	UFUNCTION()
	float GetZ() { float Z = ((Controller != nullptr) && IsLocallyControlled()) ? InputZ : 0.0f; return Z; }

	UFUNCTION()
	void SetX(float Value) { InputX = Value; }

	UFUNCTION()
	void SetZ(float Value) { InputZ = Value; }

	UFUNCTION(BlueprintCallable)
	void SetOpponent(ATachyonCharacter* NewTarget) { Opponent = NewTarget; }

	UFUNCTION(BlueprintCallable)
	void SetDynamicMoveSpeed();

	UFUNCTION(BlueprintCallable)
	void SetWorldRange(float InRange);

	UFUNCTION(BlueprintCallable)
	void SetTimescaleRecoverySpeed(float Value);

	/*UFUNCTION(BlueprintCallable)
	void DonApparel();*/

	UFUNCTION(BlueprintCallable)
	void RequestBots();

	UFUNCTION()
	void BotMove(float X, float Z);


	// ATTRIBUTES ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float FightSpeedInfluence = 2.1f;

	UPROPERTY(EditDefaultsOnly)
	float BoostSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float BoostSustain = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxMoveSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly)
	float TurnSpeed = 3600.0f;

	UPROPERTY(EditDefaultsOnly)
	float BrakeStrength = 15.0f;

	UPROPERTY(EditDefaultsOnly)
	float WorldRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly)
	float RecoverStrength = 2.1f;

	UPROPERTY(EditDefaultsOnly)
	float AttackFireRate = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float AttackAngle = 21.0f;

	UPROPERTY(EditDefaultsOnly)
	float AttackRecoil = 2100.0f;

	UPROPERTY(EditDefaultsOnly)
	float AttackDrag = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float WindupTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Replicated)
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, Replicated)
	float MaxTimescale = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float TimescaleRecoverySpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraMoveSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraDistanceScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraFOVScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraVelocityChase = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraSoloVelocityChase = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UCameraShake> CameraMoveShake = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float ChargeMax = 4.0f;

	UFUNCTION(BlueprintCallable)
	float GetAPM() { return APM; }

	UFUNCTION(BlueprintCallable)
	float GetHealth() { return Health; }

	UFUNCTION(BlueprintCallable)
	float GetMaxHealth() { return MaxHealth; }

	UFUNCTION(BlueprintCallable)
	void SetMaxTimescale(float Value);

	UFUNCTION(BlueprintCallable)
	float GetMaxTimescale();

	UFUNCTION(BlueprintCallable)
	float GetHealthDelta() { return FMath::Clamp(FMath::Abs(Health - MaxHealth), 0.1f, 50.0f); }

	UFUNCTION(BlueprintCallable)
	ATachyonCharacter* GetOpponent() { return Opponent; }


	// NETWORK FUNCTIONS ////////////////////////////////////////////////////////

	UFUNCTION()
	void StartFire();

	UFUNCTION()
	void EndFire();

	UFUNCTION()
	void StartJump();

	UFUNCTION()
	void EndJump();

	UFUNCTION()
	void Shield();

	void UpdateBody(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerUpdateBody(float DeltaTime);

	UFUNCTION()
	void EngageJump();

	UFUNCTION()
	void DisengageJump();

	UFUNCTION()
	void StartBrake();

	UFUNCTION()
	void EndBrake();

	UFUNCTION()
	void Recover();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerRecover();

	UFUNCTION(BlueprintCallable)
	void ReceiveKnockback(FVector Knockback, bool bOverrideVelocity);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerReceiveKnockback(FVector Knockback, bool bOverrideVelocity);
	UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastReceiveKnockback(FVector Knockback, bool bOverrideVelocity);

	UFUNCTION(BlueprintCallable)
	void ModifyHealth(float Value, bool Lethal);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerModifyHealth(float Value, bool Lethal);

	UFUNCTION(BlueprintCallable)
	void NewTimescale(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerNewTimescale(float Value);
	UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastNewTimescale(float Value);

	void RestartGame();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerRestartGame();
	UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastRestartGame();

	/*UFUNCTION(BlueprintCallable)
	void SetApparel(int ApparelIndex);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetApparel(int ApparelIndex);*/

	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SpawnAbilities();

	// MOVEMENT ///////////////////////////////////////////////////////////////
	void MoveRight(float Value);
	void MoveUp(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float InputX = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float InputZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LastFaceDirection = 0.0f;

	// HEALTH & CAMERA ///////////////////////////////////////////////////////////////
	void UpdateHealth(float DeltaTime);
	void UpdateCamera(float DeltaTime);

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AActor*> FramingActors;

	// COMPONENTS ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	class USceneComponent* AttackScene = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UParticleSystemComponent* AmbientParticles = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UPointLightComponent* PointLight = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UAudioComponent* SoundComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* OuterTouchCollider;

	// Shield collision
	UFUNCTION()
	void OnShieldBeginOverlap
	(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(BlueprintCallable)
	void Collide(AActor* OtherActor);



	// REPLICATED VARIABLES ///////////////////////////////////////////////////////////////
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Charge = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AttackTimer = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WindupTimer = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bShooting = false;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	float Health = 0.0f;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	class ATachyonCharacter* Opponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NearDeath")
	UParticleSystem* NearDeathEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recover")
	UParticleSystem* RecoverEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SurfaceReaction")
	UParticleSystem* SurfaceHitEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SurfaceReaction")
	UParticleSystem* CollideEffect = nullptr;


	//UPROPERTY(Replicated, BlueprintReadOnly)
	//int iApparelIndex = 0;
	//UPROPERTY(Replicated, BlueprintReadOnly)
	//class ATApparel* ActiveApparel = nullptr;


	// ARMAMENT ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATachyonAttack> AttackClass;
	UPROPERTY(Replicated)
	class ATachyonAttack* ActiveAttack = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> AttackWindupClass;
	UPROPERTY(Replicated)
	class AActor* ActiveWindup = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> BoostClass;
	UPROPERTY(Replicated)
	class ATachyonJump* ActiveBoost = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATachyonAttack> SecondaryClass;
	UPROPERTY(Replicated)
	class ATachyonAttack* ActiveSecondary = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATApparel> ApparelClass;
	/*UPROPERTY(EditDefaultsOnly)
	class ATApparel* ActiveApparel = nullptr;*/

	UPROPERTY(Replicated)
	class UParticleSystemComponent* ActiveDeath = nullptr;
	
	
};
