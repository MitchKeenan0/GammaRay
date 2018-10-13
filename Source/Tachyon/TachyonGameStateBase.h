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

	
public:
	ATachyonGameStateBase();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ATachyonCharacter>> Tachyons;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ATachyonAIController>> Controllers;

	UFUNCTION(BlueprintCallable)
	void SpawnBot(FVector SpawnLocation);

	UFUNCTION()
	void SetGlobalTimescale(float TargetTimescale);

	UFUNCTION()
	void RestartGame();

	UFUNCTION()
	void UpdateGlobalTimescale(float DeltaTime);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	bool bRecoverTimescale = false;

	UPROPERTY(Replicated)
	bool bGG = false;
	
	
};
