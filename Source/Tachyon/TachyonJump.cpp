// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonJump.h"
#include "TachyonCharacter.h"
#include "TimerManager.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"


// Sets default values
ATachyonJump::ATachyonJump()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	JumpScene = CreateDefaultSubobject<USceneComponent>(TEXT("JumpScene"));
	SetRootComponent(JumpScene);

	JumpParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("JumpParticles"));
	JumpParticles->SetupAttachment(RootComponent);

	SetReplicates(true);
	bReplicateMovement = true;
	NetDormancy = ENetDormancy::DORM_Never;
}

// Called when the game starts or when spawned
void ATachyonJump::BeginPlay()
{
	Super::BeginPlay();
	
	JumpParticles->Deactivate();

	TimeBetweenJumps = JumpFireDelay;
}


void ATachyonJump::StartJump()
{
	if (GetOwner() != nullptr)
	{
		float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		bool bCanJump = !GetWorldTimerManager().IsTimerPending(TimerHandle_TimeBetweenJumps) && !GetWorldTimerManager().IsTimerActive(TimerHandle_TimeBetweenJumps);
		if ((GlobalTimeScale == 1.0f) && bCanJump)
		{
			float FirstDelay = FMath::Max(LastJumpTime + TimeBetweenJumps - GetWorld()->TimeSeconds, 0.0f);

			GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenJumps, this, &ATachyonJump::Jump, TimeBetweenJumps, false, FirstDelay);
		}
	}
}

void ATachyonJump::EndJump()
{
	if (Role < ROLE_Authority)
	{
		ServerEndJump();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		ATachyonCharacter* MyDude = Cast<ATachyonCharacter>(MyOwner);
		if (MyDude != nullptr)
		{
			if (JumpParticles != nullptr)
			{
				JumpParticles->DeactivateSystem();
				JumpParticles->Deactivate();
			}

			MyDude->DisengageJump();
		}

		GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenJumps);
		GetWorldTimerManager().ClearTimer(TimerHandle_EndJumpTimer);
	}
}

void ATachyonJump::ServerEndJump_Implementation()
{
	EndJump();
}

bool ATachyonJump::ServerEndJump_Validate()
{
	return true;
}


void ATachyonJump::Jump()
{
	if (Role < ROLE_Authority)
	{
		ServerJump();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		ATachyonCharacter* MyDude = Cast<ATachyonCharacter>(MyOwner);
		if (MyDude != nullptr)
		{
			// Movement
			MyDude->EngageJump();

			// Angle
			FRotator VectorRotator = (MyDude->GetVelocity() + MyDude->GetActorForwardVector()).Rotation();
			SetActorRotation(VectorRotator);

			float JumpTimeScaledDuration = JumpDurationTime * (1.0f / MyDude->CustomTimeDilation);
			GetWorldTimerManager().SetTimer(TimerHandle_EndJumpTimer, this, &ATachyonJump::EndJump, JumpDurationTime, false);

			LastJumpTime = GetWorld()->TimeSeconds;
		}

		if (Role == ROLE_Authority)
		{
			DoJumpVisuals();
		}
	}
}

void ATachyonJump::ServerJump_Implementation()
{
	Jump();
}

bool ATachyonJump::ServerJump_Validate()
{
	return true;
}



// Called every frame
void ATachyonJump::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		CustomTimeDilation = MyOwner->CustomTimeDilation;
	}
}


void ATachyonJump::InitJump(FVector JumpDirection, ACharacter* Jumper)
{
	OwningJumper = Jumper;
	JumpVector = JumpDirection;
}

void ATachyonJump::DoJumpVisuals_Implementation()
{
	if ((JumpEffect != nullptr) && (JumpParticles != nullptr))
	{
		JumpParticles = UGameplayStatics::SpawnEmitterAttached(JumpEffect, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
		if (JumpParticles != nullptr)
		{
			//JumpParticles->bAutoDestroy = true;
			JumpParticles->SetIsReplicated(true);
			JumpParticles->CustomTimeDilation = 1.0f;
			JumpParticles->ComponentTags.Add("ResetKill");
		}
	}
}

// UNUSED /////
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

