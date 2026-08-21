#include "Telemetry/DroneTelemetryComponent.h"

#include "GameFramework/Actor.h"
#include "TimerManager.h"

UDroneTelemetryComponent::UDroneTelemetryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneTelemetryComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshTelemetry();

	if (UWorld* World = GetWorld())
	{
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UDroneTelemetryComponent::SetAltitudeReferenceZCentimeters(const float InReferenceZCentimeters)
{
	AltitudeReferenceZCentimeters = InReferenceZCentimeters;
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

	OnTelemetryUpdated.Broadcast(LatestSnapshot);
}

FDroneTelemetrySnapshot UDroneTelemetryComponent::CalculateSnapshot(
	const FVector& VelocityCentimetersPerSecond,
	const FVector& WorldLocationCentimeters,
	const float ActorYawDegrees,
	const float ReferenceZCentimeters)
{
	FDroneTelemetrySnapshot Snapshot;
	Snapshot.SpeedKilometersPerHour = VelocityCentimetersPerSecond.Size() * 0.036f;
	Snapshot.AltitudeMeters = (WorldLocationCentimeters.Z - ReferenceZCentimeters) * 0.01f;
	Snapshot.VerticalSpeedMetersPerSecond = VelocityCentimetersPerSecond.Z * 0.01f;
	Snapshot.HeadingDegrees = FRotator::ClampAxis(ActorYawDegrees);
	return Snapshot;
}
