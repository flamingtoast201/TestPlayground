#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaptorMovementComponent.generated.h"

class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaptorLandingRequested, const FHitResult&, LandingHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRaptorFatalImpact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRaptorLeftPlayArea);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API URaptorMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URaptorMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void SetFlightPitchInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void SetFlightTurnInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void SetFlightForwardInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void SetFlightBankInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void RequestFlap();

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void RequestLand();

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void CompleteLanding();

	UFUNCTION(BlueprintCallable, Category = "Raptor Flight")
	void SetFlightActive(bool bActive);

	UPROPERTY(BlueprintAssignable, Category = "Raptor")
	FOnRaptorLandingRequested OnLandingRequested;

	UPROPERTY(BlueprintAssignable, Category = "Raptor")
	FOnRaptorFatalImpact OnFatalImpact;

	UPROPERTY(BlueprintAssignable, Category = "Raptor")
	FOnRaptorLeftPlayArea OnLeftPlayArea;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Curves")
	TObjectPtr<UCurveFloat> TurnRateCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Curves")
	TObjectPtr<UCurveFloat> FlapFalloffCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Speed")
	float DiveSpeedGainRate = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Speed")
	float ClimbSpeedLossRate = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Speed")
	float ForwardThrustRate = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Speed")
	float BrakeRate = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Speed")
	float MinFlightSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Bank")
	float MaxBankAngle = 65.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Bank")
	float BankInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Turn")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Pitch")
	float PitchRate = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Flap")
	float FlapVelocityBoost = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Flap")
	float FlapBoostInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Flap")
	float MaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Flap")
	float StaminaCostPerFlap = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Flap")
	float StaminaRecoveryRate = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Altitude")
	float CloudSeaZ = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Altitude")
	float SpaceBoundaryHeight = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Impact")
	float FatalImpactSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Land")
	float LandTraceDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Land")
	TEnumAsByte<ECollisionChannel> LandTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Land")
	float LandingApproachSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Raptor|Land")
	float LandingArrivalTolerance = 15.f;

private:
	float PitchInput = 0.f;
	float TurnInput = 0.f;
	float BankInput = 0.f;
	float ForwardInput = 0.f;
	float CurrentBankAngle = 0.f;
	float CurrentSpeed = 800.f;
	float CurrentStamina = 100.f;
	float FlapBoostTarget = 0.f;
	bool bLandingRequested = false;
	bool bIsLanding = false;
	bool bFlappedThisTick = false;
	bool bIsFlightActive = false;
	FVector Velocity = FVector::ZeroVector;
	FVector LandingTargetLocation = FVector::ZeroVector;
	TWeakObjectPtr<class UCharacterMovementComponent> CachedCharacterMovement;

	void TickFlightRotation(float DeltaTime);
	void TickBank(float DeltaTime);
	void TickTurn(float DeltaTime);
	void TickPitchSpeedTrade(float DeltaTime);
	void TickForwardThrust(float DeltaTime);
	void TickStamina(float DeltaTime);
	void TickFlapBoost(float DeltaTime);
	void TickLandingRequest();
	void TickLandingMovement(float DeltaTime);
	void TickBoundsCheck();
	void MoveAlongVelocity(float DeltaTime);

	float GetHeightAboveCloudSea() const;
	float GetTurnRateMultiplier() const;
	float GetFlapPowerMultiplier() const;
	bool HasStaminaForFlap() const;
	bool TraceForLandingSpot(FHitResult& OutHit) const;
	void EvaluateGroundImpact(const FHitResult& Hit);
};