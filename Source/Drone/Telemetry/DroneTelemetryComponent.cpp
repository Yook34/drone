#include "Telemetry/DroneTelemetryComponent.h"

#include "GameFramework/Actor.h"
#include "TimerManager.h"

UDroneTelemetryComponent::UDroneTelemetryComponent()
{
	// 값 공급은 Timer가 담당하므로 Component Tick은 완전히 끈다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneTelemetryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 첫 Timer 주기 동안 빈 값이 보이지 않도록 BeginPlay에서 한 번 즉시 갱신한다.
	RefreshTelemetry();

	if (UWorld* World = GetWorld())
	{
		// 잘못된 에디터 값이 들어와도 0초 반복 Timer가 되지 않게 최소값을 보장한다.
		const float SafeIntervalSeconds = FMath::Max(UpdateIntervalSeconds, 0.01f);
		World->GetTimerManager().SetTimer(
			UpdateTimerHandle,
			this,
			&UDroneTelemetryComponent::RefreshTelemetry,
			SafeIntervalSeconds,
			true,
			SafeIntervalSeconds);
	}
}

void UDroneTelemetryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// World/Actor 종료 뒤 Timer Callback이 남지 않도록 먼저 해제한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UDroneTelemetryComponent::SetAltitudeReferenceZCentimeters(const float InReferenceZCentimeters)
{
	AltitudeReferenceZCentimeters = InReferenceZCentimeters;
	// 기준면을 바꾼 프레임에 HUD도 바로 바뀌어야 하므로 다음 Timer를 기다리지 않는다.
	RefreshTelemetry();
}

void UDroneTelemetryComponent::RefreshTelemetry()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	LatestSnapshot = CalculateSnapshot(
		Owner->GetVelocity(),
		Owner->GetActorLocation(),
		Owner->GetActorRotation().Yaw,
		AltitudeReferenceZCentimeters);

	// 계산은 한 번만 하고 모든 구독자에게 같은 Snapshot을 전달한다.
	OnTelemetryUpdated.Broadcast(LatestSnapshot);
}

FDroneTelemetrySnapshot UDroneTelemetryComponent::CalculateSnapshot(
	const FVector& VelocityCentimetersPerSecond,
	const FVector& WorldLocationCentimeters,
	const float ActorYawDegrees,
	const float ReferenceZCentimeters)
{
	FDroneTelemetrySnapshot Snapshot;
	// Unreal 기본 단위 cm/s → km/h: cm/s × 0.01(m/cm) × 3.6(km/h per m/s) = × 0.036.
	Snapshot.SpeedKilometersPerHour = VelocityCentimetersPerSecond.Size() * 0.036f;
	// Unreal cm → m 변환. 기준면 아래에서는 의도적으로 음수가 유지된다.
	Snapshot.AltitudeMeters = (WorldLocationCentimeters.Z - ReferenceZCentimeters) * 0.01f;
	Snapshot.VerticalSpeedMetersPerSecond = VelocityCentimetersPerSecond.Z * 0.01f;
	// ClampAxis는 World Yaw를 [0, 360) 범위로 정규화한다.
	Snapshot.HeadingDegrees = FRotator::ClampAxis(ActorYawDegrees);
	return Snapshot;
}
