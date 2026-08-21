#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Telemetry/DroneTelemetryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTelemetryCalculationTest,
	"Drone.Telemetry.Calculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTelemetryCalculationTest::RunTest(const FString& Parameters)
{
	const FVector VelocityCentimetersPerSecond(300.0f, 400.0f, 200.0f);
	const FDroneTelemetrySnapshot Snapshot = UDroneTelemetryComponent::CalculateSnapshot(
		VelocityCentimetersPerSecond,
		FVector(100.0f, -200.0f, 1250.0f),
		-45.0f,
		250.0f);

	const float ExpectedSpeedKilometersPerHour = VelocityCentimetersPerSecond.Size() * 0.036f;
	TestTrue(
		TEXT("Total velocity converts from centimeters per second to kilometers per hour"),
		FMath::IsNearlyEqual(Snapshot.SpeedKilometersPerHour, ExpectedSpeedKilometersPerHour));
	TestTrue(
		TEXT("Altitude is measured from the configured reference plane"),
		FMath::IsNearlyEqual(Snapshot.AltitudeMeters, 10.0f));
	TestTrue(
		TEXT("Vertical speed converts from centimeters per second to meters per second"),
		FMath::IsNearlyEqual(Snapshot.VerticalSpeedMetersPerSecond, 2.0f));
	TestTrue(
		TEXT("Negative yaw is normalized to a compass heading"),
		FMath::IsNearlyEqual(Snapshot.HeadingDegrees, 315.0f));

	const FDroneTelemetrySnapshot WrappedHeadingSnapshot = UDroneTelemetryComponent::CalculateSnapshot(
		FVector::ZeroVector,
		FVector(0.0f, 0.0f, -100.0f),
		720.0f,
		100.0f);

	TestTrue(
		TEXT("Zero velocity produces zero speed"),
		FMath::IsNearlyZero(WrappedHeadingSnapshot.SpeedKilometersPerHour));
	TestTrue(
		TEXT("Altitude remains signed below the reference plane"),
		FMath::IsNearlyEqual(WrappedHeadingSnapshot.AltitudeMeters, -2.0f));
	TestTrue(
		TEXT("Heading wraps into the zero to 359 degree range"),
		FMath::IsNearlyZero(WrappedHeadingSnapshot.HeadingDegrees));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTelemetryDefaultsTest,
	"Drone.Telemetry.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTelemetryDefaultsTest::RunTest(const FString& Parameters)
{
	const UDroneTelemetryComponent* TelemetryDefaults = GetDefault<UDroneTelemetryComponent>();
	TestNotNull(TEXT("Telemetry component CDO exists"), TelemetryDefaults);

	if (TelemetryDefaults)
	{
		TestFalse(
			TEXT("Telemetry component avoids per-frame ticking"),
			TelemetryDefaults->PrimaryComponentTick.bCanEverTick);
		TestEqual(
			TEXT("Default update interval is 0.1 seconds"),
			TelemetryDefaults->GetUpdateIntervalSeconds(),
			0.1f);
		TestEqual(
			TEXT("Default altitude reference uses world zero"),
			TelemetryDefaults->GetAltitudeReferenceZCentimeters(),
			0.0f);
	}

	UDroneTelemetryComponent* TelemetryInstance = NewObject<UDroneTelemetryComponent>();
	TestNotNull(TEXT("Telemetry component instance can be created"), TelemetryInstance);
	if (TelemetryInstance)
	{
		TelemetryInstance->SetAltitudeReferenceZCentimeters(350.0f);
		TestEqual(
			TEXT("Altitude reference can be configured at runtime"),
			TelemetryInstance->GetAltitudeReferenceZCentimeters(),
			350.0f);
	}

	return true;
}

#endif
