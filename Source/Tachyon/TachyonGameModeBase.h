// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TApparel.h"
#include "GameFramework/GameModeBase.h"
#include "TachyonGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TACHYON_API ATachyonGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<TSubclassOf<ATApparel>> Skins;

};
