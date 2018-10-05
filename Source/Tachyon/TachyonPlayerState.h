// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tachyon.h"
#include "TApparel.h"
#include "GameFramework/PlayerState.h"
#include "TachyonPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class TACHYON_API ATachyonPlayerState : public APlayerState
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<TSubclassOf<ATApparel>> Skins;
	
};
