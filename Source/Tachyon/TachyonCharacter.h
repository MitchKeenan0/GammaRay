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

	UFUNCTION(BlueprintCallable)
	void SetOpponent(ATachyonCharacter* NewTarget) { Opponent = NewTarget; }

	UFUNCTION(BlueprintCallable)
	void DonApparel();

	UFUNCTION(BlueprintCallable)
	void RequestBots();


	// ATTRIBUTES ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float BoostSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float BoostSustain = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxMoveSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly)
	float TurnSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float AttackFireRate = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float AttackAngle = 21.0f;

	UPROPERTY(EditDefaultsOnly)
	float AttackRecoil = 2100.0f;

	UPROPERTY(EditDefaultsOnly)
	float WindupTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Replicated)
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraMoveSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraDistanceScalar = 1.0f;

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
	float GetHealthDelta() { return FMath::Clamp(FMath::Abs(Health - MaxHealth), 0.1f, 50.0f); }

	UFUNCTION(BlueprintCallable)
	ATachyonCharacter* GetOpponent() { return Opponent; }


	// NETWORK FUNCTIONS ////////////////////////////////////////////////////////
	UFUNCTION()
	void ArmAttack();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerArmAttack();

	UFUNCTION()
	void ReleaseAttack();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerReleaseAttack();

	UFUNCTION()
	void WindupAttack(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerWindupAttack(float DeltaTime);

	UFUNCTION()
	void FireAttack();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerFireAttack();

	void UpdateAttack(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerUpdateAttack(float DeltaTime);

	void NullifyAttack();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerNullifyAttack();

	void SetX(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetX(float Value);

	void SetZ(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetZ(float Value);

	void UpdateBody(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerUpdateBody(float DeltaTime);

	void EngageJump();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerEngageJump();

	void DisengageJump();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerDisengageJump();

	void UpdateJump(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerUpdateJump(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void ReceiveKnockback(FVector Knockback, bool bOverrideVelocity);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerReceiveKnockback(FVector Knockback, bool bOverrideVelocity);

	/*UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastReceiveKnockback(FVector Knockback, bool bOverrideVelocity);*/

	UFUNCTION(BlueprintCallable)
	void ModifyHealth(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerModifyHealth(float Value);

	UFUNCTION(BlueprintCallable)
	void NewTimescale(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerNewTimescale(float Value);

	UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastNewTimescale(float Value);

	UFUNCTION(BlueprintCallable)
	void SetApparel(int ApparelIndex);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetApparel(int ApparelIndex);

	void RestartGame();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerRestartGame();

	UFUNCTION(NetMulticast, BlueprintCallable, reliable)
	void MulticastRestartGame();

	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AActor*> FramingActors;

	// MOVEMENT ///////////////////////////////////////////////////////////////
	void MoveRight(float Value);
	void MoveUp(float Value);

	// HEALTH & CAMERA ///////////////////////////////////////////////////////////////
	void UpdateHealth(float DeltaTime);
	void UpdateCamera(float DeltaTime);

	UPROPERTY(EditDefaultsOnly)
	class USceneComponent* AttackScene = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UParticleSystemComponent* AmbientParticles = nullptr;



	// REPLICATED VARIABLES ///////////////////////////////////////////////////////////////
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float InputX = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float InputZ = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float Charge = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float Health = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float AttackTimer = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float WindupTimer = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	bool bShooting = false;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	bool bJumping = false;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	FVector JumpMoveVector = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float DiminishingJumpValue = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	float BoostTimeAlive = 0.0f;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	int iApparelIndex = 0;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	class ATApparel* ActiveApparel = nullptr;
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly)
	class ATachyonCharacter* Opponent = nullptr;


	// ARMAMENT ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATachyonAttack> AttackClass;
	UPROPERTY(EditDefaultsOnly, Replicated)
	class ATachyonAttack* ActiveAttack = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> AttackWindupClass;
	UPROPERTY(EditDefaultsOnly, Replicated)
	class AActor* ActiveWindup = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> BoostClass;
	UPROPERTY(EditDefaultsOnly, Replicated)
	class AActor* ActiveBoost = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATApparel> ApparelClass;
	/*UPROPERTY(EditDefaultsOnly)
	class ATApparel* ActiveApparel = nullptr;*/
	
	
};
