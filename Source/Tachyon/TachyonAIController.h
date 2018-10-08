// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TachyonCharacter.h"
#include "AIController.h"
#include "TachyonAIController.generated.h"

/**
 * 
 */
UCLASS()
class TACHYON_API ATachyonAIController : public AAIController
{
	GENERATED_BODY()
	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void AquirePlayer();

	UFUNCTION()
	void FindOneself();

public:
	UFUNCTION(BlueprintCallable)
	void SetPlayer(ATachyonCharacter* NewPlayer) { Player = NewPlayer; }

	UPROPERTY(BlueprintReadWrite)
	class ATachyonCharacter* Player = nullptr;
	
	UFUNCTION(BlueprintCallable)
	FVector GetNewLocationTarget();

	UFUNCTION(BlueprintCallable)
	void NavigateTo(FVector TargetLocation);

	UFUNCTION(BlueprintCallable)
	void Combat(AActor* TargetActor);

	UFUNCTION(BlueprintCallable)
	void AimAtTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable)
	bool ReactionTimeIsNow(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	bool HasViewToTarget(AActor* TargetActor, FVector TargetLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	APawn* MyPawn = nullptr;

	UPROPERTY(EditDefaultsOnly)
	ATachyonCharacter* MyTachyonCharacter = nullptr;

	UPROPERTY(EditDefaultsOnly)
	FVector LocationTarget = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly)
	FVector MoveInput = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly)
	bool bViewToTarget = false;

	UPROPERTY(EditDefaultsOnly)
	bool bShootingSoon = false;

	UPROPERTY(EditDefaultsOnly)
	bool bCourseLayedIn = false;

	UPROPERTY(EditDefaultsOnly)
	float ReactionTimer = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float MoveTimer = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float TravelTimer = 0.0f;
	
};