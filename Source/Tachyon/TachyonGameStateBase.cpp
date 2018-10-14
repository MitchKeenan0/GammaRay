// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonGameStateBase.h"


ATachyonGameStateBase::ATachyonGameStateBase()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	GameSound = CreateDefaultSubobject<UAudioComponent>(TEXT("GameSound"));

	bReplicates = true;
	NetDormancy = ENetDormancy::DORM_Never;
}


void ATachyonGameStateBase::BeginPlay()
{
	Super::BeginPlay();
	
	SetGlobalTimescale(1.0f);
}


void ATachyonGameStateBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateGlobalTimescale(DeltaTime);

	if (!bGG && (DesiredTimescale == 0.01f) && !GameSound->IsPlaying())
	{
		GameSound->Play();
		bGG = true;
	}
}


void ATachyonGameStateBase::SetGlobalTimescale(float TargetTimescale)
{
	DesiredTimescale = TargetTimescale;
	
	// End-game vs recovery
	if (DesiredTimescale == 0.01f)
	{
		bRecoverTimescale = false;
	}
	else if (DesiredTimescale <= 1.0f)
	{
		bRecoverTimescale = true;
	}

	ForceNetUpdate();
}


void ATachyonGameStateBase::RestartGame()
{
	// Neutralize all weapons
	TArray<AActor*> Weapons;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATachyonAttack::StaticClass(), Weapons);
	if (Weapons.Num() > 0)
	{
		int NumWeapons = Weapons.Num();
		for (int i = 0; i < NumWeapons; ++i)
		{
			AActor* AttackActor = Weapons[i];
			if (AttackActor != nullptr)
			{
				ATachyonAttack* Attack = Cast<ATachyonAttack>(AttackActor);
				if (Attack != nullptr)
				{
					Attack->Neutralize();
				}
			}
		}
	}

	// Reset player lifepoints
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATachyonCharacter::StaticClass(), Players);
	if (Players.Num() > 0)
	{
		int NumPlayers = Players.Num();
		for (int i = 0; i < NumPlayers; ++i)
		{
			AActor* PlayerActor = Players[i];
			if (PlayerActor != nullptr)
			{
				ATachyonCharacter* Player = Cast<ATachyonCharacter>(PlayerActor);
				if (Player != nullptr)
				{
					Player->ModifyHealth(100.0f);
					Player->ForceNetUpdate();
				}
			}
		}
	}

	// To-do... Place the players a reasonable distance apart

	// Reset timescale
	SetGlobalTimescale(1.0f);
	bGG = false;
}


void ATachyonGameStateBase::UpdateGlobalTimescale(float DeltaTime)
{
	float CurrentTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if ((CurrentTime == DesiredTimescale)
		&& (CurrentTime != 0.01f))
	{
		DesiredTimescale = 1.0f;
		return;
	}

	// Interpolating to desired timescale
	float FactoredTimescale = DesiredTimescale * 10.0f;
	float InterpSpeed = 10.0f + (1.0f / FactoredTimescale);
	if (DesiredTimescale == 0.01f)
	{
		InterpSpeed *= 0.5555f;
	}
	float InterpTime = FMath::FInterpConstantTo(CurrentTime, DesiredTimescale, DeltaTime, InterpSpeed);
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), InterpTime);
	ForceNetUpdate();
}


void ATachyonGameStateBase::SpawnBot(FVector SpawnLocation)
{
	if (HasAuthority())
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
}


////////////////////////////////////////////////////////////////////////
// NETWORK REPLICATION
void ATachyonGameStateBase::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATachyonGameStateBase, bGG);
	DOREPLIFETIME(ATachyonGameStateBase, bRecoverTimescale);
	DOREPLIFETIME(ATachyonGameStateBase, DesiredTimescale);
	
}

