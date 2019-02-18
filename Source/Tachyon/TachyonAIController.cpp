// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAIController.h"



void ATachyonAIController::BeginPlay()
{
	Super::BeginPlay();

	bSetControlRotationFromPawnOrientation = false;
}

void ATachyonAIController::FindOneself()
{
	MyPawn = GetPawn();
	if (MyPawn != nullptr)
	{
		MyTachyonCharacter = Cast<ATachyonCharacter>(MyPawn);
		if (MyTachyonCharacter != nullptr)
		{
			if (!MyTachyonCharacter->Tags.Contains("Bot"))
			{
				MyTachyonCharacter->Tags.Add("Bot");
			}

			MyTachyonCharacter->Tags.Add("FramingActor");

			//MyTachyonCharacter->GetCharacterMovement()->MaxAcceleration = 5000.0f;
			//MyTachyonCharacter->GetCharacterMovement()->MaxFlySpeed = 1000.0f;
			//MyTachyonCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
			//MyTachyonCharacter->GetCharacterMovement()->BrakingFrictionFactor = 50.0f;

			MyTachyonCharacter->bUseControllerRotationYaw = true;
			MyTachyonCharacter->bUseControllerRotationPitch = true;
			MyTachyonCharacter->bUseControllerRotationRoll = true;
		}
	}
}

void ATachyonAIController::AquirePlayer()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATachyonCharacter::StaticClass(), OutActors);
	if (OutActors.Num() > 0)
	{
		int NumActors = OutActors.Num();
		for (int i = 0; i < NumActors; ++i)
		{
			AActor* CurrentActor = OutActors[i];
			if (CurrentActor != nullptr)
			{
				ATachyonCharacter* CurrentTachyon = Cast<ATachyonCharacter>(CurrentActor);
				if (CurrentTachyon != nullptr)
				{
					if ((CurrentTachyon != MyTachyonCharacter) && (!CurrentActor->ActorHasTag("Surface")))
					{
						Player = CurrentTachyon;
						return;
					}
				}
			}
		}
	}
}


void ATachyonAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ((MyTachyonCharacter != nullptr)
		&& (MyTachyonCharacter->GetHealth() > 0.0f))
	{
		if (Player != nullptr)
		{
			float GlobalTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
			if (GlobalTime == 1.0f)
			{
				// Movement
				if ((LocationTarget == FVector::ZeroVector) || (TravelTimer > 2.0f))
				{
					LocationTarget = GetNewLocationTarget();
					TravelTimer = 0.0f;
				}
				else if (LocationTarget != FVector::ZeroVector)
				{
					NavigateTo(LocationTarget);
					TravelTimer += DeltaTime;
				}

				Combat(Player, DeltaTime);
			}
		}
		else
		{
			AquirePlayer();
		}
	}
	else
	{
		FindOneself();
	}
}

FVector ATachyonAIController::GetNewLocationTarget()
{
	FVector Result = FVector::ZeroVector;

	if ((MyTachyonCharacter != nullptr) && (Player != nullptr))
	{
		FVector PlayerTargetPostion = FVector::ZeroVector;
		FVector PlayerLocation = Player->GetActorLocation();
		FVector PlayerVelocity = Player->GetCharacterMovement()->Velocity;
		FVector MyVelocity = MyTachyonCharacter->GetCharacterMovement()->Velocity;
		float VelocityScalar = 
			MoveRange 
			+ FMath::Clamp(
				(1.0f / (1.0f / MyVelocity.Size() + 1.0f)),
				1.0f, 
				100.0f);
		
		if (PlayerVelocity.Size() > 100.0f)
		{
			PlayerVelocity.Z *= 0.5f;
			PlayerTargetPostion = PlayerLocation + (PlayerVelocity * 0.9f);
		}
		else
		{
			PlayerTargetPostion = PlayerLocation;
		}

		FVector RandomOffset = FMath::VRand() * VelocityScalar;
		RandomOffset.Y = 0.0f;
		RandomOffset.Z *= 0.15f;

		Result = PlayerTargetPostion + RandomOffset;
		bCourseLayedIn = true;
	}

	return Result;
}


void ATachyonAIController::NavigateTo(FVector TargetLocation)
{
	FVector MyLocation = MyTachyonCharacter->GetActorLocation();
	FVector ToTarget = TargetLocation - MyLocation;

	// Reached target already?
	if (ToTarget.Size() < 150.0f)
	{
		LocationTarget = FVector::ZeroVector;
		bCourseLayedIn = false;
		return;
	}
	else
	{
		float VerticalDistance = ToTarget.Z;
		float LateralDistance = ToTarget.X;

		MyInputX = FMath::Clamp(LateralDistance * 100000.0f, -1.0f, 1.0f);
		MyInputZ = FMath::Clamp(VerticalDistance * 100000.0f, -1.0f, 1.0f);

		if (bJumping || bAttacking)
		{
			FVector MyPlayerAtSpeed = MyTachyonCharacter->GetActorLocation() + MyTachyonCharacter->GetVelocity();
			float DistToDest = FVector::Dist(MyPlayerAtSpeed, LocationTarget);
			if (DistToDest > (Aggression * 550.0f))
			{
				MyTachyonCharacter->BotMove(MyInputX, MyInputZ);
			}
			else
			{
				MyTachyonCharacter->EndJump();
				bJumping = false;
				JumpTimer = 0.0f;
			}
		}

		float DeltaTime = GetWorld()->DeltaTimeSeconds;
		//MyTachyonCharacter->UpdateBody(DeltaTime);

		// Set rotation so character faces direction of travel
		float TravelDirection = FMath::Clamp(MyInputX, -1.0f, 1.0f);
		float ClimbDirection = FMath::Clamp(MyInputZ * 5.0f, -5.0f, 5.0f);
		float Roll = FMath::Clamp(MyInputZ * 25.1, -25.1, 25.1);

		FRotator BodyRotation = FRotator::ZeroRotator;

		if (TravelDirection < 0.0f)
		{
			BodyRotation = FRotator(ClimbDirection, 180.0f, Roll);
		}
		else if (TravelDirection > 0.0f)
		{
			BodyRotation = FRotator(ClimbDirection, 0.0f, -Roll);
		}

		// No lateral Input - finish rotation
		else
		{
			if (FMath::Abs(GetControlRotation().Yaw) > 90.0f)
			{
				BodyRotation = FRotator(ClimbDirection, 180.0f, -Roll);
			}
			else if (FMath::Abs(GetControlRotation().Yaw) < 90.0f)
			{
				BodyRotation = FRotator(ClimbDirection, 0.0f, Roll);
			}
		}

		SetControlRotation(BodyRotation);

		// Braking
		FVector MyVelocity = MyTachyonCharacter->GetCharacterMovement()->Velocity;
		if (MyVelocity.Size() > 10.0f)
		{
			FVector VelNorm = MyVelocity.GetSafeNormal();
			FVector DestNorm = ToTarget.GetSafeNormal();
			float DotToTarget = FVector::DotProduct(VelNorm, DestNorm);
			if (DotToTarget < 0.0f)
			{
				MyTachyonCharacter->StartBrake();
			}
			else
			{
				MyTachyonCharacter->EndBrake();
			}
		}

		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Pitch: %f"), GetControlRotation().Pitch));
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("ClimbDirection: %f"), ClimbDirection));
	}
}


void ATachyonAIController::Combat(AActor* TargetActor, float DeltaTime)
{
	FVector LocalForward = MyTachyonCharacter->GetAttackScene()->GetForwardVector();
	FVector ToTarget = TargetActor->GetActorLocation() - MyTachyonCharacter->GetActorLocation();
	float RangeToTarget = ToTarget.Size();

	

	// Aim - leads to attacks and secondaries////////////////////////
	if ((RangeToTarget <= 3000.0f) && MyTachyonCharacter->WasRecentlyRendered(0.5f))
	{

		/// initializing charge
		if (ReactionTimeIsNow(DeltaTime))
		{
			
			// Recover
			float MyMaxTimescale = MyTachyonCharacter->GetMaxTimescale();
			float MyHealth = MyTachyonCharacter->GetHealth();
			if (MyMaxTimescale < 0.88f)
			{
				float Worth = MyHealth - (MyMaxTimescale * 100.0f);
				float HealthRiskRewardPercent = (10.0f / MyHealth); /// 100=0.1  50=0.2  30=0.3  10=1
				float ChancePercent = (FMath::FRand() * HealthRiskRewardPercent) * Aggression * 15.0f;
				///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("Chancu %f"), ChancePercent));
				if ((ChancePercent > 1.0f)
					|| (Worth < -20.0f))
				{
					MyTachyonCharacter->Recover();
					return;
				}
			}


			// Aim
			MyTachyonCharacter->BotMove(MyInputX, MyInputZ);

			FVector ForwardNorm = MyTachyonCharacter->GetCharacterMovement()->Velocity.GetSafeNormal();
			FVector ToPlayerNorm = ToTarget.GetSafeNormal();

			float VerticalNorm = FMath::FloorToFloat(FMath::Clamp((ToTarget.GetSafeNormal()).Z, -1.0f, 1.0f));
			float DotToTarget = FVector::DotProduct(ForwardNorm, ToPlayerNorm);
			
			MyInputZ = ToTarget.Z;
			MyInputX = ToTarget.X;

			// Attacking
			if (!bAttacking)
			{
				MyTachyonCharacter->StartFire();
				bAttacking = true;
			}
			else
			{
				ShootingChargeTimer += (DeltaTime * (1.0f / MyTachyonCharacter->CustomTimeDilation));
				///GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::White, FString::Printf(TEXT("ShootingChargeTimer: %f"), ShootingChargeTimer));
			}

			// Only fire if a) we're on screen & b) angle looks good
			float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DotToTarget));
			if (AngleToTarget <= 50.0f)  // && MyCharacter->WasRecentlyRendered(0.15f)
			{
				/// release attack
				float AttackChargeThreshold = FMath::RandRange(0.1f, Aggression);
				if (ShootingChargeTimer >= AttackChargeThreshold)
				{
					MyTachyonCharacter->EndFire();

					ShootingChargeTimer = 0.0f;
					bAttacking = false;
					TimeAtLastShotFired = GetWorld()->TimeSeconds;

					///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("FIRED"));
				}

				// Shielding
				if (!bShielding)
				{
					if (FMath::Sqrt(FMath::RandRange(0.0f, 1.0f)) >= 0.5f)
					{
						MyTachyonCharacter->Shield();
						bShielding = true;
					}
				}

				if (bShielding && bAttacking)
				{
					bShielding = false;
				}
			}
		}
	}

	// Boost
	if (!bJumping)
	{
		// Start jump
		if (ToTarget.Size() >= (Aggression * FMath::FRand() * 1500.0f))
		{
			MyTachyonCharacter->StartJump();
			bJumping = true;
		}
	}
	else
	{
		// End jump
		JumpTimer += DeltaTime;
		if (JumpTimer >= ((1.0f + FMath::FRand()) * Aggression) )
		{
			MyTachyonCharacter->EndJump();
			JumpTimer = 0.0f;
			bJumping = false;
		}
	}
}


void ATachyonAIController::AimAtTarget(AActor* TargetActor)
{

}


bool ATachyonAIController::ReactionTimeIsNow(float DeltaTime)
{
	bool Result = false;
	float TimeScalar = (1.0f / MyTachyonCharacter->CustomTimeDilation);
	ReactionTimer += (DeltaTime * TimeScalar);

	if (ReactionTimer >= ReactionTime)
	{
		Result = true;
		float RandomOffset = FMath::FRandRange(ReactionTime * -0.1f, ReactionTime * 0.9f);
		ReactionTimer = FMath::Clamp(RandomOffset, -0.1f, (ReactionTime * TimeScalar));
		//Aggression += FMath::FRandRange(-1.0f, 1.0f);
		//Aggression = FMath::Clamp(Aggression, -5.0f, 5.0f);

		///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("ReactionTimer  %f"), ReactionTimer));
		///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Aggression  %f"), Aggression));
	}

	return Result;
}


void ATachyonAIController::UpdateCharacterBody(float DeltaTime)
{
	
}


bool ATachyonAIController::HasViewToTarget(AActor* TargetActor, FVector TargetLocation)
{
	bool Result = false;

	return Result;
}
