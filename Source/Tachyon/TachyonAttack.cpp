// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonAttack.h"
#include "TachyonCharacter.h"
#include "TachyonGameStateBase.h"
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
	AttackSound->bAutoActivate = false;

	ProjectileComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileComponent->ProjectileGravityScale = 0.0f;
	ProjectileComponent->SetIsReplicated(true);

	AttackRadial = CreateDefaultSubobject<URadialForceComponent>(TEXT("AttackRadial"));

	bReplicates = true;
	bReplicateMovement = true;
}

// Called when the game starts or when spawned
void ATachyonAttack::BeginPlay()
{
	Super::BeginPlay();
	bLethal = false;
	HitTimer = (1.0f / HitsPerSecond);
	if (AttackParticles != nullptr)
	{
		AttackParticles->Deactivate();
	}
	if (AttackRadial != nullptr)
	{
		AttackRadial->SetWorldLocation(GetActorLocation());
	}
}

void ATachyonAttack::SpawnBurst()
{
	if (BurstClass != nullptr)
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

		// Attack magnitude characteristics
		float MagnitudeDelivery = (DeliveryTime * AttackMagnitude) * 0.5555f;
		DeliveryTime += MagnitudeDelivery;

		float ModifiedHitsPerSecond = HitsPerSecond * (AttackMagnitude * 2.1f);
		HitsPerSecond += ModifiedHitsPerSecond;

		float ModifiedKineticForce = (KineticForce * AttackMagnitude) * 0.2222f;
		KineticForce += ModifiedKineticForce;

		// Movement and VFX
		RedirectAttack();
		SetInitVelocities();
		SpawnBurst();

		if (FireShake != nullptr)
		{
			UGameplayStatics::PlayWorldCameraShake(GetWorld(), FireShake, GetActorLocation(), 0.0f, 5555.0f, 1.0f, false);
		}

		// Lifetime
		DynamicLifetime = (DeliveryTime + LethalTime + DurationTime);
		
		bInitialized = true;
	}
}

void ATachyonAttack::Lethalize()
{
	bLethal = true;
	bDoneLethal = true;

	if (AttackSound != nullptr)
	{
		float ModifiedPitch = AttackSound->PitchMultiplier * (AttackMagnitude);
		AttackSound->SetPitchMultiplier(ModifiedPitch);
		AttackSound->Activate();
	}

	if (AttackParticles != nullptr)
	{
		float ParticleSize = FMath::Clamp((AttackMagnitude * 2.0f), 0.5f, 1.5f);
		FVector ModifiedScale = AttackParticles->GetComponentScale();
		ModifiedScale.Z *= ParticleSize;
		ModifiedScale.Y *= ParticleSize;
		AttackParticles->SetRelativeScale3D(ModifiedScale);
		AttackParticles->Activate();
	}

	// Shooter Recoil
	FVector RecoilVector = GetActorRotation().Vector().GetSafeNormal();
	ACharacter* CharacterShooter = Cast<ACharacter>(OwningShooter);
	if (CharacterShooter != nullptr)
	{
		FVector ShooterVelocity = CharacterShooter->GetCharacterMovement()->Velocity;
		RecoilVector *= (RecoilForce * -AttackMagnitude);
		CharacterShooter->GetCharacterMovement()->Velocity = (ShooterVelocity + RecoilVector);
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
			ScalarVelocity.Z *= 0.315f;
			ProjectileComponent->Velocity += ScalarVelocity;
		}
	}

	// Slow shooter
	FVector PostFireShooterVelocity = (ShooterVelocity * HitSlow);
	ACharacter* CharacterShooter = Cast<ACharacter>(OwningShooter);
	if (CharacterShooter != nullptr)
	{
		CharacterShooter->GetCharacterMovement()->Velocity = PostFireShooterVelocity;
	}
}

void ATachyonAttack::RedirectAttack()
{
	// Aim by InputY
	float AimClampedInputZ = FMath::Clamp((AttackDirection * 10.0f), -1.0f, 1.0f);
	FVector FirePosition = AttackScene->GetComponentLocation();
	FVector LocalForward = AttackScene->GetForwardVector();
	LocalForward.Y = 0.0f;
	FRotator FireRotation = LocalForward.Rotation() + FRotator(AimClampedInputZ * ShootingAngle, 0.0f, 0.0f); /// AimClampedInputZ
	FireRotation.Pitch = FMath::Clamp(FireRotation.Pitch, -ShootingAngle, ShootingAngle);
	float ShooterYaw = FMath::Abs(OwningShooter->GetActorRotation().Yaw);
	float Yaw = 0.0f;
	if ((ShooterYaw > 50.0f)) {
		Yaw = 180.0f;
	}
	FireRotation.Yaw = Yaw;
	
	SetActorRotation(FireRotation);
}

// Called every frame
void ATachyonAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInitialized)
	{
		UpdateLifeTime(DeltaTime);
	}
}


// Update life-cycle
void ATachyonAttack::UpdateLifeTime(float DeltaT)
{
	LifeTimer += DeltaT;

	if (LifeTimer < DeliveryTime)
	{
		RedirectAttack();
	}

	if (bLethal)
	{
		float TimeProofDelta = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		HitTimer += DeltaT;
		if (HitTimer >= ((1.0f / HitsPerSecond) * TimeProofDelta))
		{
			RaycastForHit(GetActorForwardVector());
			HitTimer = 0.0f;
		}
	}

	if ((LifeTimer >= DeliveryTime)
		&& !bLethal && !bDoneLethal)
	{
		Lethalize();
	}

	if ((LifeTimer >= (DeliveryTime + LethalTime))
		&& bLethal)
	{
		bLethal = false;
	}

	if (LifeTimer >= DynamicLifetime)
	{
		Destroy();
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
}

void ATachyonAttack::SpawnHit(AActor* HitActor, FVector HitLocation)
{
	FActorSpawnParameters SpawnParams;
	FVector ToHitLocation = (HitLocation - GetActorLocation()).GetSafeNormal();
	AActor* HitSpawning = GetWorld()->SpawnActor<AActor>(DamageClass, HitLocation, ToHitLocation.Rotation(), SpawnParams);
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
		/*FVector CurrentCharVel = Chara->GetCharacterMovement()->Velocity;
		Chara->GetCharacterMovement()->Velocity += KnockVector;*/
		Chara->GetCharacterMovement()->AddImpulse(KnockVector, true);
	}
}

void ATachyonAttack::MainHit(AActor* HitActor, FVector HitLocation)
{
	ATachyonAttack* PotentialAttack = Cast<ATachyonAttack>(HitActor);
	if (PotentialAttack != nullptr)
	{
		if (PotentialAttack->OwningShooter == this->OwningShooter)
		{
			return;
		}
	}

	// Smashy fx
	SpawnHit(HitActor, HitLocation);
	ApplyKnockForce(HitActor, HitLocation, 1.0f);

	// Test damage
	ATachyonCharacter* PotentialCharacter = Cast<ATachyonCharacter>(HitActor);
	if (PotentialCharacter != nullptr)
	{
		PotentialCharacter->ModifyHealth(-AttackDamage);
	}

	// Update GameState
	ReportHitToMatch(OwningShooter, HitActor);
}

void ATachyonAttack::ReportHitToMatch(AActor* Shooter, AActor* Mark)
{
	ATachyonCharacter* MarkTachyon = Cast<ATachyonCharacter>(Mark);
	if (MarkTachyon != nullptr)
	{
		if (!bFirstHitReported)
		{
			
			ATachyonGameStateBase* GameState = Cast<ATachyonGameStateBase>(GetWorld()->GetGameState());
			if (GameState != nullptr)
			{
				float ImpactScalar = AttackMagnitude * 1.5f;
				float HitTimescale = FMath::Clamp((1.0f - ImpactScalar), 0.05f, 0.1f);
				GameState->SetGlobalTimescale(HitTimescale);
				bFirstHitReported = true;
				NumHits += 1;
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
	DOREPLIFETIME(ATachyonAttack, LifeTimer);
	DOREPLIFETIME(ATachyonAttack, HitTimer);
	DOREPLIFETIME(ATachyonAttack, bLethal);
	DOREPLIFETIME(ATachyonAttack, bDoneLethal);
	DOREPLIFETIME(ATachyonAttack, bFirstHitReported);
}
