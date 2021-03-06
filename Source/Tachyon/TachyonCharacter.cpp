// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonCharacter.h"
#include "TachyonJump.h"
#include "TachyonGameStateBase.h"
#include "TachyonMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "TachyonGameStateBase.h"


// Sets default values
ATachyonCharacter::ATachyonCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UTachyonMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Configure character movement
	///GetCapsuleComponent()->SetIsReplicated(true);
	GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
	GetCapsuleComponent()->SetCapsuleRadius(34.0f);

	///GetMesh()->SetIsReplicated(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(RootComponent);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create a camera and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = true;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	AttackScene = CreateDefaultSubobject<USceneComponent>(TEXT("AttackScene"));
	AttackScene->SetupAttachment(RootComponent);

	AmbientParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AmbientParticles"));
	AmbientParticles->SetupAttachment(RootComponent);
	AmbientParticles->bAbsoluteRotation = true;

	SoundComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundComp"));
	SoundComp->SetupAttachment(RootComponent);
	SoundComp->VolumeMultiplier = 0.1f;

	OuterTouchCollider = CreateDefaultSubobject<USphereComponent>(TEXT("OuterTouchCollider"));
	OuterTouchCollider->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	OuterTouchCollider->OnComponentBeginOverlap.AddDynamic(this, &ATachyonCharacter::OnShieldBeginOverlap);
	
	bReplicates = true;
	bReplicateMovement = true;
	ReplicatedMovement.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
	ReplicatedMovement.VelocityQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
	NetDormancy = ENetDormancy::DORM_Never;

	
}


////////////////////////////////////////////////////////////////////////
// BEGIN PLAY

void ATachyonCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Helps with networked movement
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("p.NetEnableMoveCombining 0"));

	///DonApparel();
	Health = MaxHealth;
	Tags.Add("Player");
	Tags.Add("FramingActor");

	LastFaceDirection = GetActorForwardVector().X;

	GetCharacterMovement()->MaxAcceleration = MoveSpeed;
	GetCharacterMovement()->MaxFlySpeed = MaxMoveSpeed;
	//GetCharacterMovement()->bOrientRotationToMovement = true;
	//GetCharacterMovement()->BrakingFrictionFactor = 50.0f;
	GetCharacterMovement()->BrakingFrictionFactor = BrakeStrength;

	SideViewCameraComponent->PostProcessSettings.VignetteIntensity = 0.0f;

	// Spawn player's weapon & jump objects
	SpawnAbilities();
}

void ATachyonCharacter::SpawnAbilities()
{
	if ((Role == ROLE_Authority) || ActorHasTag("Bot"))
	{
		FActorSpawnParameters SpawnParams;
		FVector SpawnLoca = AttackScene->GetComponentLocation();
		FRotator SpawnRota = GetActorForwardVector().Rotation();

		if (SecondaryClass != nullptr)
		{
			ActiveSecondary = GetWorld()->SpawnActor<ATachyonAttack>(SecondaryClass, SpawnLoca, SpawnRota, SpawnParams);
			if (ActiveSecondary != nullptr)
			{
				ActiveSecondary->SetOwner(this);
				
				if (ActiveSecondary->IsLockedEmitPoint())
				{
					ActiveSecondary->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
					///ActiveSecondary->Neutralize();
				}
			}
		}
		
		if (AttackClass != nullptr)
		{
			ActiveAttack = GetWorld()->SpawnActor<ATachyonAttack>(AttackClass, SpawnLoca, SpawnRota, SpawnParams);
			if (ActiveAttack != nullptr)
			{
				ActiveAttack->SetOwner(this);
				
				/*if (ActiveAttack->IsLockedEmitPoint())
				{
					ActiveAttack->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				}*/

				///ActiveAttack->Neutralize();
			}
		}

		if (BoostClass != nullptr)
		{
			FRotator InputRotation = GetActorRotation();
			ActiveBoost = GetWorld()->SpawnActor<ATachyonJump>(BoostClass, GetActorLocation(), InputRotation, SpawnParams);
			if (ActiveBoost != nullptr)
			{
				ActiveBoost->SetOwner(this);
				ActiveBoost->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			}
		}

		/*if (NearDeathEffect != nullptr)
		{
			ActiveDeath = UGameplayStatics::SpawnEmitterAttached(NearDeathEffect, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
			if (ActiveDeath != nullptr)
			{
				ActiveDeath->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				ActiveDeath->Deactivate();
			}
		}*/
	}
}

void ATachyonCharacter::SetDynamicMoveSpeed()
{
	float Result = -1.0f;
	float ConsideredDistance = -1.0f;
	FVector MyVelocityPosition = GetActorLocation() + GetCharacterMovement()->Velocity;
	FVector OpponentVelPosition = Opponent->GetActorLocation() + Opponent->GetCharacterMovement()->Velocity;
	float DistToOpponentVelocity = FVector::Dist(MyVelocityPosition, OpponentVelPosition);
	float DistToOpponentPosition = FVector::Dist(GetActorLocation(), Opponent->GetActorLocation());
	if (DistToOpponentPosition < DistToOpponentVelocity)
	{
		ConsideredDistance = DistToOpponentPosition;
	}
	else {
		ConsideredDistance = DistToOpponentVelocity;
	}
	float FightSpeed = FMath::Clamp((1.0f / ConsideredDistance) * FightSpeedInfluence, FightSpeedInfluence * 0.00001f, FightSpeedInfluence);
	float NewMaxAccel = MoveSpeed * (MoveSpeed * FMath::Clamp(FightSpeed, 0.0f, 10.0f));
	NewMaxAccel = FMath::Clamp(NewMaxAccel, MoveSpeed, MaxMoveSpeed);
	GetCharacterMovement()->MaxAcceleration = NewMaxAccel;

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, FString::Printf(TEXT("MaxAcceleration: %f"), NewMaxAccel));
}


////////////////////////////////////////////////////////////////////////
// TICK
void ATachyonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetController() != nullptr)
	{
		UpdateHealth(DeltaTime);
		UpdateCamera(DeltaTime);

		float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		if (GlobalTimeScale == 1.0f)
		{
			// Body rotation
			if (!ActorHasTag("Bot"))
			{
				UpdateBody(DeltaTime);
			}
		}

		// Timescale recovery
		if (Role == ROLE_Authority)
		{
			if (CustomTimeDilation < 1.0f)
			{
				ServerUpdateBody(DeltaTime);
			}
		}

		// Map bounds
		FVector ToCentre = FVector::ZeroVector - GetActorLocation();
		if (ToCentre.Size() >= WorldRange)
		{
			float CurrentV = GetCharacterMovement()->Velocity.Size();
			GetCharacterMovement()->Velocity = ToCentre.GetSafeNormal() * CurrentV;
		}
	}

	

	// Spawn death effect
	if (((Health <= 0.0f) || (CustomTimeDilation < 0.01f)) && !bSpawnedDeath)
	{
		if (NearDeathEffect != nullptr)
		{
			ActiveDeath = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), NearDeathEffect, GetActorTransform(), true);
			if (ActiveDeath != nullptr)
			{
				ActiveDeath->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				ActiveDeath->SetWorldTransform(GetActorTransform());
				bSpawnedDeath = true;
				ActiveDeath->bAutoDestroy = true;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////
// INPUT BINDING
void ATachyonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Actions
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ATachyonCharacter::StartFire);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ATachyonCharacter::EndFire);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATachyonCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ATachyonCharacter::EndJump);
	PlayerInputComponent->BindAction("Brake", IE_Pressed, this, &ATachyonCharacter::StartBrake);
	PlayerInputComponent->BindAction("Brake", IE_Released, this, &ATachyonCharacter::EndBrake);
	
	PlayerInputComponent->BindAction("Recover", IE_Pressed, this, &ATachyonCharacter::Recover);
	PlayerInputComponent->BindAction("SummonBot", IE_Pressed, this, &ATachyonCharacter::RequestBots);
	PlayerInputComponent->BindAction("Shield", IE_Pressed, this, &ATachyonCharacter::Shield);
	PlayerInputComponent->BindAction("Restart", IE_Pressed, this, &ATachyonCharacter::RestartGame);

	// Axes
	PlayerInputComponent->BindAxis("MoveRight", this, &ATachyonCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ATachyonCharacter::MoveUp);
}


////////////////////////////////////////////////////////////////////////
// MOVEMENT
// Left / Right
void ATachyonCharacter::MoveRight(float Value)
{
	if (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) > 0.5f)
	{
		AddMovementInput(FVector::ForwardVector, Value);

		if (InputX != Value)
		{
			InputX = Value;
		}
		/*else
		{
			if ((ActiveBoost != nullptr) && (Value != 0.0f))
			{
				StartJump();
			}
		}*/

		if (Value != 0.0f)
		{
			LastFaceDirection = Value;
		}
	}
}


// Up / Down
void ATachyonCharacter::MoveUp(float Value)
{
	if (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) > 0.5f)
	{
		AddMovementInput(FVector::UpVector, Value * 0.88f);

		if (InputZ != Value)
		{
			InputZ = Value;
		}

		/*if ((ActiveBoost != nullptr) && (Value != 0.0f))
		{
			StartJump();
		}*/
	}
}

/*

FVector MyVelocity = GetCharacterMovement()->Velocity;
	float MoveVal = Value;
	
	if (MyVelocity.Size() > 0.0f)
	{
		FVector MyInput = FVector(InputX, 0.0f, InputZ).GetSafeNormal();
		FVector VelNorm = MyVelocity.GetSafeNormal();
		float InputDotToVelocity = FVector::DotProduct(MyInput, VelNorm);
		float AngleToVelocity = FMath::Square(FMath::Clamp(FMath::Acos(InputDotToVelocity), 1.0f, 180.0f));

		// Adjust acceleration based on angle to input velocity
		if (GetCharacterMovement()->MaxAcceleration != BoostSpeed)
		{
			GetCharacterMovement()->MaxAcceleration = MoveSpeed * AngleToVelocity;
		}
		
		MoveVal = (Value * AngleToVelocity);
	}

*/

void ATachyonCharacter::BotMove(float X, float Z)
{
	if (ActorHasTag("Bot"))
	{
		MoveRight(X);
		MoveUp(Z);

		InputX = X;
		InputZ = Z;

		SetX(X);
		SetZ(Z);
	}
}

////////////////////////////////////////////////////////////////////////
// JUMP
void ATachyonCharacter::StartJump()
{
	if ((ActiveBoost != nullptr)
		&& ((InputX != 0.0f) || (InputZ != 0.0f)))
	{
		ActiveBoost->StartJump();
	}

	/*if ((ActiveAttack != nullptr) && !ActiveAttack->IsArmed())
	{
		if (ActiveBoost != nullptr)
		{
			ActiveBoost->StartJump();
		}
	}*/
}

void ATachyonCharacter::EndJump()
{
	if (ActiveBoost != nullptr)
	{
		ActiveBoost->EndJump();
	}
}

void ATachyonCharacter::EngageJump()
{
	/// it could just be 1
	float DilationSpeed = 1.0f; // FMath::Clamp((1.0f / CustomTimeDilation), 1.0f, 100.0f); //FMath::Sqrt(CustomTimeDilation);
	float JumpSpeed = BoostSpeed * DilationSpeed;
	float JumpTopSpeed = BoostSustain * DilationSpeed;

	GetCharacterMovement()->MaxAcceleration = JumpSpeed;
	GetCharacterMovement()->MaxFlySpeed = JumpTopSpeed;
}

void ATachyonCharacter::DisengageJump()
{
	GetCharacterMovement()->MaxAcceleration = MoveSpeed;
	GetCharacterMovement()->MaxFlySpeed = MaxMoveSpeed;
}

///////////////////////////////////////////////////////////////////////
// HANDBRAKE
void ATachyonCharacter::StartBrake()
{
	GetCharacterMovement()->BrakingFrictionFactor = BrakeStrength * BrakeStrength;

	/*if ((ActiveBoost != nullptr)
		&& ((InputX != 0.0f) || (InputZ != 0.0f)))
	{
		StartJump();
	}*/
}

void ATachyonCharacter::EndBrake()
{
	GetCharacterMovement()->BrakingFrictionFactor = BrakeStrength;
	
	/*if (ActiveBoost != nullptr)
	{
		EndJump();
	}*/
}

void ATachyonCharacter::Recover()
{
	if (Role < ROLE_Authority)
	{
		ServerRecover();
	}
	else 
	{
		if (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) == 1.0f)
		{
			// recoverable
			if (MaxTimescale < 1.0f)
			{
				// significant
				if ((MaxTimescale < 0.88f) && (MaxHealth > 1.0f)) /// ((MaxTimescale - CustomTimeDilation) > (0.1f * CustomTimeDilation))
				{
					float HealthCost = FMath::Abs(1.0f - CustomTimeDilation) * RecoverStrength * 10.0f;

					MaxTimescale = FMath::Clamp((MaxTimescale + RecoverStrength), 0.1f, 1.0f);
					CustomTimeDilation *= 0.618f; // = MaxTimescale * 0.5f;
					ModifyHealth(-HealthCost, false);

					if (RecoverEffect != nullptr)
					{
						FVector Loc = GetActorLocation();
						FRotator Roc = GetActorRotation();
						UGameplayStatics::SpawnEmitterAttached(RecoverEffect, GetRootComponent(), NAME_None, Loc, Roc, EAttachLocation::KeepWorldPosition);
					}
				}
			}
		}
	}
}
void ATachyonCharacter::ServerRecover_Implementation()
{
	Recover();
}
bool ATachyonCharacter::ServerRecover_Validate()
{
	return true;
}


void ATachyonCharacter::SetTimescaleRecoverySpeed(float Value)
{
	if (GetController() != nullptr)
	{
		TimescaleRecoverySpeed = Value;
	}
}

////////////////////////////////////////////////////////////////////////
// ATTACKING
void ATachyonCharacter::StartFire()
{
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->StartFire();
	}

	if (Aimer != nullptr) /// (!Aimer->IsVisible())
	{
		Aimer->SetVisibility(true);
	}
}

void ATachyonCharacter::EndFire()
{
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->EndFire();
	}

	if (Aimer != nullptr) /// (Aimer->IsVisible())
	{
		Aimer->SetVisibility(false);
	}
}

void ATachyonCharacter::Shield()
{
	if (ActiveSecondary != nullptr)
	{
		ActiveSecondary->StartFire();
	}
}


////////////////////////////////////////////////////////////////////////
// HEALTH & TIME
void ATachyonCharacter::ModifyHealth(float Value, bool Lethal)
{
	if (Role < ROLE_Authority)
	{
		ServerModifyHealth(Value, Lethal);
	}

	float SafeHealth = FMath::Clamp(Health, 0.0f, 100.0f);
	if (Lethal)
	{
		MaxHealth = FMath::Clamp(Health + Value, 0.0f, 100.0f);
	}
	else
	{
		MaxHealth = FMath::Clamp(Health + Value, 1.0f, 100.0f);
	}

	///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Ouch MaxHealth %f"), MaxHealth));

	// Clear old death if we're reviving
	if (Value == 100.0f)
	{
		bSpawnedDeath = false;
		SetMaxTimescale(1.0f);
		
		// Clear camera target
		Actor2 = nullptr;

		if (!GetMesh()->IsVisible())
		{
			GetMesh()->SetVisibility(true);
		}
	}
}
void ATachyonCharacter::ServerModifyHealth_Implementation(float Value, bool Lethal)
{
	if (GetOwner() != nullptr)
	{
		ModifyHealth(Value, Lethal);
	}
}
bool ATachyonCharacter::ServerModifyHealth_Validate(float Value, bool Lethal)
{
	return true;
}

void ATachyonCharacter::NewTimescale(float Value)
{
	if (Controller != nullptr)
	{
		ServerNewTimescale(Value);
	}

	if (Value == 0.01f)
	{
		if (ActiveAttack != nullptr)
		{
			ActiveAttack->ReceiveTimescale(0.001f);
		}
	}

	if (GetController() != nullptr)
	{
		float NewVignetteIntenso = 0.3f * FMath::Sqrt(1.0f / Value);
		NewVignetteIntenso = FMath::Clamp(NewVignetteIntenso, 0.0f, 1.0f);
		SideViewCameraComponent->PostProcessSettings.VignetteIntensity = NewVignetteIntenso;
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("NewVignetteIntenso: %f"), NewVignetteIntenso));
	}
}
void ATachyonCharacter::ServerNewTimescale_Implementation(float Value)
{
	MulticastNewTimescale(Value);
}
bool ATachyonCharacter::ServerNewTimescale_Validate(float Value)
{
	return true;
}
void ATachyonCharacter::MulticastNewTimescale_Implementation(float Value)
{
	CustomTimeDilation = Value;

	if (Value < 0.5f)
	{
		if (ActiveAttack != nullptr)
		{
			float AttackTimescale = FMath::Clamp(Value * 2.1f, 0.5f, 1.0f);
			ActiveAttack->ReceiveTimescale(AttackTimescale);
		}

		if (ActiveSecondary != nullptr)
		{
			float SecondaryTimescale = FMath::Clamp(Value * 2.1f, 0.5f, 1.0f);
			ActiveSecondary->ReceiveTimescale(SecondaryTimescale);
		}

		if (ActiveBoost != nullptr)
		{
			float JumpTimescale = Value;
			ActiveBoost->CustomTimeDilation = JumpTimescale;
		}
	}

	ForceNetUpdate();
}


////////////////////////////////////////////////////////////////////////
// CHARACTER UPDATES
void ATachyonCharacter::UpdateHealth(float DeltaTime)
{
	// Update smooth health value
	if (Health != MaxHealth)
	{
		float GlobalTime = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		float Timescalar = (1.0f / GlobalTime) + (1.0f / CustomTimeDilation);
		float HealthDifference = FMath::Clamp((FMath::Abs(Health - MaxHealth) * 100.0f), 1.0f, 100.0f);
		float InterpSpeed = Timescalar * FMath::Clamp(HealthDifference, 100.0f, 10000.0f);
		Health = FMath::FInterpConstantTo(Health, MaxHealth, DeltaTime, InterpSpeed);
		if (Health == 1.0f)
		{
			bSpawnedDeath = false;
		}
	}

	///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Health: %f"), Health));
}

void ATachyonCharacter::UpdateCamera(float DeltaTime)
{
	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());

	// Poll for framing actors
	/*if ((Actor1 == nullptr) || (Actor2 == nullptr))
	{
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("FramingActor"), FramingActors);
	}*/

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("FramingActor"), FramingActors);

	// Spectator ez life
	if (!this->ActorHasTag("Spectator"))
	{
		Actor1 = this;
	}

	// Focus target getting, Actor1 and Actor2
	if ((Controller != nullptr) && (FramingActors.Num() >= 1))
	{
		
		if (Actor1 == nullptr)
		{
			int LoopSize = FramingActors.Num();
			for (int i = 0; i < LoopSize; ++i)
			{
				AActor* Actore = FramingActors[i];
				if (Actore != nullptr
					&& !Actore->ActorHasTag("Spectator")
					&& Actore->ActorHasTag("Player"))
				{
					Actor1 = Actore;
					break;
				}
			}
		}

		// Camera Motion Header
		if (Actor1 != nullptr)
		{

			float VelocityCameraSpeed = CameraMoveSpeed;
			float CameraMaxSpeed = 10000.0f;
			float ConsideredDistanceScalar = CameraDistanceScalar;

			/// Position by another actor
			float DistToActor2 = 99999.0f;
			int LoopCount = FramingActors.Num();
			AActor* CurrentActor = nullptr;
			AActor* BestCandidate = nullptr;
			
			for (int i = 0; i < LoopCount; ++i)
			{
				CurrentActor = FramingActors[i];
				if (CurrentActor != nullptr
					&& CurrentActor != Actor1
					&& !CurrentActor->ActorHasTag("Spectator")
					&& !CurrentActor->ActorHasTag("Obstacle")
					&& !CurrentActor->ActorHasTag("Surface"))
				{
					float DistToTemp = FVector::Dist(CurrentActor->GetActorLocation(), GetActorLocation());
					if (DistToTemp < DistToActor2)
					{
						/// Players get veto importance
						if ((BestCandidate == nullptr)
						|| (!BestCandidate->ActorHasTag("Player")))
						{
							BestCandidate = CurrentActor;
							DistToActor2 = DistToTemp; //

							/*ATachyonCharacter* Potential = Cast<ATachyonCharacter>(CurrentActor);
							if (Potential != nullptr)
							{
								if (Potential->GetHealth() >= 1.0f)
								{
									BestCandidate = CurrentActor;
									DistToActor2 = DistToTemp;
								}
							}*/
						}
					}
				}
			}

			// Got your boye
			Actor2 = BestCandidate;


			// Framing up first actor with their own velocity
			FVector Actor1Velocity = Actor1->GetVelocity();
			float SafeVelocitySize = FMath::Clamp(Actor1Velocity.Size() * 0.005f, 0.01f, 10.0f);
			VelocityCameraSpeed = CameraMoveSpeed * SafeVelocitySize * FMath::Sqrt(1.0f / GlobalTimeScale);
			VelocityCameraSpeed = FMath::Clamp(VelocityCameraSpeed, 1.0f, CameraMaxSpeed * 0.1f) * (1.0f / CustomTimeDilation);

			FVector LocalPos = Actor1->GetActorLocation() + (Actor1Velocity * DeltaTime * CameraVelocityChase);
			PositionOne = FMath::VInterpTo(PositionOne, LocalPos, DeltaTime, VelocityCameraSpeed);

			// Setting up distance and speed dynamics
			float ChargeScalar = FMath::Clamp((FMath::Sqrt(Charge - 0.9f)), 1.0f, ChargeMax);
			float PlayerSpeed = Actor1Velocity.Size();
			float SpeedScalar = 1.0f;
			if (PlayerSpeed > 0.0f)
				SpeedScalar = FMath::Sqrt(PlayerSpeed) * 0.1f;

			float PersonalScalar = 1.0f + (36.0f * ChargeScalar * SpeedScalar) * (FMath::Sqrt(SafeVelocitySize));
			float CameraMinimumDistance = (3500.0f * CameraDistanceScalar) + (PersonalScalar * CameraDistanceScalar);
			float CameraMaxDistance = 11551000.0f;

			// If Actor2 is valid, make Pair Framing
			bool bAlone = true;
			if (Actor2 != nullptr)
			{

				// Distance check i.e pair bounds
				float PairDistanceThreshold = 2000.0f + FMath::Clamp(Actor1->GetVelocity().Size(), 1000.0f, 2000.0f);
				if (this->ActorHasTag("Spectator"))
				{
					PairDistanceThreshold *= 3.3f;
				}

				// Special care taken for vertical as we are probably widescreen
				float Vertical = FMath::Abs(( Actor2->GetActorLocation() - Actor1->GetActorLocation() ).Z);
				bool bInRange = (FVector::Dist(Actor1->GetActorLocation(), Actor2->GetActorLocation()) <= PairDistanceThreshold)
					&& (Vertical <= (PairDistanceThreshold * 0.9f));
				bool TargetVisible = Actor2->WasRecentlyRendered(0.01f);

				if (bInRange && TargetVisible
					&& Actor1->WasRecentlyRendered(0.2f))
				{
					bAlone = false;

					//// Framing up with second actor
					FVector Actor2Velocity = Actor2->GetVelocity();

					// Declare Position Two
					FVector PairFraming = Actor2->GetActorLocation() + (Actor2Velocity * DeltaTime * CameraVelocityChase);
					PositionTwo = FMath::VInterpTo(PositionTwo, PairFraming, DeltaTime, VelocityCameraSpeed);
				}
			}

			// Lone player gets Velocity Framing
			else if (bAlone || (FramingActors.Num() == 1))
			{
				// Framing lone player by their velocity
				Actor1Velocity = Actor1->GetVelocity() + GetActorForwardVector();

				// Declare Position Two
				FVector SoloPairing = Actor1Velocity;
				if (SoloPairing == FVector::ZeroVector)
				{
					SoloPairing = Actor1->GetActorForwardVector() * 100.0f;
				}
				FVector VelocityFraming = Actor1->GetActorLocation() + (SoloPairing * CameraSoloVelocityChase);
				PositionTwo = FMath::VInterpTo(PositionTwo, VelocityFraming, DeltaTime, VelocityCameraSpeed * 0.5f);

				CameraMinimumDistance *= 1.618f;

				Actor2 = nullptr;
			}


			// Positions done
			// Find the midpoint
			float MidpointBias = 0.5f;
			FVector TargetMidpoint = PositionOne + ((PositionTwo - PositionOne) * MidpointBias);
			float MidpointInterpSpeed = 10.0f * FMath::Clamp(TargetMidpoint.Size() * 0.01f, 10.0f, 100.0f);/// *(1.0f / CustomTimeDilation);
			if (!Actor1->WasRecentlyRendered(0.1f))
			{
				MidpointInterpSpeed *= 100.0f;
			}

			if (GlobalTimeScale < 1.0f)
			{
				MidpointInterpSpeed *= GlobalTimeScale;
			}

			Midpoint = FMath::VInterpTo(Midpoint, TargetMidpoint, DeltaTime, MidpointInterpSpeed); /// TargetMidpoint; /// 
			if (Midpoint.Size() > 0.0f)
			{

				// Distance
				float DistBetweenActors = FVector::Dist(PositionOne, PositionTwo);
				float ProcessedDist = 100.0f + (FMath::Sqrt(DistBetweenActors) * 1500.0f);
				float VerticalDist = FMath::Abs((PositionTwo - PositionOne).Z) * 10.0f;

				// Handle horizontal bias
				float DistancePreClamp = ProcessedDist + FMath::Sqrt(VerticalDist);
				///GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, FString::Printf(TEXT("DistancePreClamp: %f"), DistancePreClamp));
				float TargetLength = FMath::Clamp(DistancePreClamp, CameraMinimumDistance, CameraMaxDistance);

				// Modifier for prefire timing
				//if (PrefireTimer > 0.0f)
				//{
				//	float PrefireScalarRaw = PrefireTimer; //  FMath::Sqrt(PrefireTimer * 0.618f);
				//	float PrefireScalarClamped = FMath::Clamp(PrefireScalarRaw, 0.1f, 0.99f);
				//	float PrefireProduct = (-PrefireScalarClamped);
				//	TargetLength += PrefireProduct;
				//}

				// Modifier for global time dilation
				/*if (GlobalTimeScale < 0.02f)
				{
					TargetLength *= 0.5f;
					VelocityCameraSpeed *= 15.0f;
				}*/
				if (GlobalTimeScale <= 0.1f)
				{
					float RefinedGScalar = FMath::Clamp(GlobalTimeScale, 0.62f, 1.0f);
					TargetLength *= RefinedGScalar;
				}

				// Clamp useable distance
				float TargetLengthClamped = FMath::Clamp(FMath::Sqrt(TargetLength) * 15.0f * ConsideredDistanceScalar,
					CameraMinimumDistance,
					CameraMaxDistance);

				// Set Camera Distance
				float InverseTimeSpeed = FMath::Clamp((1.0f / GlobalTimeScale), 1.0f, 2.0f);
				float DesiredCameraDistance = FMath::FInterpTo(CameraBoom->TargetArmLength,
					TargetLengthClamped, DeltaTime, (VelocityCameraSpeed * 0.5f) * InverseTimeSpeed);

				// Narrowing and expanding camera FOV for closeup and outer zones
				float ActorsDist = DistBetweenActors;
				float FOVTimeScalar = FMath::Clamp(GlobalTimeScale, 0.5f, 1.0f);
				float FOV = SideViewCameraComponent->FieldOfView;
				float Verticality = FMath::Abs((PositionOne - PositionTwo).Z);
				float FOVSpeed = CameraMoveSpeed * 0.3f;

				// Inne zone
				if ((DistBetweenActors <= 150.0f) || bAlone)
				{
					FOV = 21.5f;
				}
				
				// Outer Zone
				if (((DistBetweenActors > 100.0f) || (Verticality >= 50.0f))
					&& !bAlone)
				{
					float WideAngleFOV = FMath::Clamp((0.033f * DistBetweenActors), 30.0f, 100.0f);
					FOV = WideAngleFOV;
				}
				
				// Timescale adjustment
				if (CustomTimeDilation < 0.5f)
				{
					float TimeScalar = FMath::Clamp(CustomTimeDilation, 0.77f, 0.99f);
					FOV *= TimeScalar;
				}
				
				FOV *= CameraFOVScalar;
				FOV = FMath::Clamp(FOV, 20.0f, 100.0f);

				// Set FOV
				SideViewCameraComponent->FieldOfView = FMath::FInterpTo(
					SideViewCameraComponent->FieldOfView,
					FOV,
					DeltaTime,
					FOVSpeed);

				FVector CamNorm = SideViewCameraComponent->GetForwardVector();
				FVector NormToActor1 = (Actor1->GetActorLocation() - CameraBoom->GetComponentLocation()).GetSafeNormal();
				float AngleToActor1 = FMath::Acos(FVector::DotProduct(CamNorm, NormToActor1));
				if (AngleToActor1 > 5.0f)
				{
					DesiredCameraDistance *= 1.5f;
					GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("AngleToActor1: %f"), AngleToActor1));
				}

				// Make it so
				CameraBoom->SetWorldLocation(Midpoint);
				CameraBoom->TargetArmLength = DesiredCameraDistance;
				//SideViewCameraComponent->OrthoWidth = (DesiredCameraDistance);

				///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("FOV: %f"), SideViewCameraComponent->FieldOfView));
				///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("Dist: %f"), CameraBoom->TargetArmLength));
			}
		}
	}
}

void ATachyonCharacter::ReceiveKnockback(FVector Knockback, bool bOverrideVelocity)
{
	if (Controller != nullptr)
	{
		ServerReceiveKnockback(Knockback, bOverrideVelocity);
	}
}
void ATachyonCharacter::ServerReceiveKnockback_Implementation(FVector Knockback, bool bOverrideVelocity)
{
	MulticastReceiveKnockback(Knockback, bOverrideVelocity);
}
bool ATachyonCharacter::ServerReceiveKnockback_Validate(FVector Knockback, bool bOverrideVelocity)
{
	return true;
}
void ATachyonCharacter::MulticastReceiveKnockback_Implementation(FVector Knockback, bool bOverrideVelocity)
{
	if (Controller != nullptr)
	{
		GetCharacterMovement()->AddImpulse(Knockback, bOverrideVelocity);
		GetCharacterMovement()->Velocity = GetCharacterMovement()->Velocity.GetClampedToMaxSize(MaxMoveSpeed);
		ForceNetUpdate();
	}
}

void ATachyonCharacter::UpdateBody(float DeltaTime)
{
	// Set rotation so character faces direction of travel
	if (Controller != nullptr)
	{
		float TravelDirection = FMath::Clamp(LastFaceDirection, -1.0f, 1.0f);
		float ClimbDirection = FMath::Clamp(InputZ * 5.0f, -5.0f, 5.0f);
		float Roll = FMath::Clamp(InputZ * -25.1f, -25.1f, 25.1f);

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
			if (FMath::Abs(Controller->GetControlRotation().Yaw) > 90.0f)
			{
				BodyRotation = FRotator(ClimbDirection, 180.0f, -Roll);
			}
			else if (FMath::Abs(Controller->GetControlRotation().Yaw) < 90.0f)
			{
				BodyRotation = FRotator(ClimbDirection, 0.0f, Roll);
			}
		}

		Controller->SetControlRotation(BodyRotation);

		// Update aimer
		if (Aimer != nullptr)
		{
			/*FVector Forw = GetActorForwardVector().GetSafeNormal();
			FVector Velo = GetCharacterMovement()->Velocity.GetSafeNormal();
			float MyDot = FVector::DotProduct(Forw, Velo);
			float MySpeed = GetCharacterMovement()->Velocity.Size();
			if ((MySpeed < 10.1f) || (MyDot < -0.01f))
			{
				if (Aimer->IsVisible())
				{
					Aimer->SetVisibility(false);
				}
			}
			else
			{
				if (!Aimer->IsVisible())
				{
					Aimer->SetVisibility(true);
				}
			}*/
		}
		else
		{
			// Locate aimer
			TArray<UActorComponent*> MyParticles = GetComponentsByTag(UParticleSystemComponent::StaticClass(), FName("Aimer"));
			int NumPs = MyParticles.Num();
			if (NumPs > 0)
			{
				for (int i = 0; i < NumPs; ++i)
				{
					UParticleSystemComponent* ThisAimer = Cast<UParticleSystemComponent>(MyParticles[i]);
					if (ThisAimer != nullptr)
					{
						Aimer = ThisAimer;
						Aimer->SetVisibility(false);
					}
				}
			}
		}
	}

	// Sound update
	if (SoundComp != nullptr)
	{
		float Velo = 0.001f + FMath::Sqrt(GetCharacterMovement()->Velocity.Size() * 0.00001f);
		float SpeedVolume = FMath::Clamp(Velo, 0.001f, 1.0f);
		SoundComp->SetVolumeMultiplier(SpeedVolume);
	}
}

void ATachyonCharacter::ServerUpdateBody_Implementation(float DeltaTime)
{
	// Recover personal timescale if down
	float GlobalTimescale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	
	if ((GlobalTimescale == 1.0f)
		&& (CustomTimeDilation < 1.0f))
	{
		float RecoverySpeed = TimescaleRecoverySpeed * (1.0f / CustomTimeDilation);
		RecoverySpeed = FMath::Clamp(RecoverySpeed, 0.001f, 100.0f);
		
		float InterpTime = FMath::FInterpConstantTo(CustomTimeDilation, MaxTimescale, DeltaTime, RecoverySpeed);
		NewTimescale(InterpTime);
	}
}
bool ATachyonCharacter::ServerUpdateBody_Validate(float DeltaTime)
{
	return true;
}


// Receive capped timescale from UI bars
void ATachyonCharacter::SetMaxTimescale(float Value)
{
	if (GetController() != nullptr)
	{
		if ((Value < MaxTimescale) || (Value == 1.0f))
		{
			MaxTimescale = Value;
			///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("NewMaxTimescale: %f"), MaxTimescale));
		}
	}
}

float ATachyonCharacter::GetMaxTimescale()
{
	float Result = 0.0f;
	if (GetController() != nullptr)
	{
		Result = MaxTimescale;
	}
	return Result;
}


////////////////////////////////////////////////////////////////////////
// COSMETICS

//void ATachyonCharacter::DonApparel()
//{
//	if (Controller != nullptr)
//	{
//		APlayerState* PlayerState = Controller->PlayerState;
//		if (PlayerState != nullptr)
//		{
//			ATachyonPlayerState* MyState = Cast<ATachyonPlayerState>(PlayerState);
//			if (MyState != nullptr)
//			{
//				TArray<TSubclassOf<ATApparel>> MyApparels = MyState->Skins;
//				if (MyApparels.Num() > 0)
//				{
//					FActorSpawnParameters SpawnParams;
//					TSubclassOf<ATApparel> Attire = MyApparels[iApparelIndex];
//					if (Attire != nullptr)
//					{
//						ActiveApparel = GetWorld()->SpawnActor<ATApparel>(Attire, SpawnParams);
//
//						if (ActiveApparel != nullptr)
//						{
//							GetMesh()->SetMaterial(0, ActiveApparel->ApparelMaterial);
//
//							GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Green, TEXT("Donned New Apparel"));
//						}
//					}
//				}
//			}
//		}
//	}
//}

//void ATachyonCharacter::SetApparel(int ApparelIndex)
//{
//	iApparelIndex = ApparelIndex;
//	DonApparel();
//	
//	if (Role < ROLE_Authority)
//	{
//		ServerSetApparel(ApparelIndex);
//	}
//}
//void ATachyonCharacter::ServerSetApparel_Implementation(int ApparelIndex)
//{
//	SetApparel(ApparelIndex);
//}
//bool ATachyonCharacter::ServerSetApparel_Validate(int ApparelIndex)
//{
//	return true;
//}


////////////////////////////////////////////////////////////////////////
// GAME FLOW

void ATachyonCharacter::RequestBots()
{
	ATachyonGameStateBase* TachyonGame = Cast<ATachyonGameStateBase>(GetWorld()->GetGameState());
	if (TachyonGame != nullptr)
	{
		FVector IntendedLocation = GetActorForwardVector() * 1000.0f;
		TachyonGame->SpawnBot(IntendedLocation);
	}
}

// Restart Game
void ATachyonCharacter::RestartGame()
{
	ServerRestartGame();
}
void ATachyonCharacter::ServerRestartGame_Implementation()
{
	MulticastRestartGame();
}
bool ATachyonCharacter::ServerRestartGame_Validate()
{
	return true;
}
void ATachyonCharacter::MulticastRestartGame_Implementation()
{
	ATachyonGameStateBase* GState = Cast<ATachyonGameStateBase>(GetWorld()->GetGameState());
	if (GState != nullptr)
	{
		GState->RestartGame();
	}

	ForceNetUpdate();
}


/////////////////////////////////////////////////////////////////////
// BODY COLLISION 
void ATachyonCharacter::OnShieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag("Surface")
		|| OverlappedComponent->ComponentHasTag("Surface"))
	{
		// Raycast for surface hit, since we're not sweeping
		bool HitResult = false;

		// Linecast ingredients
		TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
		TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
		TArray<FHitResult> Hits;
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		// Set up ray position
		FVector Start = GetActorLocation();
		FVector End = OtherActor->GetActorLocation();

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

		// Slam position
		FVector SlamLocation = FVector::ZeroVector;
		FRotator SlamRotation = FRotator::ZeroRotator;
		if (HitResult)
		{
			int NumHits = Hits.Num();
			if (NumHits > 0)
			{
				for (int i = 0; i < NumHits; ++i)
				{
					AActor* HitActor = Hits[i].GetActor();
					if (HitActor->ActorHasTag("Surface"))
					{
						SlamLocation = ((Hits[i].ImpactPoint - GetActorLocation()) * 0.1f) + GetActorLocation();
						SlamRotation = Hits[i].ImpactNormal.Rotation();
						break;

						//GEngine->DrawDebugLine
						DrawDebugLine(GetWorld(), Hits[i].ImpactPoint, Hits[i].ImpactNormal * 100000.0f, FColor::White, true, 0.5f, 0, 5.0f);
					}
				}
			}
		}

		// Spawn it
		UParticleSystemComponent* NewSlam = UGameplayStatics::SpawnEmitterAttached(SurfaceHitEffect, GetRootComponent(), NAME_None, SlamLocation, SlamRotation, EAttachLocation::KeepWorldPosition);
		if (NewSlam != nullptr)
		{
			NewSlam->bAutoDestroy = true;
			NewSlam->ComponentTags.Add("ResetKill");

			float VelSize = FMath::Clamp((GetCharacterMovement()->Velocity.Size() * 0.005f), 1.0f, 10.0f);
			FVector NewScale = NewSlam->GetComponentScale() * VelSize;
			NewSlam->SetWorldScale3D(NewScale);

			///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Impact VelSize: %f"), VelSize));
		}
	}

	if (OtherActor->ActorHasTag("Player"))
	{
		Collide(OtherActor);
	}

	// attack should check for shield to make reflect fx
}

void ATachyonCharacter::Collide(AActor* OtherActor)
{
	// Visual impact
	if (CollideEffect != nullptr)
	{
		FVector SlamLocation = GetActorLocation() + (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FRotator SlamRotation = SlamLocation.Rotation();
		UParticleSystemComponent* NewSlam = UGameplayStatics::SpawnEmitterAttached(CollideEffect, GetRootComponent(), NAME_None, SlamLocation, SlamRotation, EAttachLocation::KeepWorldPosition);
		if (NewSlam != nullptr)
		{
			NewSlam->bAutoDestroy = true;
			NewSlam->ComponentTags.Add("ResetKill");
		}
	}

	// Deliver damage-related event
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->RemoteHit(OtherActor, 1.0f);
	}
}


void ATachyonCharacter::SetWorldRange(float InRange)
{
	if (Controller != nullptr)
	{
		WorldRange = InRange;
	}
}


////////////////////////////////////////////////////////////////////////
// NETWORK REPLICATION

void ATachyonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	///DOREPLIFETIME(ATachyonCharacter, NetData);
	DOREPLIFETIME(ATachyonCharacter, ActiveAttack);
	DOREPLIFETIME(ATachyonCharacter, ActiveWindup);
	DOREPLIFETIME(ATachyonCharacter, ActiveBoost);
	DOREPLIFETIME(ATachyonCharacter, ActiveSecondary);
	DOREPLIFETIME(ATachyonCharacter, ActiveDeath);

	DOREPLIFETIME(ATachyonCharacter, Health);
	DOREPLIFETIME(ATachyonCharacter, MaxHealth);
	DOREPLIFETIME(ATachyonCharacter, MaxTimescale);

	DOREPLIFETIME(ATachyonCharacter, Opponent);
	
	//DOREPLIFETIME(ATachyonCharacter, ActiveApparel);
	//DOREPLIFETIME(ATachyonCharacter, iApparelIndex);
}


// Print to screen
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("sizing"));
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Charge: %f"), Charge));