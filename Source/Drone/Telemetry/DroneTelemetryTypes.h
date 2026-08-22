#pragma once

#include "CoreMinimal.h"
#include "DroneTelemetryTypes.generated.h"

/**
 * 한 번의 Telemetry 갱신에서 UI·Tutorial 기록 시스템으로 전달할 값 묶음.
 * 모든 단위 변환을 Component에서 끝내므로 Widget은 계산하지 않고 표시만 한다.
 */
USTRUCT(BlueprintType)
struct FDroneTelemetrySnapshot
{
	GENERATED_BODY()

	/** 3축 Velocity 전체 크기(상승·하강 포함)를 km/h로 변환한 속력이다. */
	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float SpeedKilometersPerHour = 0.0f;

	/** World Z와 설정된 기준면 Z의 차이. 지형 Line Trace 기반 AGL은 아직 아니다. */
	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float AltitudeMeters = 0.0f;

	/** World Z 속도. 양수는 상승, 음수는 하강이다. */
	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float VerticalSpeedMetersPerSecond = 0.0f;

	/** Actor의 World Yaw를 0 이상 360 미만으로 정규화한 값이며 진북 Compass는 아니다. */
	UPROPERTY(BlueprintReadOnly, Category="Drone|Telemetry")
	float HeadingDegrees = 0.0f;
};
