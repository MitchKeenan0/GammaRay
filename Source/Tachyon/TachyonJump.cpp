// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonJump.h"


// Sets default values
ATachyonJump::ATachyonJump()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	JumpScene = CreateDefaultSubobject<USceneComponent>(TEXT("JumpScene"));
	SetRootComponent(JumpScene);

	JumpParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("JumpParticles"));
	JumpParticles->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATachyonJump::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATachyonJump::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ((OwningJumper != nullptr)
		&& (JumpVector != FVector::ZeroVector))
	{
		UpdateJump(DeltaTime);
		ForceNetUpdate();
	}
}


void ATachyonJump::InitJump(FVector JumpDirection, ACharacter* Jumper)
{
	OwningJumper = Jumper;
	JumpVector = JumpDirection;
}


void ATachyonJump::UpdateJump(float DeltaTime)
{
	float JumpTimeAlive = GetGameTimeSinceCreation();
	if (JumpTimeAlive > 0.001f)
	{
		float InverseTimeAlive = (1.0f / JumpTimeAlive) * JumpSustain;
		float DiminishingJumpValue = FMath::Clamp(InverseTimeAlive, 0.0f, 21.0f);
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("DiminishingJumpValue: %f"), DiminishingJumpValue));

		if (DiminishingJumpValue > 0.5f)
		{
			FVector SustainedJump = JumpVector * JumpSpeed * DiminishingJumpValue * 1000.0f;
			FVector JumpLocation = OwningJumper->GetActorLocation() + SustainedJump;
			float JumpSize = SustainedJump.Size();
			FVector UpdatedJumpLocation = FMath::VInterpConstantTo(OwningJumper->GetActorLocation(), JumpLocation, DeltaTime, JumpSize);
			OwningJumper->SetActorLocation(UpdatedJumpLocation);
		}
	}
}


// NETWORK VARIABLES
void ATachyonJump::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATachyonJump, OwningJumper);
	DOREPLIFETIME(ATachyonJump, JumpVector);
}

