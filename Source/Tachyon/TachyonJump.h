// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TachyonJump.generated.h"

UCLASS()
class TACHYON_API ATachyonJump : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATachyonJump();

	void StartJump();

	void EndJump();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void InitJump(FVector JumpDirection, ACharacter* Jumper);

	UPROPERTY(EditDefaultsOnly)
	float JumpSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float JumpSustain = 1.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	float JumpDurationTime = 0.7f;

	UPROPERTY(EditDefaultsOnly)
	float JumpFireDelay = 0.1f;

	float LastJumpTime = 0.0f;
	FTimerHandle TimerHandle_TimeBetweenJumps;
	float TimeBetweenJumps = 0.0f;

	FTimerHandle TimerHandle_EndJumpTimer;

	void Jump();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerJump();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndJump();

	UPROPERTY(BlueprintReadWrite)
	float LifeTimer = 0.0f;

	UFUNCTION(NetMulticast, Reliable)
	void DoJumpVisuals();


	UFUNCTION()
	void UpdateJump(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class ACharacter* OwningJumper = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector JumpVector = FVector::ZeroVector;

	//////////////////////////////////////////////////////////////////
	// COMPONENTS
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USceneComponent* JumpScene = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UParticleSystemComponent* JumpParticles = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	UParticleSystem* JumpEffect;
	

	
	
};
