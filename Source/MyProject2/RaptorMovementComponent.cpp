//===========================================================================================
// Libraries to Include
//===========================================================================================
#include "RaptorMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
//===========================================================================================
// CPP URaptorMovementComponent Class Implementation
//===========================================================================================
// Function Definitions for URaptorMovementComponent Class
//===========================================================================================
// Function, Constructor
//-------------------------------------------------------------------------------------------
URaptorMovementComponent::URaptorMovementComponent() {
	PrimaryComponentTick.bCanEverTick = true;
	CurrentStamina = MaxStamina;
}
//===========================================================================================
// Function, Begin Play
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::BeginPlay() {
	Super::BeginPlay();
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner())) {
		CachedCharacterMovement = OwnerCharacter->GetCharacterMovement();
	}
}
//===========================================================================================
// Function, On Tick
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	if (!Owner || !bIsFlightActive) {
		return;
	} if (bIsLanding) {
		TickLandingMovement(DeltaTime);
		return;
	}
	bFlappedThisTick = false;
	TickFlightRotation(DeltaTime);
	TickPitchSpeedTrade(DeltaTime);
	TickForwardThrust(DeltaTime);
	TickFlapBoost(DeltaTime);
	TickStamina(DeltaTime);
	TickLandingRequest();
	TickBoundsCheck();
	MoveAlongVelocity(DeltaTime);
}
//===========================================================================================
// Function, Set Flight Pitch Input
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::SetFlightPitchInput(float Value) {
	PitchInput = FMath::Clamp(Value, -1.f, 1.f);
}
//===========================================================================================
// Function, Set Flight Turn Input
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::SetFlightTurnInput(float Value) {
	TurnInput = FMath::Clamp(Value, -1.f, 1.f);
}
//===========================================================================================
// Function, Set Flight Forward Input
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::SetFlightForwardInput(float Value) {
	ForwardInput = FMath::Clamp(Value, -1.f, 1.f);
}
//===========================================================================================
// Function, Set Flight Bank Input
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::SetFlightBankInput(float Value) {
	BankInput = FMath::Clamp(Value, -1.f, 1.f);
}
//===========================================================================================
// Function, Request Flap
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::RequestFlap() {
	if (bIsLanding || !HasStaminaForFlap()) {
		return;
	}
	CurrentStamina -= StaminaCostPerFlap;
	FlapBoostTarget += FlapVelocityBoost * GetFlapPowerMultiplier();
	bFlappedThisTick = true;
}
//===========================================================================================
// Function, Request Land
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::RequestLand() {
	bLandingRequested = true;
}
//===========================================================================================
// Function, Complete Landing
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::CompleteLanding() {
	bIsLanding = false;
	SetFlightActive(false);
}
//===========================================================================================
// Function, Set Flight Active
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::SetFlightActive(bool bActive) {
	bIsFlightActive = bActive;
	if (UCharacterMovementComponent* CharMove = CachedCharacterMovement.Get()) {
		if (bActive) {
			CurrentSpeed = FMath::Max(CharMove->Velocity.Size(), MinFlightSpeed);
			CharMove->SetMovementMode(MOVE_None);
		} else {
			CharMove->SetMovementMode(MOVE_Falling);
		}
	}
}
//===========================================================================================
// Function, Tick Flight Rotation
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickFlightRotation(float DeltaTime) {
	AActor* Owner = GetOwner();
	if (!Owner) return;
	TickBank(DeltaTime);
	TickTurn(DeltaTime);
	FRotator NewRotation = Owner->GetActorRotation();
	NewRotation.Pitch += PitchInput * PitchRate * DeltaTime;
	NewRotation.Roll = CurrentBankAngle;
	Owner->SetActorRotation(NewRotation);
}
//===========================================================================================
// Function, Tick Bank
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickBank(float DeltaTime) {
	const float TargetBank = BankInput * MaxBankAngle;
	CurrentBankAngle = FMath::FInterpTo(CurrentBankAngle, TargetBank, DeltaTime, BankInterpSpeed);
}
//===========================================================================================
// Function, Tick Turn
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickTurn(float DeltaTime) {
	AActor* Owner = GetOwner();
	if (!Owner) return;
	FRotator NewRotation = Owner->GetActorRotation();
	NewRotation.Yaw += TurnInput * BaseTurnRate * GetTurnRateMultiplier() * DeltaTime;
	Owner->SetActorRotation(NewRotation);
}
//===========================================================================================
// Function, Tick Pitch Speed Trade
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickPitchSpeedTrade(float DeltaTime) {
	AActor* Owner = GetOwner();
	if (!Owner) return;
	const float PitchDot = FVector::DotProduct(Owner->GetActorForwardVector(), FVector::UpVector);
	if (PitchDot > 0.f) {
		const float Loss = ClimbSpeedLossRate * FMath::Square(PitchDot) * DeltaTime;
		CurrentSpeed = FMath::Max(CurrentSpeed - Loss, 0.f);
	} else if (PitchDot < 0.f) {
		const float Gain = DiveSpeedGainRate * -PitchDot * DeltaTime;
		CurrentSpeed += Gain;
	}
	Velocity = Owner->GetActorForwardVector() * CurrentSpeed;
	if (CurrentSpeed <= KINDA_SMALL_NUMBER) {
		Velocity.Z += GetWorld()->GetGravityZ() * DeltaTime;
	}
}
//===========================================================================================
// Function, Tick Forward Thrust
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickForwardThrust(float DeltaTime) {
	if (ForwardInput > 0.f) {
		CurrentSpeed += ForwardInput * ForwardThrustRate * DeltaTime;
	} else if (ForwardInput < 0.f) {
		CurrentSpeed = FMath::Max(CurrentSpeed + ForwardInput * BrakeRate * DeltaTime, 0.f);
	}
}
//===========================================================================================
// Function, Tick Flap Boost
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickFlapBoost(float DeltaTime) {
	if (FlapBoostTarget <= KINDA_SMALL_NUMBER) { return; }
	const float PreviousSpeed = CurrentSpeed;
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, CurrentSpeed + FlapBoostTarget, DeltaTime, FlapBoostInterpSpeed);
	FlapBoostTarget -= (CurrentSpeed - PreviousSpeed);
}
//===========================================================================================
// Function, Tick Stamina
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickStamina(float DeltaTime) {
	if (bFlappedThisTick) {
		return;
	}
	CurrentStamina = FMath::Min(CurrentStamina + StaminaRecoveryRate * DeltaTime, MaxStamina);
}
//===========================================================================================
// Function, Tick Landing Request
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickLandingRequest() {
	if (!bLandingRequested) { return; }
	bLandingRequested = false;
	FHitResult LandingHit;
	if (TraceForLandingSpot(LandingHit)) {
		bIsLanding = true;
		LandingTargetLocation = LandingHit.ImpactPoint;
		OnLandingRequested.Broadcast(LandingHit);
	}
}
//===========================================================================================
// Function, Tick Landing Movement
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickLandingMovement(float DeltaTime) {
	AActor* Owner = GetOwner();
	if (!Owner) return;
	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector ToTarget = LandingTargetLocation - CurrentLocation;
	if (ToTarget.SizeSquared() <= FMath::Square(LandingArrivalTolerance)) {
		CompleteLanding();
		return;
	}
	const FVector MoveDelta = ToTarget.GetSafeNormal() * LandingApproachSpeed * DeltaTime;
	FRotator NewRotation = Owner->GetActorRotation();
	NewRotation.Roll = FMath::FInterpTo(NewRotation.Roll, 0.f, DeltaTime, BankInterpSpeed);
	FHitResult Hit(1.f);
	Owner->AddActorWorldOffset(MoveDelta, true, &Hit);
}
//===========================================================================================
// Function, Tick Bounds Check
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::TickBoundsCheck() {
	const float Height = GetHeightAboveCloudSea();
	if (Height < 0.f || Height > SpaceBoundaryHeight) {
		OnLeftPlayArea.Broadcast();
	}
}
//===========================================================================================
// Function, Move Along Velocity
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::MoveAlongVelocity(float DeltaTime) {
	AActor* Owner = GetOwner();
	if (!Owner) return;
	FHitResult Hit(1.f);
	Owner->AddActorWorldOffset(Velocity * DeltaTime, true, &Hit);
	if (Hit.IsValidBlockingHit()) {
		EvaluateGroundImpact(Hit);
	}
}
//===========================================================================================
// Function, Get Height Above Cloud Sea
//-------------------------------------------------------------------------------------------
float URaptorMovementComponent::GetHeightAboveCloudSea() const {
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation().Z - CloudSeaZ : 0.f;
}
//===========================================================================================
// Function, Get Turn Rate Multiplier
//-------------------------------------------------------------------------------------------
float URaptorMovementComponent::GetTurnRateMultiplier() const {
	if (TurnRateCurve) { return TurnRateCurve->GetFloatValue(CurrentSpeed); }
	return FMath::Clamp(12000.f / FMath::Max(CurrentSpeed, 1.f), 5.f, 100.f);
}
//===========================================================================================
// Function, Get Flap Power Multiplier
//-------------------------------------------------------------------------------------------
float URaptorMovementComponent::GetFlapPowerMultiplier() const {
	if (FlapFalloffCurve) { return FlapFalloffCurve->GetFloatValue(GetHeightAboveCloudSea()); }
	return FMath::Clamp(FMath::Exp(-GetHeightAboveCloudSea() / 300.f), 0.f, 1.f);
}
//===========================================================================================
// Function, Has Stamina For Flap
//-------------------------------------------------------------------------------------------
bool URaptorMovementComponent::HasStaminaForFlap() const {
	return CurrentStamina >= StaminaCostPerFlap;
}
//===========================================================================================
// Function, Trace For Landing Spot
//-------------------------------------------------------------------------------------------
bool URaptorMovementComponent::TraceForLandingSpot(FHitResult& OutHit) const {
	AActor* Owner = GetOwner();
	if (!Owner) return false;
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, LandTraceDistance);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, LandTraceChannel, Params);
}
//===========================================================================================
// Function, Evaluate Ground Impact
//-------------------------------------------------------------------------------------------
void URaptorMovementComponent::EvaluateGroundImpact(const FHitResult& Hit) {
	if (bIsLanding) { return; }
	if (Velocity.Size() > FatalImpactSpeed) {
		OnFatalImpact.Broadcast();
	}
}
//===========================================================================================
// End of URaptorMovementComponent Class Implementation
//===========================================================================================