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

	if (bSecondary)
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (AttackRadial != nullptr)
	{
		AttackRadial->SetWorldLocation(GetActorLocation());
		AttackRadial->Deactivate();
	}
}


// From Inputs
void ATachyonAttack::StartFire()
{
	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (GlobalTimeScale == 1.0f)
	{
		float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ATachyonAttack::Fire, TimeBetweenShots, false, FirstDelay); /// !bSecondary
	}
	
}

void ATachyonAttack::EndFire()
{
	if (!bLethal)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_DeliveryTime, this, &ATachyonAttack::Lethalize, DeliveryTime, false, DeliveryTime);
		GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
	}
}


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
			OwningShooter = MyOwner;

			// strangeley dense way to prevent particle ghosting
			if (AttackParticles != nullptr)
			{
				AttackParticles->DeactivateSystem();
				AttackParticles->Deactivate();
				AttackParticles->SetVisibility(false, true);
			}

			// Movement and VFX
			RedirectAttack();
			SetInitVelocities();
			
			if (Role == ROLE_Authority)
			{
				SpawnBurst();
			}

			bInitialized = true;
			bNeutralized = false;

			TimeAtInit = GetWorld()->TimeSeconds;

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


// unused
void ATachyonAttack::InitAttack()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (!bInitialized)
		{
			OwningShooter = MyOwner;

			if (AttackParticles != nullptr)
			{
				AttackParticles->Deactivate();
				AttackParticles->DeactivateSystem();
			}

			// Movement and VFX
			RedirectAttack();
			SetInitVelocities();
			SpawnBurst();

			bInitialized = true;
			bNeutralized = false;

			// Used to determine magnitude at Lethalize
			TimeAtInit = GetWorld()->TimeSeconds;


			// Debug bank
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("Inited attack"));
			/// GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackMagnitude: %f"), AttackMagnitude));
			/// GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("HitsPerSecond:   %f"), HitsPerSecond));
			/// GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackDamage:    %f"), AttackDamage));
		}
	}

	if (bSecondary)
	{
		EndFire();
	}
}


void ATachyonAttack::Lethalize()
{
	if (Role < ROLE_Authority)
	{
		ServerLethalize();
	}
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (bInitialized)
		{
			///RedirectAttack();

			if (AttackRadial != nullptr)
			{
				AttackRadial->Activate();
			}

			// Calculate and set magnitude characteristics
			float GeneratedMagnitude = FMath::Clamp(FMath::Square((GetWorld()->TimeSeconds - TimeAtInit)), 0.1f, 1.0f);
			if (bSecondary)
				GeneratedMagnitude = GivenMagnitude;
			AttackMagnitude = (FMath::FloorToFloat(GeneratedMagnitude * 10)) * 0.1f;

			float NewHitRate = FMath::Clamp((HitsPerSecond * AttackMagnitude), 1.0f, HitsPerSecond);
			ActualHitsPerSecond = NewHitRate;
			ActualAttackDamage *= (1.0f + AttackMagnitude);

			ActualLethalTime = LethalTime + AttackMagnitude;
			ActualDeliveryTime *= AttackMagnitude;
			ActualDurationTime = ActualDeliveryTime + DurationTime;
			DynamicLifetime = (ActualDeliveryTime + ActualDurationTime);
			HitTimer = (1.0f / ActualHitsPerSecond);
			RefireTime = 0.1f + AttackMagnitude;

			if (bSecondary)
				CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// Shooter Recoil
			if ((RecoilForce != 0.0f) && (Role == ROLE_Authority))
			{
				ATachyonCharacter* CharacterShooter = Cast<ATachyonCharacter>(MyOwner);
				if ((CharacterShooter != nullptr) && (CharacterShooter->GetController() != nullptr))
				{
					FVector RecoilVector = GetActorRotation().Vector().GetSafeNormal();
					float ClampedMagnitude = FMath::Clamp(AttackMagnitude, 0.5f, 1.0f);
					RecoilVector *= (RecoilForce * -ClampedMagnitude);
					CharacterShooter->ReceiveKnockback(RecoilVector, bAbsoluteHitForce);
				}
			}

			// Start shooting and timeout process
			LastFireTime = GetWorld()->TimeSeconds;

			if (HasAuthority())
			{
				float RefireTiming = (1.0f / ActualHitsPerSecond); // *CustomTimeDilation;
				GetWorldTimerManager().SetTimer(TimerHandle_Raycast, this, &ATachyonAttack::RaycastForHit, RefireTiming, true, ActualDeliveryTime);
			}

			GetWorldTimerManager().SetTimer(TimerHandle_Neutralize, this, &ATachyonAttack::Neutralize, ActualDurationTime, false, ActualDurationTime);
			//GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);

			RedirectAttack();
			ActivateEffects();
			ApplyKnockForce(MyOwner, GetActorLocation(), RecoilForce);

			// Clear burst object
			if (CurrentBurstObject != nullptr)
			{
				CurrentBurstObject->SetLifeSpan(0.1f);
			}

			// Screen shake
			AActor* MyOwner = GetOwner();
			if (MyOwner != nullptr)
			{
				if (FireShake != nullptr)
					UGameplayStatics::PlayWorldCameraShake(GetWorld(), FireShake, GetActorLocation(), 0.0f, 9999.0f, 1.0f, false);
			}

			bLethal = true;
			bDoneLethal = false;
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


void ATachyonAttack::ActivateEffects_Implementation()
{
	ActivateParticles();

	if (AttackSound != nullptr)
		ActivateSound();
}


void ATachyonAttack::ActivateSound()
{
	if ((AttackSound != nullptr)) /// && !AttackSound->IsPlaying()
	{
		float ModifiedPitch = AttackSound->PitchMultiplier * (AttackMagnitude);
		AttackSound->SetPitchMultiplier(ModifiedPitch);
		AttackSound->Activate();
		AttackSound->Play();
	}
}


void ATachyonAttack::ActivateParticles()
{
	if (AttackEffect != nullptr)
	{
		AttackParticles = UGameplayStatics::SpawnEmitterAttached(AttackEffect, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
		if (AttackParticles != nullptr)
		{
			AttackParticles->bAutoDestroy = true;
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
				ScalarProjectileSpeed *= 0.0f;
			FVector ScalarVelocity = ShooterVelocity.GetSafeNormal() * FMath::Sqrt(VelSize) * ScalarProjectileSpeed;
			ScalarVelocity.Z *= 0.21f;
			ProjectileComponent->Velocity += ScalarVelocity;
		}
	}
}


void ATachyonAttack::RedirectAttack()
{
	ATachyonCharacter* TachyonShooter = Cast<ATachyonCharacter>(OwningShooter);
	if (TachyonShooter != nullptr)
	{
		FVector LocalForward = TachyonShooter->GetActorForwardVector();
		LocalForward.Y = 0.0f;
		float ShooterAimDirection = FMath::Clamp(TachyonShooter->GetActorForwardVector().Z * ShootingAngle, -1.0f, 1.0f);
		/*if (ShooterAimDirection == 0.0f)
		{
		ShooterAimDirection = FMath::Clamp(TachyonShooter->GetActorForwardVector().Z * ShootingAngle, -1.0f, 1.0f);
		}*/
		float TargetPitch = 2.0f * ShootingAngle * ShooterAimDirection;
		if (FMath::Abs(ShooterAimDirection) <= 0.5f)
		{
			TargetPitch = 0.0f;
		}
		FRotator NewRotation = LocalForward.Rotation() + FRotator(TargetPitch, 0.0f, 0.0f);

		// Clamp Angles
		float ShooterYaw = FMath::Abs(OwningShooter->GetActorRotation().Yaw);
		float Yaw = 0.0f;
		if ((ShooterYaw > 50.0f))
		{
			Yaw = 180.0f;
		}
		NewRotation.Yaw = Yaw;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, -ShootingAngle, ShootingAngle);
		FRotator InterpRotation = FMath::RInterpConstantTo(
			GetActorRotation(),
			NewRotation,
			GetWorld()->DeltaTimeSeconds,
			55.0f * (1.0f / CustomTimeDilation));
		SetActorRotation(InterpRotation);

		FVector EmitLocation = TachyonShooter->GetAttackScene()->GetComponentLocation();
		//FRotator EmitRotation;
		//OwningShooter->GetActorEyesViewPoint(EmitLocation, EmitRotation);
		SetActorLocation(EmitLocation);
			

		if (AttackRadial != nullptr)
		{
			AttackRadial->SetWorldLocation(GetActorLocation());
		}

		ForceNetUpdate();
	}
}


// Called every frame
void ATachyonAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (bInitialized && !bNeutralized)
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
	if (bGameEnder && (GlobalTime != 0.01f))
	{
		CallForTimescale(this, true, 0.01f);
	}

	if (GlobalTime <= 0.1f)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Raycast);
	}

	// Fully charged 'timeout' to release attack
	/*if (!bSecondary && !bDoneLethal 
		&& ((GetWorld()->TimeSeconds - TimeAtInit) >= 2.0f))
	{
		Lethalize();
	}*/

	// Attack main line
	//if (bLethal || bDoneLethal)
	//{
	//	float CustomDeltaTime = (1.0f / CustomTimeDilation) * DeltaT;
	//	LifeTimer += CustomDeltaTime;

	//	// Responsive aim for flex shots
	//	if (LifeTimer <= RedirectionTime)
	//	{
	//		RedirectAttack();
	//	}
	//}
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


// Raycast firing solution
void ATachyonAttack::RaycastForHit()
{
	if (Role < ROLE_Authority)
	{
		ServerRaycastForHit();
	}
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		RedirectAttack();

		// Linecast ingredients
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
		TArray<FHitResult> Hits;
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(MyOwner);

		// Set up ray position
		FVector RaycastVector = GetActorForwardVector();
		FVector Start = GetActorLocation() + (GetActorForwardVector() * -100.0f);
		Start.Y = 0.0f;
		FVector End = Start + (RaycastVector * RaycastHitRange);
		End.Y = 0.0f;

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

		// Pew pew
		bool HitResult = UKismetSystemLibrary::LineTraceMultiForObjects(
			this,
			Start,
			End,
			TraceObjects,
			false,
			IgnoredActors,
			EDrawDebugTrace::None,
			Hits,
			true,
			FLinearColor::Black, FLinearColor::Red, 5.0f);

		if (HitResult)
		{
			int NumHits = Hits.Num();
			for (int i = 0; i < NumHits; ++i)
			{
				HitActor = Hits[i].GetActor();

				if (HitActor != nullptr)
				{
					MainHit(HitActor, Hits[i].ImpactPoint);
				}
			}
		}

		//ActivateEffects();
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
			FVector ToHitLocation = (HitLocation - GetActorLocation()).GetSafeNormal();
			AActor* HitSpawning = GetWorld()->SpawnActor<AActor>(DamageClass, HitLocation, ToHitLocation.Rotation(), SpawnParams);
			if (HitSpawning != nullptr)
			{
				HitSpawning->AttachToActor(HitActor, FAttachmentTransformRules::KeepWorldTransform);
				float DamageTimescale = FMath::Clamp(CustomTimeDilation, 0.0f, 1.0f);
				HitSpawning->CustomTimeDilation = DamageTimescale;
			}
		}
	}
}


void ATachyonAttack::ApplyKnockForce(AActor* HitActor, FVector HitLocation, float HitScalar)
{
	// Init intended force
	FVector KnockDirection = GetActorForwardVector() + (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	///FVector KnockDirection = (HitActor->GetActorLocation() - HitLocation).GetSafeNormal();
	FVector KnockVector = KnockDirection * KineticForce * HitScalar;
	KnockVector.Y = 0.0f;
	KnockVector = KnockVector.GetClampedToMaxSize(KineticForce / 2.0f);

	// Adjust force size for time dilation
	float GlobalDilationScalar = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (GlobalDilationScalar <= 0.05f)
		GlobalDilationScalar = 0.0f;
	else
		GlobalDilationScalar = (1.0f / GlobalDilationScalar);
	KnockVector *= GlobalDilationScalar;

	// Character case
	ATachyonCharacter* Chara = Cast<ATachyonCharacter>(HitActor);
	if (Chara != nullptr)
	{
		FVector CharaVelocity = Chara->GetMovementComponent()->Velocity;
		FVector KnockbackVector = CharaVelocity + KnockVector;

		Chara->ReceiveKnockback(KnockVector, true);
	}
}


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
		else
		{
			// Check for shield here...
			if (PotentialAttack->ActorHasTag("Shield"))
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.5f, FColor::White, TEXT("OHH get shielded on"));
				CustomTimeDilation *= 0.1f;
			}
		}
	}

	// Smashy fx
	if (Role == ROLE_Authority)
	{
		SpawnHit(HitActor, HitLocation);

		if (!bGameEnder)
		{
			ApplyKnockForce(HitActor, HitLocation, 1.0f);
		}
		
		// Update GameState
		ReportHitToMatch(GetOwner(), HitActor);

		ActualHitsPerSecond *= HitsPerSecondDecay;

		ProjectileComponent->Velocity *= (1.0f - ProjectileDrag);

		if (!bFirstHitReported)
			bFirstHitReported = true;
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
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("WHAMMMM"));
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
		
		// Call it in
		float TachyonHealth = HitTachyon->GetHealth();
		if ((TachyonHealth - ActualAttackDamage) <= 0.0f)
		{
			CallForTimescale(Mark, false, 0.01f); /// 0.01 is Terminal timescale
			CustomTimeDilation = 0.001f;
			bGameEnder = true;

			ActivateEffects();
		}
		else
		{
			// Basic hits
			if (!bFirstHitReported || bSecondary)
			{
				float ImpactScalar = (AttackMagnitude * TimescaleImpact);
				float HitTimescale = FMath::Clamp((1.0f - ImpactScalar), 0.07f, 0.7f);
				CallForTimescale(Mark, false, HitTimescale);
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


void ATachyonAttack::Neutralize()
{
	if (Role < ROLE_Authority)
	{
		ServerNeutralize();
	}
	
	AActor* MyOwner = GetOwner();
	if (MyOwner != nullptr)
	{
		if (AttackParticles != nullptr)
		{
			AttackParticles->Deactivate();
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
		TimeBetweenShots = RefireTime;
		ActualHitsPerSecond = HitsPerSecond;
		ActualDeliveryTime = DeliveryTime;
		ActualAttackDamage = AttackDamage;
		ActualLethalTime = LethalTime;
		ActualDurationTime = DurationTime;

		// Reset components
		if (bSecondary)
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (AttackSound != nullptr)
		{
			AttackSound->Deactivate();
		}

		if (AttackRadial != nullptr)
		{
			AttackRadial->SetWorldLocation(GetActorLocation());
			AttackRadial->Deactivate();
		}

		GetWorldTimerManager().ClearTimer(TimerHandle_Raycast);
		GetWorldTimerManager().ClearTimer(TimerHandle_DeliveryTime);

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
	bool bTime = !bFirstHitReported || (HitTimer >= (1 / ActualHitsPerSecond));
	bool bActors = (OwningShooter != nullptr) && (OtherActor != nullptr) && (OtherActor != OwningShooter);
	bool TimeSc = UGameplayStatics::GetGlobalTimeDilation(GetWorld()) >= 0.05f;
	if (bTime && bActors && (bLethal && !bDoneLethal) && TimeSc)
	{
		FVector DamageLocation = GetActorLocation() + (OwningShooter->GetActorForwardVector() * RaycastHitRange);
		if (ActorHasTag("Obstacle"))
		{
			DamageLocation = GetActorLocation() + SweepResult.ImpactPoint;
		}

		MainHit(OtherActor, DamageLocation);
		HitTimer = 0.0f;
	}
}
