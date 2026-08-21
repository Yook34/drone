#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePrototypePawn.generated.h"

class AController;
class UCameraComponent;
class UDroneTelemetryComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * Isolated flight-control spike used to compare a Pawn-based drone with the
 * existing Third Person Character. Values and input assets are prototypes,
 * not final flight, control, or networking rules.
 */
UCLASS()
class ADronePrototypePawn : public APawn
{
	GENERATED_BODY()

public:
	ADronePrototypePawn();

	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	USphereComponent* GetCollisionComponent() const { return CollisionComponent; }
	UStaticMeshComponent* GetVisualMeshComponent() const { return VisualMeshComponent; }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UFloatingPawnMovement* GetPrototypeMovementComponent() const { return PrototypeMovementComponent; }
	UDroneTelemetryComponent* GetTelemetryComponent() const { return TelemetryComponent; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UFloatingPawnMovement> PrototypeMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDroneTelemetryComponent> TelemetryComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputMappingContext> PrototypeMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> AltitudeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> YawAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> CameraPitchRateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Input", meta=(AllowPrivateAccess="true"))
	int32 PrototypeMappingPriority = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Movement", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float PrototypeYawRateDegreesPerSecond = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Camera", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float PrototypeMouseYawDegreesPerInput = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Camera", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float PrototypeMousePitchDegreesPerInput = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Camera", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float PrototypeGamepadPitchRateDegreesPerSecond = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Camera", meta=(ClampMin="-89.0", ClampMax="89.0", AllowPrivateAccess="true"))
	float PrototypeMinimumCameraPitchDegrees = -70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Prototype|Camera", meta=(ClampMin="-89.0", ClampMax="89.0", AllowPrivateAccess="true"))
	float PrototypeMaximumCameraPitchDegrees = 20.0f;

private:
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedInputSubsystem;
	TWeakObjectPtr<UInputMappingContext> AppliedMappingContext;
	bool bPrototypeMappingContextAdded = false;

	void ApplyPrototypeMappingContext();
	void RemovePrototypeMappingContext();

	void Move(const FInputActionValue& Value);
	void ChangeAltitude(const FInputActionValue& Value);
	void ChangeYaw(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ChangeCameraPitch(const FInputActionValue& Value);
	void AdjustCameraPitch(float PitchDeltaDegrees);
};
