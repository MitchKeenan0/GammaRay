// Fill out your copyright notice in the Description page of Project Settings.

#include "TachyonMovementComponent.h"
#include "GameFramework/Character.h"


UTachyonMovementComponent::UTachyonMovementComponent(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	GravityScale = 0.0f;
	AirControl = 1.0f;
	MaxFlySpeed = 1000.0f;
	MovementMode = MOVE_Flying;
	DefaultLandMovementMode = MOVE_Flying;
	bConstrainToPlane = true;
	SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));
	
	/*NetworkMaxSmoothUpdateDistance = 500.0f;
	NetworkMinTimeBetweenClientAckGoodMoves = 0.0f;
	NetworkMinTimeBetweenClientAdjustments = 0.0f;
	NetworkMinTimeBetweenClientAdjustmentsLargeCorrection = 0.0f;*/

	MaxAcceleration = 5000.0f;
	BrakingFrictionFactor = 25.0f;
}


// Set input flags on character from saved inputs
void UTachyonMovementComponent::UpdateFromCompressedFlags(uint8 Flags)//Client only
{
	Super::UpdateFromCompressedFlags(Flags);

}

class FNetworkPredictionData_Client* UTachyonMovementComponent::GetPredictionData_Client() const 
{
	check(PawnOwner != NULL); 
	//check(PawnOwner->Role < ROLE_Authority);

	if (!ClientPredictionData) 
	{
		UTachyonMovementComponent* MutableThis = const_cast<UTachyonMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MyMovement(*this); MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f; MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void FSavedMove_MyMovement::Clear() {
	Super::Clear();


}

uint8 FSavedMove_MyMovement::GetCompressedFlags() const 
{
	uint8 Result = Super::GetCompressedFlags();
	return Result;
}

bool FSavedMove_MyMovement::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const 
{

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_MyMovement::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) 
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UTachyonMovementComponent* CharMov = Cast<UTachyonMovementComponent>(Character->GetCharacterMovement()); 
	if (CharMov) 
	{

	}
}

void FSavedMove_MyMovement::PrepMoveFor(class ACharacter* Character) 
{
	Super::PrepMoveFor(Character);

	UTachyonMovementComponent* CharMov = Cast<UTachyonMovementComponent>(Character->GetCharacterMovement()); 
	if (CharMov) 
	{

	}
}

FNetworkPredictionData_Client_MyMovement::FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent & ClientMovement) 
	:Super(ClientMovement)
{
	
}

FSavedMovePtr FNetworkPredictionData_Client_MyMovement::AllocateNewMove()
{ 
	return FSavedMovePtr(new FSavedMove_MyMovement()); 
}