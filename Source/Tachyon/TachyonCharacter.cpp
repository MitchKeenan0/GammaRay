// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonCharacter.h"


// Sets default values
ATachyonCharacter::ATachyonCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bReplicateMovement = true;

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create a camera and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->AirControl = 1.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;
	GetCharacterMovement()->MovementMode = MOVE_Flying;
	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));
}


////////////////////////////////////////////////////////////////////////
// BEGIN PLAY
void ATachyonCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	Health = MaxHealth;
}


////////////////////////////////////////////////////////////////////////
// TICK
void ATachyonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHealth();

	if ((Controller != nullptr) && Controller->IsLocalController())
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Health: %f"), Health));
	}
}


////////////////////////////////////////////////////////////////////////
// INPUT BINDING
void ATachyonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ATachyonCharacter::FireAttack);

	PlayerInputComponent->BindAxis("MoveRight", this, &ATachyonCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ATachyonCharacter::MoveUp);
}


////////////////////////////////////////////////////////////////////////
// MOVEMENT
void ATachyonCharacter::MoveRight(float Value)
{
	if (InputX != Value)
	{
		InputX = Value;
	}
	

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
	if (InputZ != Value)
	{
		InputZ = Value;
	}


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
// ATTACKING
void ATachyonCharacter::FireAttack()
{
	if (Role < ROLE_Authority)
	{
		ServerFireAttack();
	}
	else
	{
		if (HasAuthority() && (ActiveAttack == nullptr))
		{
			if ((AttackClass != nullptr))/// || bMultipleAttacks)
			{
				// Aim by InputY
				float AimClampedInputZ = FMath::Clamp((InputZ * 10.0f), -1.0f, 1.0f);
				FVector FirePosition = GetActorLocation(); ///AttackScene->GetComponentLocation();
				FVector LocalForward = GetActorForwardVector(); /// AttackScene->GetForwardVector();
				LocalForward.Y = 0.0f;
				FRotator FireRotation = LocalForward.Rotation() + FRotator(InputZ * 21.0f, 0.0f, 0.0f); /// AimClampedInputZ
				FireRotation.Yaw = GetActorRotation().Yaw;
				if (FMath::Abs(FireRotation.Yaw) >= 90.0f)
				{
					FireRotation.Yaw = 180.0f;
				}
				else
				{
					FireRotation.Yaw = 0.0f;
				}
				FireRotation.Pitch = FMath::Clamp(FireRotation.Pitch, -21.0f, 21.0f);

				// Spawning
				FActorSpawnParameters SpawnParams;
				ActiveAttack = Cast<ATachyonAttack>(GetWorld()->SpawnActor<ATachyonAttack>(AttackClass, FirePosition, FireRotation, SpawnParams));
				if (ActiveAttack != nullptr)
				{

					// The attack is born
					if (ActiveAttack != nullptr)
					{
						ActiveAttack->InitAttack(this, 1.0f, InputZ); /// PrefireVal, AimClampedInputZ);
					}

					// Position lock, or naw
					if ((ActiveAttack != nullptr) && ActiveAttack->LockedEmitPoint)
					{
						ActiveAttack->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
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


////////////////////////////////////////////////////////////////////////
// MODIFY HEALTH
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

void ATachyonCharacter::UpdateHealth()
{
	// Update smooth health value
	if (MaxHealth < Health)
	{
		float InterpSpeed = FMath::Abs(Health - MaxHealth) * 5.1f;
		Health = FMath::FInterpConstantTo(Health, MaxHealth, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), InterpSpeed);
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

void ATachyonCharacter::UpdateAttack()
{
	if (ActiveAttack != nullptr)
	{
		if (ActiveAttack->IsPendingKillOrUnreachable())
		{
			ActiveAttack = nullptr;
		}
	}
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
}


// Print to screen
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("sizing"));
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Charge: %f"), Charge));