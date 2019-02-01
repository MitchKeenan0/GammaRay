// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonCharacter.h"
#include "TachyonJump.h"
#include "TachyonGameStateBase.h"
#include "TachyonMovementComponent.h"
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
	//GetCharacterMovement()->BrakingFrictionFactor = 50.0f;

	AttackScene = CreateDefaultSubobject<USceneComponent>(TEXT("AttackScene"));
	AttackScene->SetupAttachment(RootComponent);

	AmbientParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AmbientParticles"));
	AmbientParticles->SetupAttachment(RootComponent);
	AmbientParticles->bAbsoluteRotation = true;

	SoundComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundComp"));
	SoundComp->SetupAttachment(RootComponent);
	SoundComp->VolumeMultiplier = 0.1f;

	ForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("ForceComp"));
	ForceComp->SetupAttachment(RootComponent);
	ForceComp->SetIsReplicated(true);

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

		if (NearDeathEffect != nullptr)
		{
			ActiveDeath = UGameplayStatics::SpawnEmitterAttached(NearDeathEffect, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
			if (ActiveDeath != nullptr)
			{
				ActiveDeath->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				ActiveDeath->Deactivate();
			}
		}
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

	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());

	if (Controller != nullptr)
	{
		UpdateHealth(DeltaTime);
		UpdateCamera(DeltaTime);

		if (GlobalTimeScale == 1.0f)
		{
			// Body rotation
			if (!ActorHasTag("Bot"))
			{
				UpdateBody(DeltaTime);
			}

			// Timescale recovery
			if (HasAuthority())
			{
				if (CustomTimeDilation < 1.0f)
				{
					ServerUpdateBody(DeltaTime);
				}
			}
		}

		FVector ToCentre = FVector::ZeroVector - GetActorLocation();
		if (ToCentre.Size() >= 2000.0f)
		{
			float CurrentV = GetCharacterMovement()->Velocity.Size();
			GetCharacterMovement()->Velocity = ToCentre.GetSafeNormal() * CurrentV;
		}
	}

	// Hacky stuff
	if ((Health <= 0.0f) && !bSpawnedDeath &&
		(ActiveDeath != nullptr)) ///  && 
	{
		if (ActiveDeath != nullptr)
		{
			ActiveDeath->Activate();
			ActiveDeath->CustomTimeDilation = FMath::Clamp(CustomTimeDilation, 0.1f, 1.0f);
			bSpawnedDeath = true;
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
	PlayerInputComponent->BindAction("Shield", IE_Pressed, this, &ATachyonCharacter::Shield);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATachyonCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ATachyonCharacter::EndJump);
	PlayerInputComponent->BindAction("SummonBot", IE_Pressed, this, &ATachyonCharacter::RequestBots);
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
			if ((ActiveBoost != nullptr) && (Value != 0.0f))
			{
				ActiveBoost->StartJump();
			}

			InputX = Value;
		}

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
		AddMovementInput(FVector::UpVector, Value * 0.618f);

		if (InputZ != Value)
		{
			if ((ActiveBoost != nullptr) && (Value != 0.0f))
			{
				ActiveBoost->StartJump();
			}

			InputZ = Value;
		}
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
	if (ActiveBoost != nullptr)
	{
		ActiveBoost->StartJump();
	}
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
	float DilationSpeed = FMath::Clamp((1.0f / CustomTimeDilation), 1.0f, 3.0f); //FMath::Sqrt(CustomTimeDilation);
	float JumpSpeed = BoostSpeed * DilationSpeed;
	float JumpTopSpeed = BoostSustain * DilationSpeed;

	//if (Opponent != nullptr)
	//{
	//	FVector ToOpponent = Opponent->GetActorLocation() - GetActorLocation();
	//	float DistToOpponent = FVector::Dist(GetActorLocation(), Opponent->GetActorLocation());

	//	FVector Norm1 = GetCharacterMovement()->Velocity.GetSafeNormal();
	//	FVector Norm2 = ToOpponent.GetSafeNormal();
	//	float DotToOpponent = -FVector::DotProduct(Norm1, Norm2);
	//	DotToOpponent = FMath::Clamp(DotToOpponent, 0.77f, 1.1f);
	//	//float DotReductiveScalar = FMath::Clamp((1.0f / DotToOpponent), 0.01f, 1.0f);

	//	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, FString::Printf(TEXT("DotToOpponent: %f"), DotToOpponent));

	//	JumpSpeed *= DotToOpponent;
	//	JumpTopSpeed *= DotToOpponent;
	//}

	GetCharacterMovement()->MaxAcceleration = JumpSpeed;
	GetCharacterMovement()->MaxFlySpeed = JumpTopSpeed;
}

void ATachyonCharacter::DisengageJump()
{
	float NewMaxSpeed = MaxMoveSpeed;
	FVector MyVelocity = GetCharacterMovement()->Velocity;
	if (MyVelocity.Size() >= GetCharacterMovement()->MaxFlySpeed)
	{
		NewMaxSpeed = MaxMoveSpeed * 1.62f;
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, FString::Printf(TEXT("NewMaxSpeed: %f"), NewMaxSpeed));
	}
	GetCharacterMovement()->MaxAcceleration = MoveSpeed;
	GetCharacterMovement()->MaxFlySpeed = NewMaxSpeed;
}


////////////////////////////////////////////////////////////////////////
// ATTACKING

void ATachyonCharacter::StartFire()
{
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->StartFire();
	}
}

void ATachyonCharacter::EndFire()
{
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->EndFire();
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

void ATachyonCharacter::ModifyHealth(float Value)
{
	if (Role < ROLE_Authority)
	{
		ServerModifyHealth(Value);
	}

	float SafeHealth = FMath::Clamp(Health, 0.0f, 100.0f);
	MaxHealth = FMath::Clamp(Health + Value, 0.0f, 100.0f);

	///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Ouch MaxHealth %f"), MaxHealth));

	// Clear old death if we're reviving
	if (Value == 100.0f)
	{
		bSpawnedDeath = false;
	}
}
void ATachyonCharacter::ServerModifyHealth_Implementation(float Value)
{
	if (GetOwner() != nullptr)
	{
		ModifyHealth(Value);
	}
}
bool ATachyonCharacter::ServerModifyHealth_Validate(float Value)
{
	return true;
}

void ATachyonCharacter::NewTimescale(float Value)
{
	if (Controller != nullptr)
	{
		ServerNewTimescale(Value);
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
			float BestDistance = 0.0f;
			
			for (int i = 0; i < LoopCount; ++i)
			{
				CurrentActor = FramingActors[i];
				if (CurrentActor != nullptr
					&& CurrentActor != Actor1
					&& !CurrentActor->ActorHasTag("Spectator")
					&& !CurrentActor->ActorHasTag("Obstacle"))
				{
					float DistToTemp = FVector::Dist(CurrentActor->GetActorLocation(), GetActorLocation());
					if (DistToTemp < DistToActor2)
					{
						/// Players get veto importance
						if ((BestCandidate == nullptr)
						|| (!BestCandidate->ActorHasTag("Player")))
						{
							BestCandidate = CurrentActor;
							DistToActor2 = DistToTemp;
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
			float CameraMinimumDistance = (3500.0f * CameraDistanceScalar) + (PersonalScalar * CameraDistanceScalar); // (1100.0f + PersonalScalar)
			float CameraMaxDistance = 11551000.0f;

			// If Actor2 is valid, make Pair Framing
			bool bAlone = true;
			if (Actor2 != nullptr)
			{

				// Distance check i.e pair bounds
				float PairDistanceThreshold = 5000.0f + FMath::Clamp(Actor1->GetVelocity().Size() * 2.5f, 5000.0f, 15000.0f); /// formerly 3000.0f
				if (this->ActorHasTag("Spectator"))
				{
					PairDistanceThreshold *= 3.3f;
				}
				if (!Actor2->ActorHasTag("Player"))
				{
					PairDistanceThreshold *= 0.5f;
				}

				// Special care taken for vertical as we are probably widescreen
				float Vertical = FMath::Abs(( Actor2->GetActorLocation() - Actor1->GetActorLocation() ).Z);
				bool bInRange = (FVector::Dist(Actor1->GetActorLocation(), Actor2->GetActorLocation()) <= PairDistanceThreshold)
					&& (Vertical <= (PairDistanceThreshold * 0.9f));
				bool TargetVisible = Actor2->WasRecentlyRendered(0.2f);

				if (bInRange && TargetVisible)
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
			if (bAlone || (FramingActors.Num() == 1))
			{
				// Framing lone player by their velocity
				Actor1Velocity = Actor1->GetVelocity() + GetActorForwardVector();

				// Declare Position Two
				FVector VelocityFraming = Actor1->GetActorLocation() + (Actor1Velocity * DeltaTime * CameraSoloVelocityChase);
				PositionTwo = FMath::VInterpTo(PositionTwo, VelocityFraming, DeltaTime, VelocityCameraSpeed * 0.5f);

				Actor2 = nullptr;
			}


			// Positions done
			// Find the midpoint
			float MidpointBias = 0.5f;
			FVector TargetMidpoint = PositionOne + ((PositionTwo - PositionOne) * MidpointBias);
			float MidpointInterpSpeed = 10.0f * FMath::Clamp(TargetMidpoint.Size() * 0.01f, 10.0f, 100.0f) * (1.0f / CustomTimeDilation);
			if (bAlone)
				MidpointInterpSpeed *= 0.5f;

			Midpoint = FMath::VInterpTo(Midpoint, TargetMidpoint, DeltaTime, MidpointInterpSpeed); /// TargetMidpoint; /// 
			if (Midpoint.Size() > 0.0f)
			{

				// Distance
				float DistBetweenActors = FVector::Dist(PositionOne, PositionTwo);
				float ProcessedDist = (FMath::Sqrt(DistBetweenActors) * 1500.0f);
				float VerticalDist = FMath::Abs((PositionTwo - PositionOne).Z);
				// If paired, widescreen edges are vulnerable to overshoot
				if (!bAlone)
				{
					VerticalDist *= 2.1f;
				}
				else
				{
					ProcessedDist *= 1.5f;
					CameraMinimumDistance *= 1.5f;
				}

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

				// Last modifier for global time dilation
				float RefinedGScalar = FMath::Clamp(GlobalTimeScale, 0.5f, 1.0f);
				if (GlobalTimeScale < 0.02f)
				{
					TargetLength *= 0.5f;
					VelocityCameraSpeed *= 15.0f;
				}
				if (GlobalTimeScale <= 0.1f)
				{
					TargetLength *= RefinedGScalar;
				}

				// Clamp useable distance
				float TargetLengthClamped = FMath::Clamp(FMath::Sqrt(TargetLength * 215.0f) * ConsideredDistanceScalar,
					CameraMinimumDistance,
					CameraMaxDistance);

				// Set Camera Distance
				float InverseTimeSpeed = FMath::Clamp((1.0f / GlobalTimeScale), 1.0f, 2.0f);
				float DesiredCameraDistance = FMath::FInterpTo(CameraBoom->TargetArmLength,
					TargetLengthClamped, DeltaTime, (VelocityCameraSpeed * 0.5f) * InverseTimeSpeed);

				// Narrowing and expanding camera FOV for closeup and outer zones
				float ScalarSize = FMath::Clamp(DistBetweenActors * 0.005f, 0.05f, 52.5f);
				float FOVTimeScalar = FMath::Clamp(GlobalTimeScale, 0.5f, 1.0f);
				float FOV = 19.9f;
				float FOVSpeed = 1.0f;
				float Verticality = FMath::Abs((PositionOne - PositionTwo).Z);

				// Inner and Outer zones
				if ((DistBetweenActors <= 150.0f) && !bAlone)
				{
					FOV = 21.5f;
				}
				
				if (((DistBetweenActors > 150.0f) || (Verticality >= 100.0f))
					&& !bAlone)
				{
					float WideAngleFOV = FMath::Clamp((0.03f * DistBetweenActors), 30.0f, 120.0f);
					FOV = WideAngleFOV;
				}
				
				// GGTime Timescale adjustment
				if (GlobalTimeScale < 1.0f)
				{
					FOV -= 5.f;
				}
				
				FOV *= CameraFOVScalar;
				FOV = FMath::Clamp(FOV, 19.0f, 120.0f);

				// Set FOV
				SideViewCameraComponent->FieldOfView = FMath::FInterpTo(
					SideViewCameraComponent->FieldOfView,
					FOV,
					DeltaTime,
					FOVSpeed * ScalarSize);

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
		float RotatoeSpeed = FMath::Clamp((15000.0f * CustomTimeDilation), 1500.0f, 15000.0f);

		if (TravelDirection < 0.0f)
		{
			FRotator Fint = FMath::RInterpConstantTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 180.0f, Roll), DeltaTime, RotatoeSpeed);
			Controller->SetControlRotation(Fint);
		}
		else if (TravelDirection > 0.0f)
		{
			FRotator Fint = FMath::RInterpConstantTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 0.0f, -Roll), DeltaTime, RotatoeSpeed);
			Controller->SetControlRotation(Fint);
		}

		// No lateral Input - finish rotation
		else
		{
			if (FMath::Abs(Controller->GetControlRotation().Yaw) > 90.0f)
			{
				FRotator Fint = FMath::RInterpConstantTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 180.0f, -Roll), DeltaTime, RotatoeSpeed);
				Controller->SetControlRotation(Fint);
			}
			else if (FMath::Abs(Controller->GetControlRotation().Yaw) < 90.0f)
			{
				FRotator Fint = FMath::RInterpConstantTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 0.0f, Roll), DeltaTime, RotatoeSpeed);
				Controller->SetControlRotation(Fint);
			}
		}
	}

	// Sound control
	if (SoundComp != nullptr)
	{
		float Velo = FMath::Sqrt(GetCharacterMovement()->Velocity.Size() * 0.000001f);
		float SpeedVolume = FMath::Clamp(Velo, 0.01f, 1.0f);
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
		float RecoverySpeed = 0.1f + (FMath::Sqrt(CustomTimeDilation) * TimescaleRecoverySpeed);
		RecoverySpeed = FMath::Clamp(RecoverySpeed, 0.1f, 1.0f);
		///GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, FString::Printf(TEXT("RecoverySpeed: %f"), RecoverySpeed));
		
		float InterpTime = FMath::FInterpTo(CustomTimeDilation, 1.0f, DeltaTime, RecoverySpeed);
		NewTimescale(InterpTime);
	}
}
bool ATachyonCharacter::ServerUpdateBody_Validate(float DeltaTime)
{
	float GlobalTimescale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	if (GlobalTimescale == 1.0f)
	{
		return true;
	}
	else
	{
		return false;
	}
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
	if (OtherActor->ActorHasTag("Player"))
	{
		Collide(OtherActor);
	}

	// Visual impact
	if (CollideEffect != nullptr)
	{
		FVector SlamLocation = SweepResult.ImpactPoint;
		FRotator SlamRotation = SweepResult.ImpactNormal.Rotation();
		UParticleSystemComponent* NewSlam = UGameplayStatics::SpawnEmitterAttached(CollideEffect, GetRootComponent(), NAME_None, SlamLocation, SlamRotation, EAttachLocation::KeepWorldPosition);
		if (NewSlam != nullptr)
		{
			NewSlam->bAutoDestroy = true;
			NewSlam->ComponentTags.Add("ResetKill");
		}
	}

	// attack should check for shield to make reflect fx
}

void ATachyonCharacter::Collide(AActor* OtherActor)
{
	if (ActiveAttack != nullptr)
	{
		ActiveAttack->RemoteHit(OtherActor, 1.0f);
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

	DOREPLIFETIME(ATachyonCharacter, Opponent);
	
	//DOREPLIFETIME(ATachyonCharacter, ActiveApparel);
	//DOREPLIFETIME(ATachyonCharacter, iApparelIndex);
}


// Print to screen
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("sizing"));
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Charge: %f"), Charge));