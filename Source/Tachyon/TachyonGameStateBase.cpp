// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonGameStateBase.h"
//#include "PostProcessVolume.h"


ATachyonGameStateBase::ATachyonGameStateBase()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	GameStateScene = CreateDefaultSubobject<USceneComponent>(TEXT("GameStateScene"));
	SetRootComponent(GameStateScene);

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

	if (!bGG && (DesiredTimescale == GGTimescale) && !GameSound->IsPlaying())
	{
		GameSound->Play();
		bGG = true;

		// Auto restart
		/*FTimerHandle TimerHand;
		GetWorldTimerManager().SetTimer(TimerHand, this, &ATachyonGameStateBase::RestartGame, 0.15f, false);*/
	}
}


void ATachyonGameStateBase::SetGlobalTimescale(float TargetTimescale)
{
	float IncomingTimeClamped = FMath::Clamp(TargetTimescale, GGTimescale, 1.0f);
	DesiredTimescale = IncomingTimeClamped;
	
	// End-game vs recovery
	if (DesiredTimescale <= GGTimescale)
	{
		bRecoverTimescale = false;
	}
	else if (DesiredTimescale == 1.0f)
	{
		bRecoverTimescale = true;
	}

	ForceNetUpdate();
}


void ATachyonGameStateBase::SetActorTimescale(AActor* TargetActor, float TargetTimescale)
{
	ATachyonCharacter* TargetTachyon = Cast<ATachyonCharacter>(TargetActor);
	if (TargetTachyon != nullptr)
	{
		TargetTachyon->NewTimescale(TargetTimescale);
	}
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
					if (Role == ROLE_Authority)
						Attack->Neutralize();
				}
			}
		}
	}

	// Clear doomed actors
	TArray<AActor*> Doomies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("ResetKill"), Doomies);
	if (Doomies.Num() > 0)
	{
		int DoomNum = Doomies.Num();
		for (int i = 0; i < DoomNum; ++i)
		{
			AActor* ThisDoomie = Doomies[i];
			ThisDoomie->Destroy();
		}
	}

	// Clear dead Bots
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Bot"), Doomies);
	{
		if (Doomies.Num() > 0)
		{
			int DoomNum = Doomies.Num();
			for (int i = 0; i < DoomNum; ++i)
			{
				AActor* ThisBot = Doomies[i];
				ATachyonCharacter* BotPlayer = Cast<ATachyonCharacter>(ThisBot);
				if (BotPlayer != nullptr)
				{
					if (BotPlayer->GetHealth() <= 0.0f)
					{
						BotPlayer->Destroy();
					}
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
					if (Role == ROLE_Authority)
					{
						Player->ModifyHealth(100.0f);
						Player->NewTimescale(1.0f);
						//Player->ForceNetUpdate();
					}

					Player->GetCharacterMovement()->Velocity *= 0.1f;
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
	
	// Interpolating to desired timescale
	float FactoredTimescale = DesiredTimescale * 10.0f;
	float InterpSpeed = TimescaleRecoverySpeed + (1.0f / FactoredTimescale);
	if (DesiredTimescale == GGTimescale)
	{
		InterpSpeed *= 0.5f;
	}
	float InterpTime = FMath::FInterpConstantTo(CurrentTime, DesiredTimescale, DeltaTime, InterpSpeed);
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), InterpTime);
	ForceNetUpdate();
}


void ATachyonGameStateBase::SpawnBot(FVector SpawnLocation)
{
	// Temp hardcode, just spawning 1st type for now
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
			NewTachyon->Tags.Add("Bot");
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("ran SpawnBot"));
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

