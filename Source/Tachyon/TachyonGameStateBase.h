// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TachyonAIController.h"
#include "GameFramework/GameStateBase.h"
#include "TachyonGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class TACHYON_API ATachyonGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> TimescalingPlayers;
	
public:
	ATachyonGameStateBase();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly)
	float TimescaleRecoverySpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float GGTimescale = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ATachyonCharacter>> Tachyons;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ATachyonAIController>> Controllers;

	UFUNCTION(BlueprintCallable)
	void SpawnBot(FVector SpawnLocation);

	UFUNCTION()
	void SetGlobalTimescale(float TargetTimescale);

	UFUNCTION()
	void SetActorTimescale(AActor* TargetActor, float TargetTimescale);

	UFUNCTION()
	void RestartGame();

	UFUNCTION()
	void UpdateGlobalTimescale(float DeltaTime);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	class USceneComponent* GameStateScene = nullptr;

	UPROPERTY(Replicated)
	bool bRecoverTimescale = false;

	UPROPERTY(Replicated)
	bool bGG = false;
	
	UPROPERTY(Replicated)
	float DesiredTimescale = 1.0f;

	// Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UAudioComponent* GameSound = nullptr;
	
};
