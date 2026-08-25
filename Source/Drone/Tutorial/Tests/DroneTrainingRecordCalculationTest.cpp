#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tutorial/DroneTrainingLapRecorderComponent.h"
#include "Tutorial/DroneTrainingRecordTypes.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingRecordCalculationTest,
	"Drone.Tutorial.TrainingRecordCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingRecordCalculationTest::RunTest(const FString& Parameters)
{
	// Unreal cm와 World Game Time 초를 TUT-03 표시 단위로 바꾸는 핵심 식을 고정한다.
	TestTrue(
		TEXT("1000 cm over two seconds equals 18 km/h"),
		FMath::IsNearlyEqual(
			UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(1000.0, 2.0),
			18.0,
			UE_DOUBLE_KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("A 55 m eight-second Lap averages 24.75 km/h"),
		FMath::IsNearlyEqual(
			UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(5500.0, 8.0),
			24.75,
			UE_DOUBLE_KINDA_SMALL_NUMBER));

	// 0초, 역행 시간, 비정상 값이 Blueprint 결과에 NaN/Inf를 흘리지 않게 한다.
	TestEqual(
		TEXT("Zero duration safely returns zero speed"),
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(1000.0, 0.0),
		0.0);
	TestEqual(
		TEXT("Negative duration safely returns zero speed"),
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(1000.0, -1.0),
		0.0);
	TestEqual(
		TEXT("Negative distance safely returns zero speed"),
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(-1000.0, 1.0),
		0.0);
	TestEqual(
		TEXT("NaN distance safely returns zero speed"),
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(
			std::numeric_limits<double>::quiet_NaN(),
			1.0),
		0.0);
	TestEqual(
		TEXT("Infinite distance safely returns zero speed"),
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(
			std::numeric_limits<double>::infinity(),
			1.0),
		0.0);

	// Struct 기본값은 UI가 기록 전 상태를 안전하게 표시할 수 있어야 한다.
	const FDroneTrainingLapRecord EmptyLap;
	TestFalse(TEXT("Default Lap is not marked completed"), EmptyLap.bCompleted);
	TestEqual(TEXT("Default Lap owns no Segments"), EmptyLap.Segments.Num(), 0);
	TestTrue(TEXT("Default Lap elapsed time is finite"), FMath::IsFinite(EmptyLap.ElapsedSeconds));
	TestTrue(TEXT("Default Lap speed is finite"), FMath::IsFinite(EmptyLap.AverageSpeedKilometersPerHour));

	return !HasAnyErrors();
}

#endif
