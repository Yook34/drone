#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "DroneNPCAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UDroneNPCProfileComponent;
class UDroneSmartObjectReservationComponent;
class UStateTreeAIComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FDroneTargetPerceptionChangedSignature,
	AActor*, TargetActor,
	bool, bSuccessfullySensed);

/**
 * Hostile/Friendly NPC가 함께 사용하는 StateTree·Perception·Smart Object Controller다.
 *
 * Hostile NPC만 Drone Prototype을 교전 대상으로 받아들인다. 감지 순간 순찰 Claim을
 * 해제하고 DroneDetected Event를 StateTree에 보낸다. Friendly NPC는 Base Patrol/Ambient
 * Activity만 기본 검색하며 이 감지 Event로 전투 전환하지 않는다.
 */
UCLASS(Blueprintable)
class DRONE_API ADroneNPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	ADroneNPCAIController();

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	UStateTreeAIComponent* GetStateTreeAIComponent() const { return StateTreeAIComponent; }

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	UAIPerceptionComponent* GetDronePerceptionComponent() const { return DronePerceptionComponent; }

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	UDroneSmartObjectReservationComponent* GetReservationComponent() const { return ReservationComponent; }

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	AActor* GetDetectedDrone() const { return DetectedDrone.Get(); }

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	bool HasDetectedDrone() const { return DetectedDrone.IsValid(); }

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	bool UsesRifle() const;

	UFUNCTION(BlueprintPure, Category="Drone|AI")
	bool UsesShotgun() const;

	/** Profile에 따라 Enemy Patrol 또는 Friendly Base Patrol 검색 Tag를 다시 설정한다. */
	UFUNCTION(BlueprintCallable, Category="Drone|AI|SmartObject")
	void ConfigureDefaultPatrolActivities();

	/** Hostile이며 MG 사용 허용 Profile일 때만 MG Turret Activity 검색으로 전환한다. */
	UFUNCTION(BlueprintCallable, Category="Drone|AI|SmartObject")
	bool PrepareMGTurretSearch();

	UPROPERTY(BlueprintAssignable, Category="Drone|AI|Perception")
	FDroneTargetPerceptionChangedSignature OnDronePerceptionChanged;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UDroneNPCProfileComponent* GetPossessedProfile() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Components")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Components")
	TObjectPtr<UAIPerceptionComponent> DronePerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Components")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Components")
	TObjectPtr<UDroneSmartObjectReservationComponent> ReservationComponent;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Drone|AI|Perception")
	TWeakObjectPtr<AActor> DetectedDrone;
};
