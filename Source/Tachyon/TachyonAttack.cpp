// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAttack.h"
#include "PaperSpriteComponent.h"


// Sets default values
ATachyonAttack::ATachyonAttack()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AttackScene = CreateDefaultSubobject<USceneComponent>(TEXT("AttackScene"));
	SetRootComponent(AttackScene);

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ATachyonAttack::OnAttackBeginOverlap);

	AttackParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AttackParticles"));
	AttackParticles->SetupAttachment(RootComponent);

	AttackSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("AttackSprite"));
	AttackSprite->SetupAttachment(RootComponent);

	bReplicates = true;
	bReplicateMovement = true;
}

// Called when the game starts or when spawned
void ATachyonAttack::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATachyonAttack::InitAttack(AActor* Shooter, float Magnitude, float YScale)
{
	OwningShooter = Shooter;
	if (OwningShooter != nullptr)
	{
		AttackMagnitude = Magnitude;
		AttackDirection = YScale;

		// Last-second redirection
		float ShooterYaw = FMath::Abs(OwningShooter->GetActorRotation().Yaw);
		float TargetYaw = 0.0f;
		if ((ShooterYaw > 50.0f))
		{
			TargetYaw = 180.0f;
		}
		float Pitch = FMath::Clamp(GetActorRotation().Pitch, -ShootingAngle, ShootingAngle);
		float Roll = GetActorRotation().Roll;
		FRotator NewRotation = FRotator(Pitch, TargetYaw, Roll);
		SetActorRotation(NewRotation);

		// Lifetime
		DynamicLifetime = DurationTime;

		bInitialized = true;
	}
}

// Called every frame
void ATachyonAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInitialized)
	{
		RaycastForHit(GetActorForwardVector());
		UpdateLifeTime(DeltaTime);
	}
}


// Update life-cycle
void ATachyonAttack::UpdateLifeTime(float DeltaT)
{
	LifeTimer += DeltaT;
	if (LifeTimer >= DynamicLifetime)
	{
		if (OwningShooter != nullptr)
		{
			/*ATachyonCharacter* ShooterCharacter = Cast<ATachyonCharacter>(OwningShooter);
			if (ShooterCharacter != nullptr)
			{
				ShooterCharacter->NullifyAttack(); broken why?
			}*/
		}
	}
}


// Raycast firing solution
void ATachyonAttack::RaycastForHit(FVector RaycastVector)
{
	if (HasAuthority())
	{
		// Linecast ingredients
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));

		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(OwningShooter);

		FVector Start = GetActorLocation() + (GetActorForwardVector() * -100.0f);
		Start.Y = 0.0f;
		FVector End = Start + (RaycastVector * RaycastHitRange);
		End.Y = 0.0f; /// strange y-axis drift



					  // Swords, etc, get tangible ray space
		if (bRaycastOnMesh)
		{
			if (AttackSprite->GetSprite() != nullptr)
			{
				float SpriteLength = (AttackSprite->Bounds.BoxExtent.X) * (1.0f + AttackMagnitude);
				float AttackBodyLength = SpriteLength * RaycastHitRange;
				Start = AttackSprite->GetComponentLocation() + (GetActorForwardVector() * (-SpriteLength / 2.1f));
				Start.Y = 0.0f;
				End = Start + (RaycastVector * AttackBodyLength);
				End.Y = 0.0f;
			}
			else if (AttackParticles != nullptr)
			{
				float AttackBodyLength = RaycastHitRange;
				Start = AttackParticles->GetComponentLocation() + (GetActorForwardVector() * (-AttackBodyLength / 2.1f));
				Start.Y = 0.0f;
				End = Start + (RaycastVector * AttackBodyLength);
				End.Y = 0.0f;
			}
		}

		TArray<FHitResult> Hits;

		// Pew pew
		bool HitResult = UKismetSystemLibrary::LineTraceMultiForObjects(
			this,
			Start,
			End,
			TraceObjects,
			false,
			IgnoredActors,
			EDrawDebugTrace::ForDuration,
			Hits,
			true,
			FLinearColor::Gray, FLinearColor::Red, 5.0f);

		if (HitResult)
		{
			int NumHits = Hits.Num();
			for (int i = 0; i < NumHits; ++i)
			{
				HitActor = Hits[i].Actor.Get();
				if ((HitActor != nullptr)
					&& (HitActor != OwningShooter)
					&& (HitActor->WasRecentlyRendered(0.2f)))
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Green, TEXT("B A N G    G O T T E M"));
					//HitEffects(HitActor, Hits[i].ImpactPoint);
				}
			}
		}
	}
}


// COLLISION BEGIN
void ATachyonAttack::OnAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		bool bTime = (HitTimer >= (1 / HitsPerSecond));
		bool bActors = (OwningShooter != nullptr) && (OtherActor != nullptr);
		float TimeSc = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		if (bTime && bActors && bLethal && (OtherActor != OwningShooter) && (TimeSc > 0.5f))
		{
			FVector DamageLocation = GetActorLocation() + (OwningShooter->GetActorForwardVector() * RaycastHitRange);
			if (ActorHasTag("Obstacle"))
			{
				DamageLocation = GetActorLocation() + SweepResult.ImpactPoint;
			}

			// Got'em
			//HitEffects(OtherActor, DamageLocation);
		}
	}
}


// NETWORK VARIABLES
void ATachyonAttack::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATachyonAttack, bInitialized);
	DOREPLIFETIME(ATachyonAttack, OwningShooter);
	DOREPLIFETIME(ATachyonAttack, HitActor);
	DOREPLIFETIME(ATachyonAttack, DynamicLifetime);
	DOREPLIFETIME(ATachyonAttack, LethalTime);
	DOREPLIFETIME(ATachyonAttack, bHit);
	DOREPLIFETIME(ATachyonAttack, NumHits);
	DOREPLIFETIME(ATachyonAttack, AttackMagnitude);
	DOREPLIFETIME(ATachyonAttack, AttackDirection);
	DOREPLIFETIME(ATachyonAttack, AttackDamage);
	
}
