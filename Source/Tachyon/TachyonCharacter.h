// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
#include "TachyonAttack.h"
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
	UPROPERTY()
	float X = 0.0f;
	UPROPERTY()
	float Z = 0.0f;
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

public:
	// Sets default values for this character's properties
	ATachyonCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void UpdateHealth(float DeltaTime);
	void UpdateBody(float DeltaTime);
	void UpdateCamera(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void NullifyAttack() { ActiveAttack = nullptr; }


	// ATTRIBUTES ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxMoveSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly)
	float TurnSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraMoveSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraDistanceScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraVelocityChase = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float CameraSoloVelocityChase = 5.0f;

	UPROPERTY(EditDefaultsOnly)
	float ChargeMax = 4.0f;

	UFUNCTION(BlueprintCallable)
	float GetAPM() { return APM; }


	// NETWORK FUNCTIONS ////////////////////////////////////////////////////////
	UFUNCTION()
	void FireAttack();
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerFireAttack();

	void UpdateAttack(float DeltaTime);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerUpdateAttack(float DeltaTime);

	void SetX(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetX(float Value);

	void SetZ(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerSetZ(float Value);


	UFUNCTION(BlueprintCallable)
	void ModifyHealth(float Value);
	UFUNCTION(Server, BlueprintCallable, reliable, WithValidation)
	void ServerModifyHealth(float Value);
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AActor*> FramingActors;

	// MOVEMENT ///////////////////////////////////////////////////////////////
	void MoveRight(float Value);
	void MoveUp(float Value);



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


	// ARMAMENT ///////////////////////////////////////////////////////////////
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATachyonAttack> AttackClass;

	UPROPERTY(EditDefaultsOnly)
	class ATachyonAttack* ActiveAttack = nullptr;
	
	
};
