#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tutorial/DroneTrainingLapRecorderComponent.h"
#include "Tutorial/DroneTrainingRecordTypes.h"

namespace DroneTrainingComparisonTest
{
FDroneTrainingLapRecord MakeLap(
	const double ElapsedSeconds,
	const double SpeedKilometersPerHour,
	const double FirstSegmentSeconds,
	const double SecondSegmentSeconds)
{
	FDroneTrainingLapRecord Lap;
	Lap.bCompleted = true;
	Lap.ElapsedSeconds = ElapsedSeconds;
	Lap.TravelDistanceMeters = 100.0;
	Lap.AverageSpeedKilometersPerHour = SpeedKilometersPerHour;

	FDroneTrainingSegmentRecord FirstSegment;
	FirstSegment.SegmentIndex = 0;
	FirstSegment.FromGateIndex = 0;
	FirstSegment.ToGateIndex = 1;
	FirstSegment.ElapsedSeconds = FirstSegmentSeconds;
	FirstSegment.TravelDistanceMeters = 50.0;
	FirstSegment.AverageSpeedKilometersPerHour = SpeedKilometersPerHour - 1.0;
	Lap.Segments.Add(FirstSegment);

	FDroneTrainingSegmentRecord SecondSegment;
	SecondSegment.SegmentIndex = 1;
	SecondSegment.FromGateIndex = 1;
	SecondSegment.ToGateIndex = 2;
	SecondSegment.ElapsedSeconds = SecondSegmentSeconds;
	SecondSegment.TravelDistanceMeters = 50.0;
	SecondSegment.AverageSpeedKilometersPerHour = SpeedKilometersPerHour + 1.0;
	Lap.Segments.Add(SecondSegment);
	return Lap;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingComparisonTest,
	"Drone.Tutorial.TrainingComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingComparisonTest::RunTest(const FString& Parameters)
{
	using namespace DroneTrainingComparisonTest;
	auto CheckNear = [this](
		const TCHAR* What,
		const double Actual,
		const double Expected,
		const double Tolerance)
	{
		TestTrue(What, FMath::IsNearlyEqual(Actual, Expected, Tolerance));
	};

	const FDroneTrainingLapRecord FirstLap = MakeLap(10.0, 30.0, 4.0, 6.0);
	const FDroneTrainingLapComparison FirstResult =
		UDroneTrainingLapRecorderComponent::BuildLapComparison({}, FirstLap);
	TestFalse(TEXT("First success has no previous average"), FirstResult.bHasPreviousBaseline);
	TestEqual(TEXT("First success compares against zero previous Laps"), FirstResult.PreviousLapCount, 0);
	TestTrue(TEXT("First success starts the Best time"), FirstResult.bIsNewBestTime);
	CheckNear(TEXT("First success stores its own Best time"), FirstResult.BestElapsedSeconds, 10.0, 0.001);
	TestEqual(TEXT("First success still creates per-segment results"), FirstResult.SegmentComparisons.Num(), 2);

	TArray<FDroneTrainingLapRecord> PreviousLaps;
	PreviousLaps.Add(FirstLap);
	PreviousLaps.Add(MakeLap(12.0, 28.0, 5.0, 7.0));
	// 완료되지 않은 부분 시도는 평균과 Best를 오염시키지 않는다.
	PreviousLaps.Add(FDroneTrainingLapRecord());

	const FDroneTrainingLapRecord FastLap = MakeLap(9.0, 34.0, 3.5, 5.5);
	const FDroneTrainingLapComparison FastResult =
		UDroneTrainingLapRecorderComponent::BuildLapComparison(PreviousLaps, FastLap);
	TestTrue(TEXT("A later success has a previous baseline"), FastResult.bHasPreviousBaseline);
	TestEqual(TEXT("Only completed valid previous Laps are counted"), FastResult.PreviousLapCount, 2);
	CheckNear(TEXT("Previous average excludes the current 9-second Lap"), FastResult.PreviousAverageElapsedSeconds, 11.0, 0.001);
	CheckNear(TEXT("Previous speed average excludes the current speed"), FastResult.PreviousAverageSpeedKilometersPerHour, 29.0, 0.001);
	CheckNear(TEXT("Negative time Delta means faster"), FastResult.ElapsedDeltaFromPreviousAverageSeconds, -2.0, 0.001);
	CheckNear(TEXT("Positive speed Delta means faster"), FastResult.SpeedDeltaFromPreviousAverageKilometersPerHour, 5.0, 0.001);
	TestTrue(TEXT("Nine seconds is a new Best"), FastResult.bIsNewBestTime);
	CheckNear(TEXT("Current new Best becomes the stored Best"), FastResult.BestElapsedSeconds, 9.0, 0.001);

	if (FastResult.SegmentComparisons.Num() == 2)
	{
		const FDroneTrainingSegmentComparison& FirstSegment = FastResult.SegmentComparisons[0];
		TestEqual(TEXT("Segment comparison uses two previous records"), FirstSegment.PreviousRecordCount, 2);
		CheckNear(TEXT("First segment previous average is 4.5 seconds"), FirstSegment.PreviousAverageElapsedSeconds, 4.5, 0.001);
		CheckNear(TEXT("First segment is one second faster than previous average"), FirstSegment.ElapsedDeltaFromPreviousAverageSeconds, -1.0, 0.001);
		TestTrue(TEXT("First segment creates a new Best"), FirstSegment.bIsNewBestTime);
	}

	const FDroneTrainingLapRecord SlowerLap = MakeLap(11.5, 27.0, 4.5, 7.0);
	const FDroneTrainingLapComparison SlowerResult =
		UDroneTrainingLapRecorderComponent::BuildLapComparison(PreviousLaps, SlowerLap);
	TestFalse(TEXT("A slower current Lap is not a new Best"), SlowerResult.bIsNewBestTime);
	CheckNear(TEXT("Previous 10-second Best remains"), SlowerResult.BestElapsedSeconds, 10.0, 0.001);
	CheckNear(TEXT("Positive time Delta means slower"), SlowerResult.ElapsedDeltaFromPreviousAverageSeconds, 0.5, 0.001);

	return !HasAnyErrors();
}

#endif
