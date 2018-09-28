// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
#include "GameFramework/Actor.h"
#include "TachyonAttackWarmup.generated.h"

UCLASS()
class TACHYON_API ATachyonAttackWarmup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATachyonAttackWarmup();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void InitWarmup();
	void UpdateWarmup(float DeltaTime);

	// Attributes
	UPROPERTY(EditDefaultsOnly)
	float GainSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float InitialSize = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxSize = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float InitialPitch = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxPitch = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float InitialVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxVolume = 1.0f;

	// Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USceneComponent* WarmupScene = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UParticleSystemComponent* WarmupParticles = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UAudioComponent* WarmupSound = nullptr;
	
	
};
