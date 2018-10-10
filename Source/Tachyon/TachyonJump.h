// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
#include "GameFramework/Actor.h"
#include "TachyonJump.generated.h"

UCLASS()
class TACHYON_API ATachyonJump : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATachyonJump();

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

	UFUNCTION()
	void UpdateJump(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated)
	class ACharacter* OwningJumper = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated)
	FVector JumpVector = FVector::ZeroVector;

	//////////////////////////////////////////////////////////////////
	// COMPONENTS
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USceneComponent* JumpScene = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UParticleSystemComponent* JumpParticles = nullptr;
	

	
	
};
