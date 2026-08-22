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

/**
 * Drone Actor의 이동 상태를 주기적 Snapshot Event로 공급하는 공용 Component.
 * 기본 주기는 0.1초(10Hz)이며 BeginPlay·기준면 변경·명시적 Refresh 때는 즉시 갱신한다.
 * Actor/Widget Tick을 사용하지 않고 Timer 한 곳에서 값을 계산해 C++와 Blueprint에 전달한다.
 */
UCLASS(ClassGroup=(Drone), BlueprintType, meta=(BlueprintSpawnableComponent))
class UDroneTelemetryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneTelemetryComponent();

	/** HUD나 Tutorial 기록기가 구독하는 주기적·즉시 갱신 공용 Event다. */
	UPROPERTY(BlueprintAssignable, Category="Drone|Telemetry")
	FDroneTelemetryUpdatedSignature OnTelemetryUpdated;

	/** 다음 Timer까지 기다리지 않고 마지막 계산 결과를 읽을 때 사용한다. */
	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	FDroneTelemetrySnapshot GetLatestSnapshot() const { return LatestSnapshot; }

	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	float GetUpdateIntervalSeconds() const { return UpdateIntervalSeconds; }

	UFUNCTION(BlueprintPure, Category="Drone|Telemetry")
	float GetAltitudeReferenceZCentimeters() const { return AltitudeReferenceZCentimeters; }

	/** Course/Mission 기준면 Z를 cm 단위로 바꾸고 즉시 새 Snapshot을 Broadcast한다. */
	UFUNCTION(BlueprintCallable, Category="Drone|Telemetry")
	void SetAltitudeReferenceZCentimeters(float InReferenceZCentimeters);

	/** 현재 Owner Transform/Velocity로 즉시 다시 계산하고 Event를 보낸다. */
	UFUNCTION(BlueprintCallable, Category="Drone|Telemetry")
	void RefreshTelemetry();

	/** World 의존성이 없는 계산 함수라 단위 테스트에서 경계값을 직접 검증할 수 있다. */
	static FDroneTelemetrySnapshot CalculateSnapshot(
		const FVector& VelocityCentimetersPerSecond,
		const FVector& WorldLocationCentimeters,
		float ActorYawDegrees,
		float ReferenceZCentimeters);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 0.1초 = 10Hz. UI가 매 프레임 계산하지 않도록 공급 주기를 한 곳에서 관리한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true", ClampMin="0.01", Units="s"))
	float UpdateIntervalSeconds = 0.1f;

	/** Tutorial 시작 Pad 또는 Mission이 지정할 World Z 기준면(cm)이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true", Units="cm"))
	float AltitudeReferenceZCentimeters = 0.0f;

	/** 마지막 Broadcast와 동일한 값. 새 HUD가 연결될 때 초기 화면을 즉시 채우는 데 사용한다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Drone|Telemetry", meta=(AllowPrivateAccess="true"))
	FDroneTelemetrySnapshot LatestSnapshot;

	/** EndPlay에서 정확히 이 Timer만 해제하기 위한 Handle이다. */
	FTimerHandle UpdateTimerHandle;
};
