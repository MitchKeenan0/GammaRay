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
				MyTachyonCharacter->Tags.Add("FramingActor");
			}
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
					if (CurrentTachyon != MyTachyonCharacter)
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

	if (MyTachyonCharacter != nullptr)
	{
		if (Player != nullptr)
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

			// Combat
			if (ReactionTimeIsNow(DeltaTime))
			{
				Combat(Player);
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
			PlayerTargetPostion = PlayerLocation + (PlayerVelocity * 0.9f);
		}
		else
		{
			PlayerTargetPostion = PlayerLocation;
		}

		FVector RandomOffset = FMath::VRand() * VelocityScalar;
		RandomOffset.Y = 0.0f;
		RandomOffset.Z *= 0.3f;

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
	if (ToTarget.Size() < 100.0f)
	{
		LocationTarget = FVector::ZeroVector;
		bCourseLayedIn = false;
		return;
	}
	else
	{
		float VerticalDistance = ToTarget.Z;
		float LateralDistance = ToTarget.X;

		MyInputX = FMath::Clamp(LateralDistance, -1.0f, 1.0f);
		MyInputZ = FMath::Clamp(VerticalDistance, -1.0f, 1.0f);

		/*if (MyTachyonCharacter != nullptr)
		{
			MyTachyonCharacter->SetX(MyInputX);
			MyTachyonCharacter->SetZ(MyInputZ);
		}*/

		float DeltaTime = GetWorld()->DeltaTimeSeconds;
		MyTachyonCharacter->UpdateBody(DeltaTime);

		//// Set rotation so character faces direction of travel
		//float DeltaTime = GetWorld()->DeltaTimeSeconds;
		//float TravelDirection = FMath::Clamp(MyInputX, -1.0f, 1.0f);
		//float ClimbDirection = FMath::Clamp(MyInputZ, -1.0f, 1.0f) * 5.0f;
		//float Roll = FMath::Clamp(MyInputZ, -1.0f, 1.0f) * 15.0f;
		//float RotatoeSpeed = 15.0f;

		//if (TravelDirection < 0.0f)
		//{
		//	FRotator Fint = FMath::RInterpTo(GetControlRotation(), FRotator(ClimbDirection, 180.0f, Roll), DeltaTime, RotatoeSpeed);
		//	SetControlRotation(Fint);
		//}
		//else if (TravelDirection > 0.0f)
		//{
		//	FRotator Fint = FMath::RInterpTo(GetControlRotation(), FRotator(ClimbDirection, 0.0f, -Roll), DeltaTime, RotatoeSpeed);
		//	SetControlRotation(Fint);
		//}

		//// No lateral Input - finish rotation
		//else
		//{
		//	if (FMath::Abs(GetControlRotation().Yaw) > 90.0f)
		//	{
		//		FRotator Fint = FMath::RInterpTo(GetControlRotation(), FRotator(ClimbDirection, 180.0f, -Roll), DeltaTime, RotatoeSpeed);
		//		SetControlRotation(Fint);
		//	}
		//	else if (FMath::Abs(GetControlRotation().Yaw) < 90.0f)
		//	{
		//		FRotator Fint = FMath::RInterpTo(GetControlRotation(), FRotator(ClimbDirection, 0.0f, Roll), DeltaTime, RotatoeSpeed);
		//		SetControlRotation(Fint);
		//	}
		//}
	}
}


void ATachyonAIController::Combat(AActor* TargetActor)
{
	// Aim - leads to attacks and secondaries
	FVector LocalForward = MyTachyonCharacter->GetAttackScene()->GetForwardVector();
	FVector ToTarget = TargetActor->GetActorLocation() - MyTachyonCharacter->GetActorLocation();
	FVector ForwardNorm = LocalForward.GetSafeNormal();
	FVector ToPlayerNorm = ToTarget.GetSafeNormal();
	float VerticalNorm = FMath::FloorToFloat(FMath::Clamp((ToTarget.GetSafeNormal()).Z, -1.0f, 1.0f));
	float DotToTarget = FVector::DotProduct(ForwardNorm, ToPlayerNorm);
	float RangeToTarget = ToTarget.Size();
	//float MyShootingAngle = MyTachyonCharacter

	if (RangeToTarget <= 2000.0f)
	{

		// Only fire if a) we're on screen & b) angle looks good
		float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DotToTarget));

		if (AngleToTarget <= 21.0f)  // && MyCharacter->WasRecentlyRendered(0.15f)
		{


			// Line up a shot with player Z input
			if (FMath::Abs(AngleToTarget) <= 0.25f)
			{
				//MyTachyonCharacter->SetZ(0.0f);
				MyInputZ = 0.0f;
			}
			else
			{
				//MyTachyonCharacter->SetZ(1.0f);
				MyInputZ = 1.0f;
			}

			// Aim input -- seems unnecessary
			/*float XTarget = FMath::Clamp(ToPlayer.X, -1.0f, 1.0f);
			MyCharacter->SetX(XTarget, 1.0f);*/

			// Attacking
			MyTachyonCharacter->StartFire();

			/*else
			{
				MyTachyonCharacter->ReleaseAttack();
			}*/
		}
	}
}


void ATachyonAIController::AimAtTarget(AActor* TargetActor)
{

}


bool ATachyonAIController::ReactionTimeIsNow(float DeltaTime)
{
	bool Result = false;
	float GlobalTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	float TimeScalar = (1.0f / GlobalTime);
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
