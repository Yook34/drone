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
 * 기존 Third Person Character와 분리한 Drone 조종 Prototype Pawn.
 *
 * 이 클래스는 입력·이동·카메라와 Telemetry Component 소유까지만 담당한다.
 * HUD 생성은 PlayerController, 화면 외형은 WBP가 담당하므로 Pawn 교체 때 UI가 사라지지 않는다.
 * 현재 수치와 입력 Asset은 최종 비행 물리·감도·네트워크 규칙이 아니다.
 */
UCLASS(Blueprintable)
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

	/** 충돌과 이동의 기준 Root. Visual Mesh와 분리해 구매 에셋 교체 영향을 줄인다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	/** 현재 Engine 기본 도형을 표시하는 외형 전용 Component. Collision은 사용하지 않는다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prototype|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UFloatingPawnMovement> PrototypeMovementComponent;

	/** HUD와 Tutorial 기록기에 기본 0.1초 주기 및 명시적 즉시 갱신 Snapshot을 공급한다. */
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
	/** 이 Pawn이 실제로 추가한 IMC만 나중에 제거하기 위해 소유 기록을 보관한다. */
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedInputSubsystem;
	TWeakObjectPtr<UInputMappingContext> AppliedMappingContext;
	bool bPrototypeMappingContextAdded = false;

	/** 로컬 Player에 Prototype IMC를 정확히 한 번 추가한다. */
	void ApplyPrototypeMappingContext();

	/** 다른 시스템의 IMC는 건드리지 않고 이 Pawn이 추가한 Mapping만 제거한다. */
	void RemovePrototypeMappingContext();

	void Move(const FInputActionValue& Value);
	void ChangeAltitude(const FInputActionValue& Value);
	void ChangeYaw(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ChangeCameraPitch(const FInputActionValue& Value);
	void AdjustCameraPitch(float PitchDeltaDegrees);
};
