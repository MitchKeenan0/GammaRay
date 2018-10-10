// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonGameStateBase.h"


ATachyonGameStateBase::ATachyonGameStateBase()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}


void ATachyonGameStateBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bRecoverTimescale)
	{
		UpdateGlobalTimescale(DeltaTime);
	}
}


void ATachyonGameStateBase::SetGlobalTimescale(float TargetTimescale)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TargetTimescale);

	// End-game vs recovery
	if (TargetTimescale <= 0.01f)
	{
		bRecoverTimescale = false;
	}
	else
	{
		bRecoverTimescale = true;
	}

	ForceNetUpdate();
}


void ATachyonGameStateBase::UpdateGlobalTimescale(float DeltaTime)
{
	float CurrentTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (CurrentTime > 1.0f)
	{
		bRecoverTimescale = false;
		SetGlobalTimescale(1.0f);
	}
	else
	{
		float InterpTime = FMath::FInterpConstantTo(CurrentTime, 1.0f, DeltaTime, 11.0f);
		SetGlobalTimescale(InterpTime);
	}
}


void ATachyonGameStateBase::SpawnBot(FVector SpawnLocation)
{
	FActorSpawnParameters SpawnParams;
	FRotator Rotation = FRotator::ZeroRotator;
	TSubclassOf<ATachyonCharacter> TachyonSpawning = Tachyons[0];
	TSubclassOf<ATachyonAIController> AISpawning = Controllers[0];
	
	// Spawn the body
	ATachyonCharacter* NewTachyon = 
		Cast<ATachyonCharacter>
			(GetWorld()->SpawnActor<AActor>
				(TachyonSpawning, SpawnLocation, Rotation, SpawnParams));
	
	// Possess with AI controller
	if (NewTachyon != nullptr)
	{
		ATachyonAIController* NewController =
			Cast<ATachyonAIController>
				(GetWorld()->SpawnActor<AActor>
					(AISpawning, SpawnLocation, Rotation, SpawnParams));

		if (NewController != nullptr)
		{
			NewTachyon->Controller = NewController;
			NewController->Possess(NewTachyon);
		}
	}
}

