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

	AttackParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AttackParticles"));
	AttackParticles->SetupAttachment(RootComponent);

	AttackSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("AttackSprite"));
	AttackSprite->SetupAttachment(RootComponent);

	AttackSound = CreateDefaultSubobject<UAudioComponent>(TEXT("AttackSound"));
	AttackSound->SetupAttachment(RootComponent);

	ProjectileComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileComponent->ProjectileGravityScale = 0.0f;
	ProjectileComponent->SetIsReplicated(true);

	AttackRadial = CreateDefaultSubobject<URadialForceComponent>(TEXT("AttackRadial"));

	SetReplicates(true);
	bReplicateMovement = true;
	NetDormancy = ENetDormancy::DORM_Never;
}


// Called when the game starts or when spawned
void ATachyonAttack::BeginPlay()
{
	Super::BeginPlay();
	
	// Tactical setup
	bLethal = false;
	HitTimer = (1.0f / HitsPerSecond);
	AttackDamage = 1.0f;
	TimeBetweenShots = RefireTime;

	
}


void ATachyonAttack::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ATachyonAttack::Fire, TimeBetweenShots, true, FirstDelay);
}

void ATachyonAttack::EndFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
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
			Neutralize();
			
			InitAttack(GetOwner(), 1.0f, 0.0f);
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
	if (HasAuthority() && (BurstClass != nullptr))
	{
		FActorSpawnParameters SpawnParams;
		AActor* NewBurst = GetWorld()->SpawnActor<AActor>(BurstClass, GetActorLocation(), GetActorRotation(), SpawnParams);
		if (NewBurst != nullptr)
		{
			NewBurst->AttachToActor(OwningShooter, FAttachmentTransformRules::KeepWorldTransform);
			
			float VisibleMagnitude = FMath::Clamp(AttackMagnitude, 0.5f, 1.0f);
			FVector NewBurstScale = NewBurst->GetActorRelativeScale3D() * VisibleMagnitude;
			NewBurst->SetActorRelativeScale3D(NewBurstScale);
			
			NewBurst->SetLifeSpan(AttackMagnitude);
		}
	}
}


void ATachyonAttack::InitAttack(AActor* Shooter, float Magnitude, float YScale)
{
	OwningShooter = Shooter;
	if (OwningShooter != nullptr)
	{
		AttackMagnitude = FMath::Clamp(Magnitude, 0.1f, 1.0f);
		AttackDirection = YScale;
		AttackDamage = (1.0f * AttackMagnitude);

		// Attack magnitude characteristics
		/*float MagnitudeDelivery = (DeliveryTime * AttackMagnitude) * 0.5555f;
		DeliveryTime = (DeliveryTime + MagnitudeDelivery);

		float ModifiedHitsPerSecond = HitsPerSecond * (AttackMagnitude * 2.1f);
		HitsPerSecond = (HitsPerSecond + ModifiedHitsPerSecond);

		float ModifiedKineticForce = (KineticForce * AttackMagnitude) * 0.2222f;
		KineticForce += ModifiedKineticForce;*/

		// Movement and VFX
		RedirectAttack();
		SetInitVelocities();
		SpawnBurst();

		// Lifetime
		DynamicLifetime = (DeliveryTime + LethalTime + DurationTime);
		///SetLifeSpan(DynamicLifetime * 1.15f);

		bInitialized = true;
		bNeutralized = false;

		//ForceNetUpdate();

		///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("Inited attack"));
		///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackMagnitude: %f"), AttackMagnitude));
		///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("HitsPerSecond:   %f"), HitsPerSecond));
		///GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::White, FString::Printf(TEXT("AttackDamage:    %f"), AttackDamage));
	}
}


void ATachyonAttack::Lethalize()
{
	bLethal = true;
	bDoneLethal = false;

	// Shooter Recoil
	if (RecoilForce > 0.0f)
	{
		ATachyonCharacter* CharacterShooter = Cast<ATachyonCharacter>(OwningShooter);
		if (CharacterShooter != nullptr)
		{
			FVector RecoilVector = GetActorRotation().Vector().GetSafeNormal();
			float ClampedMagnitude = FMath::Clamp(AttackMagnitude, 0.5f, 1.0f);
			RecoilVector *= (RecoilForce * -ClampedMagnitude);
			CharacterShooter->ReceiveKnockback(RecoilVector, true);
		}
	}

	// Custom timescale
	//AttackParticles->CustomTimeDilation = FMath::Clamp((1.777f - AttackMagnitude), 0.1f, 1.0f);
}


void ATachyonAttack::ActivateEffects_Implementation()
{
	ActivateParticles();
	ActivateSound();
	if (FireShake != nullptr)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), FireShake, GetActorLocation(), 0.0f, 5555.0f, 1.0f, false);
	}
}


void ATachyonAttack::ActivateSound()
{
	if (AttackSound != nullptr)
	{
		float ModifiedPitch = AttackSound->PitchMultiplier * (AttackMagnitude);
		AttackSound->SetPitchMultiplier(ModifiedPitch);
		AttackSound->Activate();
	}
}


void ATachyonAttack::ActivateParticles()
{
	if (AttackParticles != nullptr)
	{
		AttackParticles->Activate();
	}
}


void ATachyonAttack::SetInitVelocities()
{
	FVector ShooterVelocity = OwningShooter->GetVelocity();

	// Inherit some velocity from owning shooter
	if ((ProjectileComponent != nullptr) && (ProjectileSpeed != 0.0f))
	{
		ProjectileComponent->Velocity = GetActorForwardVector() * ProjectileSpeed;
		
		float VelSize = ShooterVelocity.Size();
		if (VelSize != 0.0f)
		{
			FVector ScalarVelocity = ShooterVelocity.GetSafeNormal() * FMath::Sqrt(VelSize) * ProjectileSpeed;
			ScalarVelocity.Z *= 0.21f;
			ProjectileComponent->Velocity += ScalarVelocity;
		}
	}

	// Slow shooter
	/*FVector PostFireShooterVelocity = (ShooterVelocity * -HitSlow);
	ATachyonCharacter* CharacterShooter = Cast<ATachyonCharacter>(OwningShooter);
	if (CharacterShooter != nullptr)
	{
		CharacterShooter->ReceiveKnockback(PostFireShooterVelocity, true);
	}*/
}


void ATachyonAttack::RedirectAttack()
{
	if (OwningShooter != nullptr)
	{
		ATachyonCharacter* TachyonShooter = Cast<ATachyonCharacter>(OwningShooter);
		if (TachyonShooter != nullptr)
		{
			FVector LocalForward = TachyonShooter->GetActorForwardVector();
			LocalForward.Y = 0.0f;
			float ShooterAimDirection = FMath::Clamp(OwningShooter->GetActorForwardVector().Z * ShootingAngle, -1.0f, 1.0f);
			float TargetPitch = ShootingAngle * ShooterAimDirection;
	
			FRotator NewRotation = LocalForward.Rotation() + FRotator(TargetPitch, 0.0f, 0.0f);
			
			// Clamp Yaw for feels
			float ShooterYaw = FMath::Abs(OwningShooter->GetActorRotation().Yaw);
			float Yaw = 0.0f;
			if ((ShooterYaw > 50.0f))
			{
				Yaw = 180.0f;
			}
			NewRotation.Yaw = Yaw;
			
			NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, -ShootingAngle, ShootingAngle);
			
			SetActorRotation(NewRotation);
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("ShooterAimDirection: %f"), ShooterAimDirection));
		}
		
		FVector EmitLocation;
		FRotator EmitRotation;
		OwningShooter->GetActorEyesViewPoint(EmitLocation, EmitRotation);
		SetActorLocation(EmitLocation);
	}
}


// Called every frame
void ATachyonAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInitialized && !bNeutralized)
	{
		UpdateLifeTime(DeltaTime);
	}
}


// Update life-cycle
void ATachyonAttack::UpdateLifeTime(float DeltaT)
{
	float CustomDeltaTime = (1.0f / CustomTimeDilation) * DeltaT;
	LifeTimer += CustomDeltaTime;

	// Catch lost game ender
	if (bGameEnder && (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) != 0.01f))
	{
		CallForTimescale(this, true, 0.01f);
	}

	// Responsive aim for flex shots
	if (LifeTimer <= (DeliveryTime * 1.5f))
	{
		RedirectAttack();
	}


	// Attack main line
	if (!bDoneLethal && (LifeTimer >= DeliveryTime))
	{
		
		if (!bLethal)
		{
			Lethalize();
		}
		else
		{
			// Hits Per Second
			float GlobalDilation = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
			if (GlobalDilation > 0.01f)
			{
				HitTimer += DeltaT;
				if (HitTimer >= ((1.0f / HitsPerSecond) * GlobalDilation))
				{

					// Powe
					RaycastForHit(GetActorForwardVector());
					HitTimer = 0.0f;
				}
			}
		}
	}

	// Time to go
	if ((LifeTimer >= (DeliveryTime + LethalTime))
		&& bLethal)
	{
		bDoneLethal = true;
		bLethal = false;
	}

	if (LifeTimer >= DynamicLifetime)
	{
		Neutralize();
	}
}


// Shooter Input Enabling
void ATachyonAttack::SetShooterInputEnabled(bool bEnabled)
{
	// Re-enable Shooter's Input
	ACharacter* CharacterShooter = Cast<ACharacter>(OwningShooter);
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


// Raycast firing solution
void ATachyonAttack::RaycastForHit(FVector RaycastVector)
{
	// Linecast ingredients
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
	TArray<FHitResult> Hits;
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(OwningShooter);
	IgnoredActors.Add(this->GetOwner());

	// Set up ray position
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
			
			if ((HitActor != nullptr)
				&& (HitActor->WasRecentlyRendered(0.2f)))
			{
				MainHit(HitActor, Hits[i].ImpactPoint);
			}
		}
	}

	ActivateEffects();

	LastFireTime = GetWorld()->TimeSeconds;
}


void ATachyonAttack::SpawnHit(AActor* HitActor, FVector HitLocation)
{
	if (HasAuthority())
	{
		if (DamageClass != nullptr)
		{
			FActorSpawnParameters SpawnParams;
			FVector ToHitLocation = (HitLocation - GetActorLocation()).GetSafeNormal();
			AActor* HitSpawning = GetWorld()->SpawnActor<AActor>(DamageClass, HitLocation, ToHitLocation.Rotation(), SpawnParams);
			if (HitSpawning != nullptr)
			{
				HitSpawning->AttachToActor(HitActor, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}


void ATachyonAttack::ApplyKnockForce(AActor* HitActor, FVector HitLocation, float HitScalar)
{
	FVector KnockDirection = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector KnockVector = KnockDirection * (KineticForce * HitScalar * AttackMagnitude);
	KnockVector.Y = 0.0f;

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
	}

	// Smashy fx
	if (!bGameEnder)
	{
		SpawnHit(HitActor, HitLocation);
		ApplyKnockForce(HitActor, HitLocation, 1.0f);
	}

	// Update GameState
	ReportHitToMatch(OwningShooter, HitActor);

	HitsPerSecond *= 0.5f;
}
void ATachyonAttack::ServerMainHit_Implementation(AActor* HitActor, FVector HitLocation)
{
	MainHit(HitActor, HitLocation);
}
bool ATachyonAttack::ServerMainHit_Validate(AActor* HitActor, FVector HitLocation)
{
	return true;
}


void ATachyonAttack::ReportHitToMatch(AActor* Shooter, AActor* Mark)
{
	ATachyonCharacter* HitTachyon = Cast<ATachyonCharacter>(Mark);
	if (HitTachyon != nullptr)
	{
		NumHits += 1;
		AttackDamage = NumHits;

		// Update Shooter's Opponent reference
		ATachyonCharacter* OwningTachyon = Cast<ATachyonCharacter>(OwningShooter);
		if (OwningTachyon != nullptr)
		{
			if (OwningTachyon->GetOpponent() != HitTachyon)
			{
				OwningTachyon->SetOpponent(HitTachyon);
			}
		}

		// Modify Health of Mark
		HitTachyon->ModifyHealth(-AttackDamage);

		// Call it in
		float TachyonHealth = HitTachyon->GetHealth();
		if (TachyonHealth <= 0.0f)
		{
			// Deadly blow
			CallForTimescale(Mark, false, 0.01f); /// 0.01 is Terminal timescale
			bGameEnder = true;
		}
		else
		{
			// Basic hits
			if (!bFirstHitReported)
			{
				float ImpactScalar = AttackMagnitude;
				float HitTimescale = FMath::Clamp((1.0f - ImpactScalar), 0.07f, 1.0f);
				CallForTimescale(Mark, false, HitTimescale);
				bFirstHitReported = true;
			}
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
		}
		else
		{
			TGState->SetActorTimescale(TargetActor, NewTimescale);
			TGState->SetActorTimescale(OwningShooter, NewTimescale);
		}
		
		TGState->ForceNetUpdate();
	}
}


void ATachyonAttack::Neutralize()
{
	// Reset variables
	bNeutralized = true;
	bInitialized = false;
	bDoneLethal = false;
	bLethal = false;
	bGameEnder = false;
	bFirstHitReported = false;
	LifeTimer = 0.0f;
	HitTimer = (1.0f / HitsPerSecond);
	HitsPerSecond = 50.0f;
	
	// Reset components
	if (AttackParticles != nullptr)
	{
		AttackParticles->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		AttackParticles->Deactivate();
	}
	if (AttackSound != nullptr)
	{
		AttackSound->Deactivate();
	}
	if (AttackRadial != nullptr)
	{
		AttackRadial->SetWorldLocation(GetActorLocation());
	}

	// Reset rotation
	FRotator MyRotation = GetActorRotation();
	MyRotation.Pitch = 0.0f;
	SetActorRotation(MyRotation);
}


// COLLISION BEGIN
void ATachyonAttack::OnAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	bool bTime = (HitTimer >= (1 / HitsPerSecond));
	bool bActors = (OwningShooter != nullptr) && (OtherActor != nullptr);
	float TimeSc = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (bTime && bActors && (bLethal && !bDoneLethal) && (OtherActor != OwningShooter) && (TimeSc > 0.5f))
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


// NETWORK VARIABLES
//void ATachyonAttack::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//	DOREPLIFETIME_CONDITIONAL(ATachyonAttack, ATachyonAttack);
//	/*DOREPLIFETIME(ATachyonAttack, bInitialized);
//	DOREPLIFETIME(ATachyonAttack, OwningShooter);
//	DOREPLIFETIME(ATachyonAttack, HitActor);
//	DOREPLIFETIME(ATachyonAttack, DynamicLifetime);
//	DOREPLIFETIME(ATachyonAttack, LethalTime);
//	DOREPLIFETIME(ATachyonAttack, bHit);
//	DOREPLIFETIME(ATachyonAttack, NumHits);
//	DOREPLIFETIME(ATachyonAttack, AttackMagnitude);
//	DOREPLIFETIME(ATachyonAttack, AttackDirection);
//	DOREPLIFETIME(ATachyonAttack, AttackDamage);
//	DOREPLIFETIME(ATachyonAttack, HitTimer);
//	DOREPLIFETIME(ATachyonAttack, bLethal);
//	DOREPLIFETIME(ATachyonAttack, bDoneLethal);
//	DOREPLIFETIME(ATachyonAttack, bFirstHitReported);
//	DOREPLIFETIME(ATachyonAttack, bNeutralized);*/
//}
