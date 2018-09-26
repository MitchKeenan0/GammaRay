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

// Called when the game starts or when spawned
void ATachyonCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATachyonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
		// Spawning
		if (HasAuthority() && (ActiveAttack == nullptr))
		{
			if ((AttackClass != nullptr))/// || bMultipleAttacks)
			{
				FActorSpawnParameters SpawnParams;
				FVector FirePosition = GetActorLocation();
				FRotator FireRotation = GetActorForwardVector().Rotation();
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
// NETWORKED PROPERTY REPLICATION
void ATachyonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATachyonCharacter, InputX);
	DOREPLIFETIME(ATachyonCharacter, InputZ);
}


// Print to screen
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, TEXT("sizing"));
// GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::White, FString::Printf(TEXT("Charge: %f"), Charge));