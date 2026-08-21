#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Telemetry/DroneTelemetryTypes.h"
#include "TimerManager.h"
#include "DroneTelemetryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FDroneTelemetryUpdatedSignature,
	FDroneTelemetrySnapshot,
	Snapshot);

UCLASS(ClassGroup=(Drone), BlueprintType, meta=(BlueprintSpawnableComponent))
class UDroneTelemetryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneTelemetryComponent();

	UPROPERTY(BlueprintAssignable, Category="Drone|Telemetry")
	FDroneTelemetryUpdatedSignature OnTelemetryUpdated;

	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	FDroneTelemetrySnapshot GetLatestSnapshot() const { return LatestSnapshot; }

	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	float GetUpdateIntervalSeconds() const { return UpdateIntervalSeconds; }

	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	float GetAltitudeReferenceZCentimeters() const { return AltitudeReferenceZCentimeters; }

	UFUNCTION(BlueprintCallable, Category="Drone|Telemetry")
	void SetAltitudeReferenceZCentimeters(float InReferenceZCentimeters);

	UFUNCTION(BlueprintCallable, Category="Drone|Telemetry")
	void RefreshTelemetry();

	static FDroneTelemetrySnapshot CalculateSnapshot(
		const FVector& VelocityCentimetersPerSecond,
		const FVector& WorldLocationCentimeters,
		float ActorYawDegrees,
		float ReferenceZCentimeters);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true", ClampMin="0.01", Units="s"))
	float UpdateIntervalSeconds = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true", Units="cm"))
	float AltitudeReferenceZCentimeters = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true"))
	FDroneTelemetrySnapshot LatestSnapshot;

	FTimerHandle UpdateTimerHandle;
};
