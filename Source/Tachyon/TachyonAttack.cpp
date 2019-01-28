// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAttack.h"
#include "TachyonCharacter.h"
#include "TimerManager.h"
#include "TachyonGameStateBase.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
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

	/*AttackParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AttackParticles"));
	AttackParticles->SetupAttachment(RootComponent);*/

	AttackSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("AttackSprite"));
	AttackSprite->SetupAttachment(RootComponent);

	AttackSound = CreateDefaultSubobject<UAudioComponent>(TEXT("AttackSound"));
	AttackSound->SetupAttachment(RootComponent);

	ProjectileComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileComponent->ProjectileGravityScale = 0.0f;
	ProjectileComponent->SetIsReplicated(true);

	AttackRadial = CreateDefaultSubobject<URadialForceComponent>(TEXT("AttackRadial"));
	AttackRadial->SetupAttachment(RootComponent);

	SetReplicates(true);
	bReplicateMovement = true;
	NetDormancy = ENetDormancy::DORM_Never;
	NetUpdateFrequency = 60.0f;
	MinNetUpdateFrequency = 30.0f;
}


// Called when the game starts or when spawned
void ATachyonAttack::BeginPlay()
{
	Super::BeginPlay();
	
	TimeBetweenShots = RefireTime;
	ActualAttackDamage = AttackDamage;
	ActualDeliveryTime = DeliveryTime;
	DamageTimer = 0.0f;

	if (CapsuleComponent != nullptr)
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AttackRadial != nullptr)
	{
		AttackRadial->SetWorldLocation(GetActorLocation());
		AttackRadial->Deactivate();
	}
}


// INPUT RECEPTION /////////////////////////////////////////////
void ATachyonAttack::StartFire()
{
	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (GlobalTimeScale == 1.0f)
	{
		float FirstDelay = (FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f));
		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ATachyonAttack::Fire, TimeBetweenShots, false, FirstDelay);
	}
	
}
void ATachyonAttack::EndFire()
{
	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (GlobalTimeScale == 1.0f)
	{
		float FirstDelay = DeliveryTime * CustomTimeDilation;

		GetWorldTimerManager().SetTimer(TimerHandle_DeliveryTime, this, &ATachyonAttack::Lethalize, DeliveryTime, false, FirstDelay);

		GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);

		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("Set Timer to Lethalize"));
	}
}


// ARMING /////////////////////////////////////////////////
void ATachyonAttack::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (!bInitialized)
		{
			//Neutralize();

			OwningShooter = MyOwner;

			// strangeley dense way to prevent particle ghosting
			/*if (AttackParticles != nullptr)
			{
				AttackParticles->DeactivateSystem();
				AttackParticles->Deactivate();
				AttackParticles->SetVisibility(false, true);
			}*/

			bInitialized = true;
			bNeutralized = false;

			TimeAtInit = GetWorld()->TimeSeconds;
			DamageTimer = 99.9f;

			// Shooter slow and position to fire point
			ATachyonCharacter* ShooterCharacter = Cast<ATachyonCharacter>(OwningShooter);
			if (ShooterCharacter != nullptr)
			{
				float ShooterTimeDilation = OwningShooter->CustomTimeDilation * FMath::Sqrt(ShooterSlow);
				ShooterCharacter->NewTimescale(ShooterTimeDilation);

				FVector EmitLocation = ShooterCharacter->GetAttackScene()->GetComponentLocation();
				SetActorLocation(EmitLocation);
			}

			//RedirectAttack();

			if (Role == ROLE_Authority)
			{
				SpawnBurst();
			}


			// Used to determine magnitude at Lethalize
			// Start deliverytime timer
			if (bSecondary && !(GetWorldTimerManager().IsTimerActive(TimerHandle_DeliveryTime)))
			{
				EndFire();
			}

			// Debug bank
			///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackMagnitude: %f"), AttackMagnitude));
			///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("HitsPerSecond:   %f"), HitsPerSecond));
			///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackDamage:    %f"), AttackDamage));
		}
	}
}
void ATachyonAttack::ServerFire_Implementation()
{
	Fire();
}
bool ATachyonAttack::ServerFire_Validate()
{
	return true;
}


// PRE-LETHAL EFFECTS
void ATachyonAttack::SpawnBurst()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if ((BurstClass != nullptr)) ///(Role == ROLE_Authority) &&  
		{
			FActorSpawnParameters SpawnParams;
			CurrentBurstObject = GetWorld()->SpawnActor<AActor>(BurstClass, GetActorLocation(), GetActorRotation(), SpawnParams);
			if (CurrentBurstObject != nullptr)
			{
				CurrentBurstObject->AttachToActor(MyOwner, FAttachmentTransformRules::KeepWorldTransform);

				// Scaling
				/*float VisibleMagnitude = FMath::Clamp(AttackMagnitude, 0.5f, 1.0f);
				FVector NewBurstScale = CurrentBurstObject->GetActorRelativeScale3D() * VisibleMagnitude;
				CurrentBurstObject->SetActorRelativeScale3D(NewBurstScale);*/
			}
		}
	}
}


// LETHAL ACTIVATION ///////////////////////////////////////////////
void ATachyonAttack::Lethalize()
{
	if (Role < ROLE_Authority)
	{
		ServerLethalize();
	}

	bLethal = true;
	bDoneLethal = false;
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if ((bInitialized) && (OwningShooter != nullptr))
		{
			if (AttackRadial != nullptr)
			{
				AttackRadial->Activate();
			}

			if (Role == ROLE_Authority)
			{
				// Clear burst object
				if (CurrentBurstObject != nullptr)
				{
					CurrentBurstObject->SetLifeSpan(0.1f);
				}

				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, TEXT("One Timer!"));

				// Start shooting and timeout process
				LastFireTime = GetWorld()->TimeSeconds;

				FTimerHandle EffectsTimer;
				GetWorldTimerManager().SetTimer(EffectsTimer, this, &ATachyonAttack::ActivateEffects, ActualDeliveryTime, false, ActualDeliveryTime * 0.9f);

				// Raycasting
				float RefireTiming = (1.0f / ActualHitsPerSecond) * CustomTimeDilation;
				float FirstDelay = 0.09f * CustomTimeDilation;
				GetWorldTimerManager().SetTimer(TimerHandle_Raycast, this, &ATachyonAttack::RaycastForHit, RefireTiming, true, FirstDelay);

				// Effects
				SetInitVelocities();
				ActivateEffects();

				FVector RecoilLocation = GetActorLocation() + (GetActorForwardVector() * 1000.0f);
				ApplyKnockForce(OwningShooter, RecoilLocation, RecoilForce); // MyOwner
			}

			// Lifetime
			GetWorldTimerManager().SetTimer(TimerHandle_Neutralize, this, &ATachyonAttack::Neutralize, ActualDurationTime, false, ActualDurationTime);

			RedirectAttack();

			// Attack numbers are born here, to be reset later at Neutralize

			// Calculate and set magnitude characteristics
			float GeneratedMagnitude = FMath::Clamp(FMath::Square((GetWorld()->TimeSeconds - TimeAtInit)), 0.1f, 1.0f);
			if (bSecondary)
				GeneratedMagnitude = GivenMagnitude;

			// This leavens the number to one significant digit
			AttackMagnitude = (FMath::FloorToFloat(GeneratedMagnitude * 10)) * 0.1f;
			AttackMagnitude = FMath::Clamp(AttackMagnitude, 0.1f, 1.0f);

			ActualAttackDamage = (AttackDamage * AttackMagnitude);
			///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("ActualAttackDamage: %f"), ActualAttackDamage));

			// Shooter Slow and location
			ATachyonCharacter* ShooterCharacter = Cast<ATachyonCharacter>(OwningShooter);
			if (ShooterCharacter != nullptr)
			{
				float ShooterTimeDilation = OwningShooter->CustomTimeDilation * (ShooterSlow * 0.5f);
				ShooterTimeDilation = FMath::Clamp(ShooterTimeDilation, 0.3f, 0.9f);

				ShooterCharacter->NewTimescale(ShooterTimeDilation);

				FVector EmitLocation = ShooterCharacter->GetAttackScene()->GetComponentLocation();
				SetActorLocation(EmitLocation);
			}

			// Timing numbers
			float NewHitRate = FMath::Clamp((HitsPerSecond * AttackMagnitude), 1.0f, HitsPerSecond);
			ActualHitsPerSecond = NewHitRate;

			ActualDeliveryTime = DeliveryTime + AttackMagnitude;
			ActualDurationTime = (DurationTime + AttackMagnitude) * CustomTimeDilation;
			ActualLethalTime = LethalTime + AttackMagnitude;

			if (bSecondary)
			{
				ActualDurationTime = DurationTime;
			}

			HitTimer = (1.0f / ActualHitsPerSecond) * CustomTimeDilation;
			RefireTime = RefireTime * AttackMagnitude;

			///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("ActualDeliveryTime: %f"), ActualDeliveryTime));
			///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("ActualDurationTime: %f"), ActualDurationTime));

			if (CapsuleComponent != nullptr)
			{
				CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}

			// TIMERS ///////////////////////////

			
			
			// Screen shake
			AActor* MyOwner = GetOwner();
			if (MyOwner != nullptr)
			{
				if (FireShake != nullptr)
					UGameplayStatics::PlayWorldCameraShake(GetWorld(), FireShake, GetActorLocation(), 0.0f, 9999.0f, 1.0f, false);
			}

			// this contravenes above
			//ReceiveTimescale(1.0f);
			
			// Visual slow
			if (AttackParticles != nullptr)
			{
				AttackParticles->CustomTimeDilation = FMath::Clamp(AttackMagnitude, 0.33f, 1.0f);
			}

			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("Lethalized!"));
		}
	}
}
void ATachyonAttack::ServerLethalize_Implementation()
{
	Lethalize();
}
bool ATachyonAttack::ServerLethalize_Validate()
{
	return true;
}


// EFFECTS & SOUND /////////////////////////////////////////////////////
void ATachyonAttack::ActivateEffects_Implementation()
{

	/*AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		AttackParticles = ActivateParticles();
	}*/

	if (Role == ROLE_Authority)
	{
		ActivateParticles();
	}

	if (AttackSound != nullptr)
		ActivateSound();
}


void ATachyonAttack::ActivateSound()
{
	if ((AttackSound != nullptr)) /// && !AttackSound->IsPlaying()
	{
		float ModifiedPitch = AttackSound->PitchMultiplier * FMath::Sqrt(AttackMagnitude);
		AttackSound->SetPitchMultiplier(ModifiedPitch);
		AttackSound->Activate();
		AttackSound->Play();
	}
}


void ATachyonAttack::ActivateParticles()
{
	AActor* Result = nullptr;
	TSubclassOf<AActor> TypeSpawning = nullptr;

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (AttackMagnitude != 0.0f)
		{
			///GEngine->AddOnScreenDebugMessage(-1, 3.3f, FColor::White, FString::Printf(TEXT("Perceived AttackMagnitude: %f"), AttackMagnitude));

			if (AttackMagnitude < 0.5f)
			{
				if (AttackEffectLight != nullptr)
				{
					TypeSpawning = AttackEffectLight;
					///GEngine->AddOnScreenDebugMessage(-1, 3.3f, FColor::White, FString::Printf(TEXT("TypeSpawning light")));
				}
			}
			else
			{
				if (AttackEffectHeavy != nullptr)
				{
					TypeSpawning = AttackEffectHeavy;
					///GEngine->AddOnScreenDebugMessage(-1, 3.3f, FColor::White, FString::Printf(TEXT("TypeSpawning HEAVY")));
				}
			}
		}

		FActorSpawnParameters SpawnParams;
		AttackParticles = GetWorld()->SpawnActor<AActor>(TypeSpawning, GetActorLocation(), GetActorRotation(), SpawnParams);
		if (AttackParticles != nullptr)
		{
			AttackParticles->AttachToActor(MyOwner, FAttachmentTransformRules::KeepWorldTransform);

			// Scaling
			/*float VisibleMagnitude = FMath::Clamp(AttackMagnitude, 0.5f, 1.0f);
			FVector NewBurstScale = CurrentBurstObject->GetActorRelativeScale3D() * VisibleMagnitude;
			CurrentBurstObject->SetActorRelativeScale3D(NewBurstScale);*/
		}
	}
}


void ATachyonAttack::SetInitVelocities()
{
	FVector ShooterVelocity = GetOwner()->GetVelocity();

	// Inherit some velocity from owning shooter
	if ((ProjectileComponent != nullptr) && (ProjectileSpeed != 0.0f))
	{
		FVector InitialVelocity = GetActorForwardVector() * ProjectileSpeed;
		InitialVelocity.Y = 0.0f;
		
		ProjectileComponent->Velocity = InitialVelocity;
		
		
		float VelSize = ShooterVelocity.Size();
		if (VelSize != 0.0f)
		{
			float ScalarProjectileSpeed = ProjectileSpeed;
			if (bSecondary)
				ScalarProjectileSpeed *= 0.001f;
			FVector ScalarVelocity = ShooterVelocity.GetSafeNormal() * FMath::Sqrt(VelSize) * ScalarProjectileSpeed;
			ScalarVelocity.Z *= 0.21f;
			ProjectileComponent->Velocity += ScalarVelocity;
		}
	}
}


// REDIRECTION ////////////////////////////////////////////////////////////
void ATachyonAttack::RedirectAttack()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		ATachyonCharacter* TachyonShooter = Cast<ATachyonCharacter>(OwningShooter);
		bool bTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld()) == 1.0f;
		if (bTime && !bGameEnder && (TachyonShooter != nullptr))
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("Redirecting...")));

			FVector LocalForward = TachyonShooter->GetActorForwardVector();
			LocalForward.Y = 0.0f;
			float ShooterAimDirection = FMath::Clamp(
				(TachyonShooter->GetActorForwardVector().Z) * 1.0f,
				-1.0f, 1.0f);

			float TargetPitch = ShootingAngle * ShooterAimDirection * 3.14f;
			TargetPitch = FMath::Clamp(TargetPitch, -ShootingAngle, ShootingAngle);

			FRotator NewRotation = LocalForward.Rotation() + FRotator(TargetPitch, 0.0f, 0.0f);
			NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, -ShootingAngle, ShootingAngle);

			// Clamp Angles
			float ShooterYaw = FMath::Abs(OwningShooter->GetActorRotation().Yaw);
			if ((ShooterYaw > 50.0f))
			{
				NewRotation.Yaw = 180.0f;
			}
			else
			{
				NewRotation.Yaw = 0.0f;
			}


			// To Rotation
			FRotator PreRotation = FRotator::ZeroRotator;

			if (!bSecondary)
			{
				float ShooterTimeDilation = TachyonShooter->CustomTimeDilation;
				float PitchInterpSpeed = 10.0f + (((5.0f * AttackMagnitude) + (5.0f * ShooterTimeDilation)) * RedirectionSpeed);
				PitchInterpSpeed /= (NumHits * 2.0f);
				PitchInterpSpeed = FMath::Clamp(PitchInterpSpeed, 1.0f, 500.0f);

				FRotator InterpRotation = FMath::RInterpTo(
					GetActorRotation(),
					NewRotation,
					GetWorld()->DeltaTimeSeconds,
					PitchInterpSpeed);


				// Shot angle return to zero
				/*if (FMath::Abs(ShooterAimDirection) <= 0.1f)
				{
				InterpRotation.Pitch *= 0.999f;
				GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::White, TEXT("attack redirect bottom out"));
				}*/

				// Quick-snap to angle shot
				if (ShooterAimDirection < -0.1f)
				{
					InterpRotation.Pitch = FMath::Clamp(InterpRotation.Pitch, -ShootingAngle, -1.0f);
				}
				else if (ShooterAimDirection > 0.1f)
				{
					InterpRotation.Pitch = FMath::Clamp(InterpRotation.Pitch, 1.0f, ShootingAngle);
				}

				PreRotation = InterpRotation;
			}
			else
			{
				PreRotation = NewRotation;
			}

			// Final recalculation to honour dimensional plane
			PreRotation.Pitch = FMath::Clamp(PreRotation.Pitch, -ShootingAngle, ShootingAngle);
			FRotator FinalRotation = PreRotation; // .Vector().ProjectOnTo(FVector::RightVector).Rotation();
			SetActorRotation(FinalRotation);

			// Update Location
			if (LockedEmitPoint) /// formerly bSecondary
			{
				FVector EmitLocation = TachyonShooter->GetAttackScene()->GetComponentLocation();
				if (bSecondary)
				{
					EmitLocation = TachyonShooter->GetActorLocation() + TachyonShooter->GetActorForwardVector();
				}
				FVector InterpPos = FMath::VInterpTo(GetActorLocation(), EmitLocation, GetWorld()->DeltaTimeSeconds, 100.0f);

				SetActorLocation(InterpPos);
			}


			if (AttackRadial != nullptr)
			{
				AttackRadial->SetWorldLocation(GetActorLocation());
			}

			ForceNetUpdate();
		}
	}
}


// TICK /////////////////////////////////////////////////////////////////////
void ATachyonAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (bLethal && !bNeutralized)
		{
			UpdateLifeTime(DeltaTime);
		}
	}
}


// Update life-cycle
void ATachyonAttack::UpdateLifeTime(float DeltaT)
{
	float GlobalTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());

	// Catch lost game ender
	if (GlobalTime >= 0.9f)
	{
		if (bGameEnder)
		{
			//ActivateEffects();
			CallForTimescale(OwningShooter, true, 0.01f);
			GetWorldTimerManager().ClearTimer(TimerHandle_Raycast);
		}
		else if (!bSecondary)
		{
			RedirectAttack();
		}

		if (HitResultsArray.Num() > 0)
		{
			TimedHit(DeltaT);
		}
	}
}


// Shooter Input Enabling
void ATachyonAttack::SetShooterInputEnabled(bool bEnabled)
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		// Re-enable Shooter's Input
		ACharacter* CharacterShooter = Cast<ACharacter>(GetOwner());
		if (CharacterShooter != nullptr)
		{
			APlayerController* TachyonController = Cast<APlayerController>(CharacterShooter->GetController());
			if (TachyonController != nullptr)
			{
				if (bEnabled)
				{
					CharacterShooter->EnableInput(TachyonController);
				}
				else
				{
					CharacterShooter->DisableInput(TachyonController);
				}
			}
		}
	}
}


// HIT DELIVERY //////////////////////////////////////////////////////////
void ATachyonAttack::TimedHit(float DeltaTime)
{
	DamageTimer += (DeltaTime * (1.0f/CustomTimeDilation));
	float HitRate = (1.0f / ActualHitsPerSecond) * 0.1f;

	if (DamageTimer >= HitRate)
	{
		AActor* CurrentActor = HitResultsArray[0].GetActor();
		if (CurrentActor != nullptr)
		{
			FVector HitLocation = HitResultsArray[0].ImpactPoint;
			if (HitLocation == FVector::ZeroVector)
			{
				//float DistToHit = FVector::Dist(GetActorLocation(), CurrentActor->GetActorLocation());
				HitLocation = GetActorLocation() + GetActorForwardVector();
			}
			
			FVector ForNorm = GetActorForwardVector().GetSafeNormal();
			FVector HitNorm = (HitLocation - GetActorLocation()).GetSafeNormal();
			float AngleToHit = FMath::Acos(FVector::DotProduct(ForNorm, HitNorm));
			if (AngleToHit <= 1.0f)
			{
				MainHit(CurrentActor, HitLocation);

				if (HitResultsArray.Num() > 0)
				{
					HitResultsArray.RemoveAt(0);
				}

				DamageTimer = 0.0f;
			}
			else if (HitResultsArray.Num() > 0)
			{
				HitResultsArray.RemoveAt(0);
			}
		}
	}
}


// RAYCAST ////////////////////////////////////////////////////////////////
void ATachyonAttack::RaycastForHit()
{
	if (Role < ROLE_Authority)
	{
		ServerRaycastForHit();
	}

	bool HitResult = false;

	// Linecast ingredients
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
	TArray<FHitResult> Hits;
	TArray<AActor*> IgnoredActors;

	// Set up ray position
	FVector RaycastVector = GetActorForwardVector();
	FVector Start = GetActorLocation() + (GetActorForwardVector() * -100.0f);
	Start.Y = 0.0f;
	FVector End = Start + (RaycastVector * RaycastHitRange);
	End.Y = 0.0f;
	
	AActor* MyOwner = GetOwner();
	if (!bGameEnder && (MyOwner != nullptr))
	{
		IgnoredActors.Add(MyOwner);

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
				Start = AttackParticles->GetActorLocation() + (GetActorForwardVector() * (-AttackBodyLength / 2.1f));
				Start.Y = 0.0f;
				End = Start + (RaycastVector * AttackBodyLength);
				End.Y = 0.0f;
			}
		}

		Start.Y = End.Y = 0.0f;

		// Pew pew
		HitResult = UKismetSystemLibrary::LineTraceMultiForObjects(
			this,
			Start,
			End,
			TraceObjects,
			false,
			IgnoredActors,
			EDrawDebugTrace::None,
			Hits,
			true,
			FLinearColor::Green, FLinearColor::Red, 5.0f);
	}

	if (HitResult)
	{
		int NumHits = Hits.Num();
		if (NumHits > 0)
		{
			for (int i = 0; i < NumHits; ++i)
			{
				HitActor = Hits[i].GetActor();
				if (HitActor != nullptr)
				{
					const FHitResult ThisHit = Hits[i];
					HitResultsArray.Add(ThisHit);
				}
			}
		}
	}
}
void ATachyonAttack::ServerRaycastForHit_Implementation()
{
	RaycastForHit();
}
bool ATachyonAttack::ServerRaycastForHit_Validate()
{
	return true;
}


void ATachyonAttack::SpawnHit(AActor* HitActor, FVector HitLocation)
{
	if (Role == ROLE_Authority)
	{
		if (DamageClass != nullptr)
		{
			FActorSpawnParameters SpawnParams;
			FVector ActualHitLocation = HitLocation;
			
			FVector ToHitLocation = (ActualHitLocation - GetActorLocation()).GetSafeNormal();
			AActor* HitSpawning = GetWorld()->SpawnActor<AActor>(DamageClass, ActualHitLocation, ToHitLocation.Rotation(), SpawnParams);
			if (HitSpawning != nullptr)
			{
				//HitSpawning->AttachToActor(HitActor, FAttachmentTransformRules::KeepWorldTransform);
				
				//float DamageTimescale = FMath::Clamp(HitActor->CustomTimeDilation, 0.05f, 1.0f);
				HitSpawning->CustomTimeDilation = FMath::Clamp(HitActor->CustomTimeDilation, 0.2f, 0.5f);
				
				float HitLifespan = 2.0f * HitSpawning->CustomTimeDilation;
				HitSpawning->SetLifeSpan(HitLifespan);
			}
		}
	}
}


void ATachyonAttack::ApplyKnockForce(AActor* HitActor, FVector HitLocation, float HitScalar)
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		// Init intended force
		FVector KnockDirection = GetActorForwardVector() + (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		///FVector KnockDirection = (HitActor->GetActorLocation() - HitLocation).GetSafeNormal();
		FVector KnockVector = KnockDirection * KineticForce * HitScalar;
		KnockVector = KnockVector.GetClampedToMaxSize(KineticForce / 2.0f);

		// Adjust force size for time dilation
		float GlobalDilationScalar = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		if (GlobalDilationScalar <= 0.05f)
			GlobalDilationScalar = 0.0f;
		else
			GlobalDilationScalar = (1.0f / GlobalDilationScalar);
		KnockVector *= GlobalDilationScalar;

		float TimescaleAverage = (HitActor->CustomTimeDilation + MyOwner->CustomTimeDilation) / 2.0f;
		KnockVector *= TimescaleAverage;

		// Character case
		ATachyonCharacter* Chara = Cast<ATachyonCharacter>(HitActor);
		if (Chara != nullptr)
		{
			KnockVector.Y = 0.0f;
			
			FVector CharaVelocity = Chara->GetMovementComponent()->Velocity;
			FVector KnockbackVector = CharaVelocity + KnockVector;

			Chara->ReceiveKnockback(KnockVector, false);
		}
	}
}


// HIT ////////////////////////////////////////////////////////////////////////
void ATachyonAttack::MainHit(AActor* HitActor, FVector HitLocation)
{
	// Bail out if we hit our own attack type
	ATachyonAttack* PotentialAttack = Cast<ATachyonAttack>(HitActor);
	if (PotentialAttack != nullptr)
	{
		if (PotentialAttack->OwningShooter == this->OwningShooter)
		{
			return;
		}
		else if (!bGameEnder)
		{
			// Check for shield here...
			if (PotentialAttack->ActorHasTag("Shield"))
			{
				CallForTimescale(PotentialAttack, false, 0.1f);
			}
		}
	}

	if (!bGameEnder && HasAuthority())
	{
		SpawnHit(HitActor, HitLocation);

		ApplyKnockForce(HitActor, HitLocation, KineticForce);

		// Update GameState
		ReportHitToMatch(GetOwner(), HitActor);

		ProjectileComponent->Velocity *= (1.0f - ProjectileDrag);

		ActualHitsPerSecond *= (HitsPerSecondDecay * CustomTimeDilation);
	}
}

// unused
void ATachyonAttack::ServerMainHit_Implementation(AActor* HitActor, FVector HitLocation)
{
	MainHit(HitActor, HitLocation);
}
bool ATachyonAttack::ServerMainHit_Validate(AActor* HitActor, FVector HitLocation)
{
	return true;
}


void ATachyonAttack::RemoteHit(AActor* Target, float Damage)
{
	if ((OwningShooter != nullptr) && (Role == ROLE_Authority))
	{
		ATachyonCharacter* HitTachyon = Cast<ATachyonCharacter>(Target);
		if ((HitTachyon != nullptr) && (HitTachyon != OwningShooter))
		{
			HitTachyon->ModifyHealth(-Damage);
			CallForTimescale(HitTachyon, false, 0.5f);
			ApplyKnockForce(OwningShooter, HitTachyon->GetActorLocation(), RecoilForce * 1000000.0f);
		}
	}
}

void ATachyonAttack::ReportHitToMatch(AActor* Shooter, AActor* Mark)
{
	ATachyonCharacter* HitTachyon = Cast<ATachyonCharacter>(Mark);
	if (HitTachyon != nullptr)
	{
		// Update Shooter's Opponent reference
		AActor* MyOwner = GetOwner();
		if (MyOwner != nullptr)
		{
			ATachyonCharacter* OwningTachyon = Cast<ATachyonCharacter>(MyOwner);
			if (OwningTachyon != nullptr)
			{
				OwningTachyon->SetOpponent(HitTachyon);
				HitTachyon->SetOpponent(OwningTachyon);
			}
		}

		HitTachyon->ModifyHealth(-ActualAttackDamage);
		/*if (HasAuthority())
		{
			HitTachyon->ModifyHealth(-ActualAttackDamage);
		}*/
		
		// Call it in
		float TachyonHealth = HitTachyon->GetHealth();
		if ((TachyonHealth - ActualAttackDamage) <= 0.0f)
		{
			//CallForTimescale(HitTachyon, false, 0.05f);
			//CallForTimescale(MyOwner, false, 0.05f);

			CallForTimescale(HitTachyon, true, 0.01f);

			CustomTimeDilation *= 0.1f;

			bGameEnder = true;
			
			/*if (AttackParticles != nullptr)
			{
				AttackParticles->CustomTimeDilation = 0.1f;
			}*/

			GetWorldTimerManager().ClearTimer(TimerHandle_Neutralize);
			float NewNeutTime = GetWorld()->TimeSeconds + 1.0f;
			GetWorldTimerManager().SetTimer(TimerHandle_Neutralize, this, &ATachyonAttack::Neutralize, ActualDurationTime, false, ActualDurationTime);
			//ActivateEffects();
		}
		else
		{
			// Basic hits
			// Slow the target
			float MarkTimescale = Mark->CustomTimeDilation;
			float NewTimescale = MarkTimescale - (AttackMagnitude * TimescaleImpact);
			float HitTimescale = FMath::Clamp(NewTimescale, 0.05f, 0.5f);

			if (NewTimescale <= MarkTimescale)
			{
				CallForTimescale(Mark, false, HitTimescale);
			}

			// A little slow for the shooter
			if (OwningShooter != nullptr)
			{
				float ShooterTimescale = HitTimescale * 1.618f; ///1.0f - (AttackMagnitude * TimescaleImpact * 0.01f);
				ShooterTimescale = FMath::Clamp(ShooterTimescale, 0.05f, 0.5f);
				if (ShooterTimescale < CustomTimeDilation)
				{
					CallForTimescale(OwningShooter, false, ShooterTimescale);
				}
			}
		}

		// Modify Health of Mark
		if (!bGameEnder) /// && Role == ROLE_Authority
		{
			// Scale intensity for next hit
			NumHits += 1;
			//ActualAttackDamage += NumHits;
			ActualLethalTime += TimeExtendOnHit;
			ActualDurationTime += TimeExtendOnHit;
		}

		if (!bFirstHitReported)
			bFirstHitReported = true;
	}
}


void ATachyonAttack::CallForTimescale(AActor* TargetActor, bool bGlobal, float NewTimescale)
{
	ATachyonGameStateBase* TGState = Cast<ATachyonGameStateBase>(GetWorld()->GetGameState());
	if (TGState != nullptr)
	{
		if (bGlobal)
		{
			TGState->SetGlobalTimescale(NewTimescale);
			Neutralize();
		}
		else
		{
			TGState->SetActorTimescale(TargetActor, NewTimescale);
			float ShooterTimescale = FMath::Clamp(NewTimescale + 0.5f, 0.5f, 1.0f);
			TGState->SetActorTimescale(OwningShooter, ShooterTimescale);
		}

		TGState->ForceNetUpdate();
	}
}

void ATachyonAttack::ReceiveTimescale(float InTimescale)
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (Role == ROLE_Authority)
		{
			CustomTimeDilation = InTimescale;
		}
	}
}


void ATachyonAttack::Neutralize()
{
	if (Role < ROLE_Authority)
	{
		ServerNeutralize();
	}

	if (AttackParticles != nullptr)
	{
		//AttackParticles->CustomTimeDilation = 1.0f;
		AttackParticles->Destroy();
		AttackParticles = nullptr;
	}

	// Reset variables
	bNeutralized = true;
	bInitialized = false;
	bDoneLethal = false;
	bLethal = false;
	bGameEnder = false;
	bFirstHitReported = false;
	LifeTimer = 0.0f;
	NumHits = 0;
	HitTimer = (1.0f / HitsPerSecond);
	DamageTimer = 0.0f;
	TimeBetweenShots = RefireTime;
	ActualHitsPerSecond = HitsPerSecond;
	ActualDeliveryTime = DeliveryTime;
	ActualAttackDamage = AttackDamage;
	ActualLethalTime = LethalTime;
	ActualDurationTime = DurationTime;

	GetWorldTimerManager().ClearTimer(TimerHandle_Raycast);
	GetWorldTimerManager().ClearTimer(TimerHandle_DeliveryTime);
	GetWorldTimerManager().ClearTimer(TimerHandle_HitDelivery);

	HitResultsArray.Empty();
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		

		// Reset components
		if (CapsuleComponent != nullptr)
		{
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (AttackSound != nullptr)
		{
			AttackSound->Deactivate();
		}

		if (AttackRadial != nullptr)
		{
			AttackRadial->SetWorldLocation(GetActorLocation());
			AttackRadial->Deactivate();
		}

		CustomTimeDilation = 1.0f;
	}
}
void ATachyonAttack::ServerNeutralize_Implementation()
{
	Neutralize();
}
bool ATachyonAttack::ServerNeutralize_Validate()
{
	return true;
}


// COLLISION BEGIN
void ATachyonAttack::OnAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (RaycastHitRange == 0.0f)
	{
		///bool bTime = !bFirstHitReported || (HitTimer >= (1 / ActualHitsPerSecond));
		bool bActors = (OwningShooter != nullptr) && (OtherActor != nullptr) && (OtherActor != OwningShooter);
		bool TimeSc = UGameplayStatics::GetGlobalTimeDilation(GetWorld()) >= 0.05f;
		if (bActors && (bLethal && !bDoneLethal) && TimeSc)
		{
			HitResultsArray.Add(SweepResult);
		}
	}
}
