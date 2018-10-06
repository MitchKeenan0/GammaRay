// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonCharacter.h"


// Sets default values
ATachyonCharacter::ATachyonCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Configure character movement
	///GetCapsuleComponent()->SetIsReplicated(true);
	GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
	GetCapsuleComponent()->SetCapsuleRadius(34.0f);

	///GetMesh()->SetIsReplicated(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->AirControl = 1.0f;
	GetCharacterMovement()->MaxFlySpeed = 1000.0f;
	GetCharacterMovement()->MovementMode = MOVE_Flying;
	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));
	GetCharacterMovement()->SetNetAddressable();
	GetCharacterMovement()->SetIsReplicated(true);
	GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 500.0f;
	GetCharacterMovement()->NetworkMinTimeBetweenClientAckGoodMoves = 0.0f;
	GetCharacterMovement()->NetworkMinTimeBetweenClientAdjustments = 0.0f;
	GetCharacterMovement()->NetworkMinTimeBetweenClientAdjustmentsLargeCorrection = 0.0f;

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
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = true;

	AttackScene = CreateDefaultSubobject<USceneComponent>(TEXT("AttackScene"));
	AttackScene->SetupAttachment(RootComponent);
	
	bReplicates = true;
	bReplicateMovement = true;
}


////////////////////////////////////////////////////////////////////////
// BEGIN PLAY
void ATachyonCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Helps with networked movement
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("p.NetEnableMoveCombining 0"));
	
	Health = MaxHealth;
	Tags.Add("Player");
	Tags.Add("FramingActor");

	///DonApparel();
}


////////////////////////////////////////////////////////////////////////
// TICK
void ATachyonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// Local stuff
	if (Controller != nullptr)
	{
		UpdateHealth(DeltaTime);
		UpdateBody(DeltaTime);
		UpdateCamera(DeltaTime);
	}

	if (HasAuthority())
	{
		if (bJumping)
		{
			UpdateJump(DeltaTime);
		}

		if (bShooting)
		{
			WindupAttack(DeltaTime);
		}
	}
}


////////////////////////////////////////////////////////////////////////
// INPUT BINDING
void ATachyonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Actions
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ATachyonCharacter::ArmAttack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ATachyonCharacter::ReleaseAttack);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATachyonCharacter::EngageJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ATachyonCharacter::DisengageJump);

	// Axes
	PlayerInputComponent->BindAxis("MoveRight", this, &ATachyonCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ATachyonCharacter::MoveUp);
}


////////////////////////////////////////////////////////////////////////
// MOVEMENT
void ATachyonCharacter::MoveRight(float Value)
{
	SetX(Value);
	
	float MoveByDot = 0.0f;
	FVector MoveInput = FVector(InputX, 0.0f, InputZ).GetSafeNormal();
	FVector CurrentV = GetMovementComponent()->Velocity;
	FVector VNorm = CurrentV.GetSafeNormal();

	// Move by dot product for skating effect
	if ((MoveInput != FVector::ZeroVector)
		&& (Controller != nullptr))
	{

		float DotToInput = (FVector::DotProduct(MoveInput, VNorm));
		float AngleToInput = FMath::Acos(DotToInput);
		AngleToInput = FMath::Clamp(AngleToInput, 1.0f, 1000.0f);

		// Effect Move
		float TurnScalar = MoveSpeed + FMath::Square(TurnSpeed * AngleToInput);
		float DeltaTime = GetWorld()->DeltaTimeSeconds;
		MoveByDot = MoveSpeed * TurnScalar;
		AddMovementInput(FVector(1.0f, 0.0f, 0.0f), InputX * DeltaTime * MoveSpeed * MoveByDot);
	}
}

void ATachyonCharacter::MoveUp(float Value)
{
	SetZ(Value);

	float MoveByDot = 0.0f;
	FVector MoveInput = FVector(InputX, 0.0f, InputZ).GetSafeNormal();
	FVector CurrentV = GetMovementComponent()->Velocity;
	FVector VNorm = CurrentV.GetSafeNormal();

	// Move by dot product for skating effect
	if ((MoveInput != FVector::ZeroVector)
		&& (Controller != nullptr))
	{
		float DotToInput = (FVector::DotProduct(MoveInput, VNorm));
		float AngleToInput = FMath::Acos(DotToInput);
		AngleToInput = FMath::Clamp(AngleToInput, 1.0f, 1000.0f);

		// Effect Move
		float TurnScalar = MoveSpeed + FMath::Square(TurnSpeed * AngleToInput);
		float DeltaTime = GetWorld()->DeltaTimeSeconds;
		MoveByDot = MoveSpeed * TurnScalar;
		AddMovementInput(FVector(0.0f, 0.0f, 1.0f), InputZ * DeltaTime * MoveSpeed * MoveByDot);
	}
}

////////////////////////////////////////////////////////////////////////
// JUMP
void ATachyonCharacter::EngageJump()
{
	if (Role < ROLE_Authority)
	{
		ServerEngageJump();
	}
	else if (!bJumping)
	{
		bJumping = true;
	}
}
void ATachyonCharacter::ServerEngageJump_Implementation()
{
	EngageJump();
}
bool ATachyonCharacter::ServerEngageJump_Validate()
{
	return true;
}

void ATachyonCharacter::DisengageJump()
{
	if (Role < ROLE_Authority)
	{
		ServerDisengageJump();
	}
	else if (bJumping)
	{
		bJumping = false;
		DiminishingJumpValue = 0.0f;
		BoostTimeAlive = 0.0f;

		if (ActiveBoost != nullptr)
		{
			ActiveBoost->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			ActiveBoost->SetActorLocation(GetActorLocation());
			ActiveBoost->ForceNetUpdate();
			ActiveBoost->SetLifeSpan(0.5f);
			ActiveBoost = nullptr;
		}
	}
}
void ATachyonCharacter::ServerDisengageJump_Implementation()
{
	DisengageJump();
}
bool ATachyonCharacter::ServerDisengageJump_Validate()
{
	return true;
}

void ATachyonCharacter::UpdateJump(float DeltaTime)
{
	if (Role < ROLE_Authority)
	{
		ServerUpdateJump(DeltaTime);
	}
	else
	{
		FVector MoveInputVector = FVector(InputX, 0.0f, InputZ).GetSafeNormal();
		
		// First-timer does initial kick and visuals
		if ((ActiveBoost == nullptr) && (BoostClass != nullptr))
		{
			if (GetCharacterMovement() != nullptr)
			{
				GetCharacterMovement()->AddImpulse(MoveInputVector * 1000.0f, true);
			}

			if (HasAuthority())
			{
				FActorSpawnParameters SpawnParams;
				FRotator InputRotation = MoveInputVector.Rotation();
				FVector SpawnLocation = GetActorLocation() + (FVector::UpVector * 10.0f);
				
				ActiveBoost = GetWorld()->SpawnActor<AActor>(BoostClass, SpawnLocation, InputRotation, SpawnParams);
				if (ActiveBoost != nullptr)
				{
					ActiveBoost->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
		}
		else
		{
			// Diminishing propulsion
			if (ActiveBoost != nullptr)
			{
				BoostTimeAlive = ActiveBoost->GetGameTimeSinceCreation();
				if (BoostTimeAlive > 0.05f)
				{
					DiminishingJumpValue = FMath::Clamp((1.0f / BoostTimeAlive), 0.1f, 1000.0f);
					FVector JumpVector = MoveInputVector * BoostSpeed * DiminishingJumpValue * DeltaTime;

					if (GetCharacterMovement() != nullptr)
					{
						GetCharacterMovement()->AddImpulse(JumpVector, true);
					}
				}
			}
		}
	}
}
void ATachyonCharacter::ServerUpdateJump_Implementation(float DeltaTime)
{
	UpdateJump(DeltaTime);
}
bool ATachyonCharacter::ServerUpdateJump_Validate(float DeltaTime)
{
	return true;
}

////////////////////////////////////////////////////////////////////////
// NET AIM
void ATachyonCharacter::SetX(float Value)
{
	InputX = Value; /// FMath::FInterpConstantTo(InputX, Value, GetWorld()->DeltaTimeSeconds, 100.0f);
	
	if (Role < ROLE_Authority)
	{
		ServerSetX(Value);
	}
}
void ATachyonCharacter::ServerSetX_Implementation(float Value)
{
	SetX(Value);
}
bool ATachyonCharacter::ServerSetX_Validate(float Value)
{
	return true;
}

void ATachyonCharacter::SetZ(float Value)
{
	InputZ = Value; /// FMath::FInterpConstantTo(InputZ, Value, GetWorld()->DeltaTimeSeconds, 100.0f);

	if (Role < ROLE_Authority)
	{
		ServerSetZ(Value);
	}
}
void ATachyonCharacter::ServerSetZ_Implementation(float Value)
{
	SetZ(Value);
}
bool ATachyonCharacter::ServerSetZ_Validate(float Value)
{
	return true;
}


////////////////////////////////////////////////////////////////////////
// ATTACKING
void ATachyonCharacter::ArmAttack()
{
	bShooting = true;

	if (Role < ROLE_Authority)
	{
		ServerArmAttack();
	}
}
void ATachyonCharacter::ServerArmAttack_Implementation()
{
	ArmAttack();
}
bool ATachyonCharacter::ServerArmAttack_Validate()
{
	return true;
}


void ATachyonCharacter::ReleaseAttack()
{
	bShooting = false;

	if ((WindupTimer > 0.0f)
		&& HasAuthority())
	{
		FireAttack();
		WindupTimer = 0.0f;
		ActiveWindup->Destroy();
		ActiveWindup = nullptr;
	}
	
	if (Role < ROLE_Authority)
	{
		ServerReleaseAttack();
	}
}
void ATachyonCharacter::ServerReleaseAttack_Implementation()
{
	ReleaseAttack();
}
bool ATachyonCharacter::ServerReleaseAttack_Validate()
{
	return true;
}


void ATachyonCharacter::WindupAttack(float DeltaTime)
{
	if (HasAuthority())
	{
		if (ActiveWindup == nullptr)
		{
			FVector FirePosition = GetActorLocation(); ///AttackScene->GetComponentLocation();
			FVector LocalForward = GetActorForwardVector(); /// AttackScene->GetForwardVector();
			LocalForward.Y = 0.0f;
			FRotator FireRotation = LocalForward.GetSafeNormal().Rotation();
			FActorSpawnParameters SpawnParams;
			ActiveWindup = GetWorld()->SpawnActor<AActor>(AttackWindupClass, FirePosition, FireRotation, SpawnParams);
			if (ActiveWindup != nullptr)
			{
				ActiveWindup->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			}
		}

		WindupTimer += DeltaTime;
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("WindupTimer: %f"), WindupTimer));

		/*if (WindupTimer >= WindupTime)
		{
		FireAttack();
		ReleaseAttack();
		WindupTimer = 0.0f;
		}*/
	}

	if (Role < ROLE_Authority)
	{
		ServerWindupAttack(DeltaTime);
	}
}
void ATachyonCharacter::ServerWindupAttack_Implementation(float DeltaTime)
{
	WindupAttack(DeltaTime);
}
bool ATachyonCharacter::ServerWindupAttack_Validate(float DeltaTime)
{
	return true;
}


void ATachyonCharacter::FireAttack()
{
	if (Role < ROLE_Authority)
	{
		ServerFireAttack();
	}
	else
	{
		if (HasAuthority() && (AttackTimer <= 0.0f))
		{
			if ((AttackClass != nullptr))/// || bMultipleAttacks)
			{
				// Spawning
				FVector FirePosition = AttackScene->GetComponentLocation();
				FVector LocalForward = AttackScene->GetForwardVector();
				LocalForward.Y = 0.0f;
				FRotator FireRotation = LocalForward.GetSafeNormal().Rotation();
				FireRotation += FRotator((AttackAngle * InputZ), 0.0f, 0.0f);
				FActorSpawnParameters SpawnParams;
				
				ActiveAttack = Cast<ATachyonAttack>(GetWorld()->SpawnActor<ATachyonAttack>(AttackClass, FirePosition, FireRotation, SpawnParams));
				if (ActiveAttack != nullptr)
				{
					// The attack is born
					if (ActiveAttack != nullptr)
					{
						float AttackStrength = FMath::Clamp(WindupTimer, 0.1f, 1.0f);
						ActiveAttack->InitAttack(this, AttackStrength, InputZ); /// PrefireVal, AimClampedInputZ);
						
						// Position lock, or naw
						if (ActiveAttack->IsLockedEmitPoint())
						{
							ActiveAttack->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
						}

						// Recoil
						FVector RecoilVector = FireRotation.Vector().GetSafeNormal();
						GetCharacterMovement()->AddImpulse(
							RecoilVector
							* -AttackRecoil
							* FMath::Square(1.0f + AttackStrength) 
							* 30.0f);
					}

					

					// Clean up previous flash
					/*if ((GetActiveFlash() != nullptr))
					{
						ActiveFlash->Destroy();
						ActiveFlash = nullptr;
					}*/

					// Slowing the character on fire
					/*if (GetCharacterMovement() != nullptr)
					{
						GetCharacterMovement()->Velocity *= SlowmoMoveBoost;
					}*/

					// Break handbrake
					//CheckPowerSlideOff();
				}
			}
			//else
			//{
			//	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Blue, TEXT("No attack class to spawn"));
			//}

			//// Spend it!
			//float ChargeSpend = 1.0f;
			///// More magnitude -> higher cost
			///*if (PrefireTimer >= 0.33f)
			//{
			//float BigSpend = FMath::FloorToFloat(ChargeMax * PrefireTimer);
			//ChargeSpend = BigSpend;
			//}*/
			//Charge -= ChargeSpend;


			//// Sprite Scaling
			//float ClampedCharge = FMath::Clamp(Charge * 0.7f, 1.0f, ChargeMax);
			//float SCharge = FMath::Sqrt(ClampedCharge);
			//FVector ChargeSize = FVector(SCharge, SCharge, SCharge);
			//GetCapsuleComponent()->SetWorldScale3D(ChargeSize);
		}
	}
}
void ATachyonCharacter::ServerFireAttack_Implementation()
{
	FireAttack();
}
bool ATachyonCharacter::ServerFireAttack_Validate()
{
	return true;
}


void ATachyonCharacter::UpdateAttack(float DeltaTime)
{
	if (Role < ROLE_Authority)
	{
		ServerUpdateAttack(DeltaTime);
	}
	else if (ActiveAttack != nullptr)
	{

	}
}
void ATachyonCharacter::ServerUpdateAttack_Implementation(float DeltaTime)
{
	UpdateAttack(DeltaTime);
}
bool ATachyonCharacter::ServerUpdateAttack_Validate(float DeltaTime)
{
	return true;
}


////////////////////////////////////////////////////////////////////////
// HEALTH
void ATachyonCharacter::ModifyHealth(float Value)
{
	if (Value >= 100.0f)
	{
		Health = 100.0f;
		MaxHealth = 100.0f;
	}
	else
	{
		MaxHealth = FMath::Clamp(Health + Value, -1.0f, 100.0f);
	}

	if (Role < ROLE_Authority)
	{
		ServerModifyHealth(Value);
	}
}
void ATachyonCharacter::ServerModifyHealth_Implementation(float Value)
{
	ModifyHealth(Value);
}
bool ATachyonCharacter::ServerModifyHealth_Validate(float Value)
{
	return true;
}


////////////////////////////////////////////////////////////////////////
// CHARACTER UPDATES
void ATachyonCharacter::UpdateHealth(float DeltaTime)
{
	///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Health: %f"), Health));

	// Update smooth health value
	if (MaxHealth < Health)
	{
		float InterpSpeed = FMath::Abs(Health - MaxHealth) * 5.1f;
		Health = FMath::FInterpConstantTo(Health, MaxHealth, DeltaTime, InterpSpeed);
		///GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("InterpSpeed: %f"), InterpSpeed));

		//// Player killed fx
		//if ((KilledFX != nullptr)
		//	&& (ActiveKilledFX == nullptr)
		//	&& (Health <= 5.0f))
		//{
		//	FActorSpawnParameters SpawnParams;
		//	ActiveKilledFX = GetWorld()->SpawnActor<AActor>(KilledFX, GetActorLocation(), GetActorRotation(), SpawnParams);
		//	if (ActiveKilledFX != nullptr)
		//	{
		//		ActiveKilledFX->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		//	}

		//	// Particle coloring (for later)
		//	/*if (ActiveKilledFX != nullptr)
		//	{
		//	TArray<UParticleSystemComponent*> ParticleComps;
		//	ActiveKilledFX->GetComponents<UParticleSystemComponent>(ParticleComps);
		//	float NumParticles = ParticleComps.Num();
		//	if (NumParticles > 0)
		//	{
		//	UParticleSystemComponent* Parti = ParticleComps[0];
		//	if (Parti != nullptr)
		//	{
		//	FLinearColor PlayerColor = GetCharacterColor().ReinterpretAsLinear();
		//	Parti->SetColorParameter(FName("InitialColor"), PlayerColor);
		//	}
		//	}
		//	}*/
		//}
	}
}

void ATachyonCharacter::UpdateBody(float DeltaTime)
{
	// Set rotation so character faces direction of travel
	float TravelDirection = FMath::Clamp(InputX, -1.0f, 1.0f);
	float ClimbDirection = FMath::Clamp(InputZ, -1.0f, 1.0f) * 5.0f;
	float Roll = FMath::Clamp(InputZ, -1.0f, 1.0f) * 15.0f;
	float RotatoeSpeed = 15.0f;

	if (TravelDirection < 0.0f)
	{
		FRotator Fint = FMath::RInterpTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 180.0f, Roll), DeltaTime, RotatoeSpeed);
		Controller->SetControlRotation(Fint);
	}
	else if (TravelDirection > 0.0f)
	{
		FRotator Fint = FMath::RInterpTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 0.0f, -Roll), DeltaTime, RotatoeSpeed);
		Controller->SetControlRotation(Fint);
	}

	// No lateral Input - finish rotation
	else
	{
		if (FMath::Abs(Controller->GetControlRotation().Yaw) > 90.0f)
		{
			FRotator Fint = FMath::RInterpTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 180.0f, -Roll), DeltaTime, RotatoeSpeed);
			Controller->SetControlRotation(Fint);
		}
		else if (FMath::Abs(Controller->GetControlRotation().Yaw) < 90.0f)
		{
			FRotator Fint = FMath::RInterpTo(Controller->GetControlRotation(), FRotator(ClimbDirection, 0.0f, Roll), DeltaTime, RotatoeSpeed);
			Controller->SetControlRotation(Fint);
		}
	}

	// Body stretch-and-squash
	float Lateral = 1.0f + FMath::Abs(InputX * 0.1f);
	float Vertical = 1.0f + (InputZ * 0.1f);
	FVector BodyVector = FVector(Lateral, 1.1f, Vertical) * 2.2f; // Hard-Code!!
	GetMesh()->SetWorldScale3D(BodyVector);

	// Camera Move-Sway
	//bool bMoving = (InputX != InputZ);
	//if (bMoving && (CameraMoveShake != nullptr))
	//{
	//	UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->PlayCameraShake(CameraMoveShake, 1.0f, ECameraAnimPlaySpace::CameraLocal, FRotator::ZeroRotator);
	//	//UGameplayStatics::PlayWorldCameraShake(GetWorld(), CameraMoveShake, GetActorLocation(), 0.0f, 5555.0f, 1.0f, false);
	//}

	if (Role < ROLE_Authority)
	{
		ServerUpdateBody(DeltaTime);
	}

	// Locator scaling
	/*if (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) > 0.01f)
	{
		LocatorScaling();
	}*/
}
void ATachyonCharacter::ServerUpdateBody_Implementation(float DeltaTime)
{
	UpdateBody(DeltaTime);
}
bool ATachyonCharacter::ServerUpdateBody_Validate(float DeltaTime)
{
	return true;
}

void ATachyonCharacter::UpdateCamera(float DeltaTime)
{
	// Start by checking valid actor
	AActor* Actor1 = nullptr;
	AActor* Actor2 = nullptr;

	float GlobalTimeScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());

	// Poll for framing actors
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("FramingActor"), FramingActors);
	///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("FramingActors Num:  %i"), FramingActors.Num()));
	/// TODO - add defense on running get?

	// Spectator ez life
	if (!this->ActorHasTag("Spectator"))
	{
		Actor1 = this;
	}

	if ((Controller != nullptr) && (FramingActors.Num() >= 1))
	{
		// Edge case: player is spectator, find a sub
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

		// Let's go
		if ((Actor1 != nullptr)) ///  && IsValid(Actor1) && !Actor1->IsUnreachable()
		{

			float VelocityCameraSpeed = CameraMoveSpeed;
			float CameraMaxSpeed = 10000.0f;
			float ConsideredDistanceScalar = CameraDistanceScalar;

			// Position by another actor
			bool bAlone = true;
			// Find closest best candidate for Actor 2
			if (Actor2 == nullptr)
			{

				// Find Actor2 by nominating best actor
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

							// Players get veto importance
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
			}



			// Framing up first actor with their own velocity
			FVector Actor1Velocity = Actor1->GetVelocity();
			float SafeVelocitySize = FMath::Clamp(Actor1Velocity.Size() * 0.005f, 0.01f, 10.0f);
			VelocityCameraSpeed = CameraMoveSpeed * SafeVelocitySize * FMath::Sqrt(1.0f / GlobalTimeScale);
			VelocityCameraSpeed = FMath::Clamp(VelocityCameraSpeed, 1.0f, CameraMaxSpeed * 0.1f);

			FVector LocalPos = Actor1->GetActorLocation() + (Actor1Velocity * DeltaTime * CameraVelocityChase);
			PositionOne = FMath::VInterpTo(PositionOne, LocalPos, DeltaTime, VelocityCameraSpeed);

			// Setting up distance and speed dynamics
			float ChargeScalar = FMath::Clamp((FMath::Sqrt(Charge - 0.9f)), 1.0f, ChargeMax);
			float SpeedScalar = 1.0f + FMath::Sqrt(Actor1Velocity.Size() + 0.01f) * 0.1f;
			float PersonalScalar = 1.0f + (36.0f * ChargeScalar * SpeedScalar) * (FMath::Sqrt(SafeVelocitySize));
			float CameraMinimumDistance = 2500.0f + (PersonalScalar * CameraDistanceScalar); // (1100.0f + PersonalScalar)
			float CameraMaxDistance = 11551000.0f;


			// If Actor2 is valid, make Pair Framing
			if (Actor2 != nullptr)
			{

				// Distance check i.e pair bounds
				float PairDistanceThreshold = FMath::Clamp(Actor1->GetVelocity().Size(), 3000.0f, 15000.0f); /// formerly 3000.0f
				if (this->ActorHasTag("Spectator"))
				{
					PairDistanceThreshold *= 3.3f;
				}
				if (!Actor2->ActorHasTag("Player"))
				{
					PairDistanceThreshold *= 0.5f;
				}

				// Special care taken for vertical as we are probably widescreen
				float Vertical = FMath::Abs((Actor2->GetActorLocation() - Actor1->GetActorLocation()).Z);
				bool bInRange = (FVector::Dist(Actor1->GetActorLocation(), Actor2->GetActorLocation()) <= PairDistanceThreshold)
					&& (Vertical <= (PairDistanceThreshold * 0.55f));
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
				Actor1Velocity = Actor1->GetVelocity();

				// Declare Position Two
				FVector VelocityFraming = Actor1->GetActorLocation() + (Actor1Velocity * DeltaTime * CameraSoloVelocityChase);
				PositionTwo = FMath::VInterpTo(PositionTwo, VelocityFraming, DeltaTime, VelocityCameraSpeed * 0.5f);

				Actor2 = nullptr;
			}


			// Positions done
			// Find the midpoint
			float MidpointBias = 0.5f;
			/*if ((Actor2 != nullptr) && !Actor2->ActorHasTag("Player")) {
			MidpointBias = 0.2f; /// super jerky
			}*/
			FVector TargetMidpoint = PositionOne + ((PositionTwo - PositionOne) * MidpointBias);
			float MidpointInterpSpeed = 10.0f * FMath::Clamp(TargetMidpoint.Size() * 0.01f, 10.0f, 100.0f);

			Midpoint = TargetMidpoint; /// FMath::VInterpTo(Midpoint, TargetMidpoint, DeltaTime, MidpointInterpSpeed);
			if (Midpoint.Size() > 0.0f)
			{

				// Distance
				float DistBetweenActors = FVector::Dist(PositionOne, PositionTwo);
				float ProcessedDist = (FMath::Sqrt(DistBetweenActors) * 1500.0f);
				float VerticalDist = FMath::Abs((PositionTwo - PositionOne).Z);
				// If paired, widescreen edges are vulnerable to overshoot
				if (!bAlone)
				{
					VerticalDist *= 1.5f;
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
				float ScalarSize = FMath::Clamp(DistBetweenActors * 0.005f, 0.05f, 1.0f);
				float FOVTimeScalar = FMath::Clamp(GlobalTimeScale, 0.1f, 1.0f);
				float FOV = 23.0f;
				float FOVSpeed = 1.0f;
				float Verticality = FMath::Abs((PositionOne - PositionTwo).Z);

				// Inner and Outer zones
				if ((DistBetweenActors <= 90.0f) && !bAlone)
				{
					FOV = 19.0f;
				}
				else if (((DistBetweenActors >= 555.0f) || (Verticality >= 250.0f))
					&& !bAlone)
				{
					float WideAngleFOV = FMath::Clamp((0.025f * DistBetweenActors), 42.0f, 71.0f);
					FOV = WideAngleFOV; // 40
				}
				// GGTime Timescale adjustment
				if (GlobalTimeScale < 0.02f)
				{
					FOV *= FOVTimeScalar;
					FOVSpeed *= 0.5f;
				}

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


////////////////////////////////////////////////////////////////////////
// COSMETICS
void ATachyonCharacter::DonApparel()
{
	if (Controller != nullptr)
	{
		APlayerState* PlayerState = Controller->PlayerState;
		if (PlayerState != nullptr)
		{
			ATachyonPlayerState* MyState = Cast<ATachyonPlayerState>(PlayerState);
			if (MyState != nullptr)
			{
				TArray<TSubclassOf<ATApparel>> MyApparels = MyState->Skins;
				if (MyApparels.Num() > 0)
				{
					FActorSpawnParameters SpawnParams;
					TSubclassOf<ATApparel> Attire = MyApparels[iApparelIndex];
					if (Attire != nullptr)
					{
						ActiveApparel = GetWorld()->SpawnActor<ATApparel>(Attire, SpawnParams);

						if (ActiveApparel != nullptr)
						{
							GetMesh()->SetMaterial(0, ActiveApparel->ApparelMaterial);

							GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Green, TEXT("Donned New Apparel"));
						}
					}
				}
			}
		}
	}
}

void ATachyonCharacter::SetApparel(int ApparelIndex)
{
	iApparelIndex = ApparelIndex;
	DonApparel();
	
	if (Role < ROLE_Authority)
	{
		ServerSetApparel(ApparelIndex);
	}
}
void ATachyonCharacter::ServerSetApparel_Implementation(int ApparelIndex)
{
	SetApparel(ApparelIndex);
}
bool ATachyonCharacter::ServerSetApparel_Validate(int ApparelIndex)
{
	return true;
}


////////////////////////////////////////////////////////////////////////
// NETWORK REPLICATION
void ATachyonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATachyonCharacter, InputX);
	DOREPLIFETIME(ATachyonCharacter, InputZ);
	DOREPLIFETIME(ATachyonCharacter, Charge);
	DOREPLIFETIME(ATachyonCharacter, Health);
	DOREPLIFETIME(ATachyonCharacter, AttackTimer);
	DOREPLIFETIME(ATachyonCharacter, bShooting);
	DOREPLIFETIME(ATachyonCharacter, WindupTimer);
	DOREPLIFETIME(ATachyonCharacter, bJumping);
	DOREPLIFETIME(ATachyonCharacter, DiminishingJumpValue);
	DOREPLIFETIME(ATachyonCharacter, ActiveApparel);
	DOREPLIFETIME(ATachyonCharacter, iApparelIndex);
}


// Print to screen
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("sizing"));
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Charge: %f"), Charge));