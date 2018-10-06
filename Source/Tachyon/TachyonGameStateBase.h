// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tachyon.h"
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


	UFUNCTION()
	void SetGlobalTimescale(float TargetTimescale);

	UFUNCTION()
	void UpdateGlobalTimescale(float DeltaTime);

	UPROPERTY()
	bool bRecoverTimescale = true;
	
	
};
