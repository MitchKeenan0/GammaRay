// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAttackWarmup.h"


// Sets default values
ATachyonAttackWarmup::ATachyonAttackWarmup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WarmupScene = CreateDefaultSubobject<USceneComponent>(TEXT("WarmupScene"));
	SetRootComponent(WarmupScene);

	WarmupParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("WarmupParticles"));
	WarmupParticles->SetupAttachment(RootComponent);

	WarmupSound = CreateDefaultSubobject<UAudioComponent>(TEXT("WarmupSound"));
	WarmupSound->SetupAttachment(RootComponent);

	bReplicates = true;
	bReplicateMovement = true;
	WarmupSound->SetIsReplicated(true);
}

// Called when the game starts or when spawned
void ATachyonAttackWarmup::BeginPlay()
{
	Super::BeginPlay();
	
	InitWarmup();
}

// Called every frame
void ATachyonAttackWarmup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateWarmup(DeltaTime);
}

void ATachyonAttackWarmup::InitWarmup()
{
	if (WarmupSound != nullptr)
	{
		WarmupSound->SetVolumeMultiplier(InitialVolume);
		WarmupSound->SetPitchMultiplier(InitialPitch);
	}
}

void ATachyonAttackWarmup::UpdateWarmup(float DeltaTime)
{

	// Scaling sound
	if (WarmupSound != nullptr)
	{
		float CurrentPitch = WarmupSound->PitchMultiplier;
		float InterpPitch = FMath::FInterpTo(CurrentPitch, MaxPitch, DeltaTime, (GainSpeed + CurrentPitch));
		float CurrentVolume = WarmupSound->VolumeMultiplier;
		float InterpVolume = FMath::FInterpConstantTo(CurrentVolume, MaxVolume, DeltaTime, (GainSpeed + CurrentVolume));
		WarmupSound->SetPitchMultiplier(InterpPitch);
		WarmupSound->SetVolumeMultiplier(InterpVolume);
	}
}

