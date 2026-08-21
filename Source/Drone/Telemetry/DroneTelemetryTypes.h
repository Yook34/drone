#pragma once

#include "CoreMinimal.h"
#include "DroneTelemetryTypes.generated.h"

USTRUCT(BlueprintType)
struct FDroneTelemetrySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float SpeedKilometersPerHour = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float AltitudeMeters = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float VerticalSpeedMetersPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float HeadingDegrees = 0.0f;
};
