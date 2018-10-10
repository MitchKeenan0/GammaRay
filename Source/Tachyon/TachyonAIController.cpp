// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAIController.h"



void ATachyonAIController::BeginPlay()
{
	Super::BeginPlay();

	
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
		float ValueX = 0.0f;
		float ValueZ = 0.0f;
		float VerticalDistance = ToTarget.Z;
		float LateralDistance = ToTarget.X;

		ValueX = FMath::Clamp(LateralDistance, -1.0f, 1.0f);
		ValueZ = FMath::Clamp(VerticalDistance, -1.0f, 1.0f);

		if (MyTachyonCharacter != nullptr)
		{
			MyTachyonCharacter->SetX(ValueX);
			MyTachyonCharacter->SetZ(ValueZ);

			// This doesn't work for some reason...
			float HastyDeltaTime = GetWorld()->DeltaTimeSeconds;
			MyTachyonCharacter->UpdateBody(HastyDeltaTime);
		}
	}
}


void ATachyonAIController::Combat(AActor* TargetActor)
{

}


void ATachyonAIController::AimAtTarget(AActor* TargetActor)
{

}


bool ATachyonAIController::ReactionTimeIsNow(float DeltaTime)
{
	bool Result = false;

	return Result;
}


bool ATachyonAIController::HasViewToTarget(AActor* TargetActor, FVector TargetLocation)
{
	bool Result = false;

	return Result;
}
