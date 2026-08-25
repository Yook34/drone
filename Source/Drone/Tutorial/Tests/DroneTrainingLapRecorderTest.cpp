#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypePawn.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tutorial/DroneTrainingCourse.h"
#include "Tutorial/DroneTrainingGate.h"
#include "Tutorial/DroneTrainingGateSequenceComponent.h"
#include "Tutorial/DroneTrainingLapRecorderComponent.h"
#include "Tutorial/DroneTrainingRecordTypes.h"

#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingLapRecorderTest,
	"Drone.Tutorial.TrainingLapRecorder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingLapRecorderTest::RunTest(const FString& Parameters)
{
	// 기록기는 Telemetry Event만 구독하므로 자체 Tick을 만들지 않아야 한다.
	const UDroneTrainingLapRecorderComponent* RecorderDefaults =
		GetDefault<UDroneTrainingLapRecorderComponent>();
	TestNotNull(TEXT("Training Lap Recorder CDO exists"), RecorderDefaults);
	if (RecorderDefaults)
	{
		TestFalse(
			TEXT("Training Lap Recorder avoids per-frame ticking"),
			RecorderDefaults->PrimaryComponentTick.bCanEverTick);
	}

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	UWorld* TestWorld = WorldWrapper.GetTestWorld();
	TestNotNull(TEXT("Lap Recorder test World exists"), TestWorld);
	if (!TestWorld)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADroneTrainingCourse* Course = TestWorld->SpawnActor<ADroneTrainingCourse>(
		ADroneTrainingCourse::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	TestNotNull(TEXT("Training Course spawns for Lap recording"), Course);
	if (!Course)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	// Gate 0은 출발선이고 Gate 2는 결승선이다. 따라서 정상 Lap은 Segment 두 개를 만든다.
	TArray<ADroneTrainingGate*> Gates;
	for (int32 GateIndex = 0; GateIndex < 3; ++GateIndex)
	{
		ADroneTrainingGate* Gate = TestWorld->SpawnActor<ADroneTrainingGate>(
			ADroneTrainingGate::StaticClass(),
			FTransform(FVector(static_cast<float>(GateIndex) * 1000.0f, 0.0f, 0.0f)),
			SpawnParameters);
		TestNotNull(*FString::Printf(TEXT("Lap Gate %d spawns"), GateIndex), Gate);
		if (!Gate)
		{
			continue;
		}

		// SegmentDistance는 메타데이터일 뿐 아래 실제 이동 거리 계산에는 사용하지 않는다.
		Gate->ConfigureGateDefinition(
			Course->GetCourseId(),
			GateIndex,
			static_cast<float>(GateIndex) * 10000.0f);
		Gate->RerunConstructionScripts();
		Gates.Add(Gate);
	}

	TestEqual(TEXT("All three Lap Gates spawn"), Gates.Num(), 3);
	if (Gates.Num() != 3)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	Course->ConfigureOrderedGates(Gates);

	ADronePrototypePawn* Drone = TestWorld->SpawnActor<ADronePrototypePawn>(
		ADronePrototypePawn::StaticClass(),
		FTransform(FVector(-1000.0f, 5000.0f, 0.0f)),
		SpawnParameters);
	TestNotNull(TEXT("Prototype Drone spawns for Lap recording"), Drone);
	if (!Drone)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	// 모든 Actor와 Telemetry Timer를 같은 실제 Play 수명주기로 시작한다.
	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	UDroneTrainingGateSequenceComponent* Sequence = Course->GetGateSequenceComponent();
	UDroneTrainingLapRecorderComponent* Recorder = Course->GetLapRecorderComponent();
	UDroneTelemetryComponent* Telemetry = Drone->GetTelemetryComponent();
	TestNotNull(TEXT("Course owns the Gate Sequence"), Sequence);
	TestNotNull(TEXT("Course owns the Lap Recorder"), Recorder);
	TestNotNull(TEXT("Drone owns Telemetry for distance samples"), Telemetry);
	if (!Sequence || !Recorder || !Telemetry)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	TestTrue(TEXT("Three-Gate recording configuration is valid"), Sequence->IsConfigurationValid());
	TestTrue(TEXT("Lap Recorder is ready after Course BeginPlay"), Recorder->IsRecordingReady());
	TestEqual(
		TEXT("Lap Recorder starts Idle"),
		Recorder->GetRecordState(),
		EDroneTrainingLapRecordState::Idle);
	TestEqual(TEXT("Lap history starts empty"), Recorder->GetSuccessfulLapCount(), 0);

	auto ForwardEntry = [](const ADroneTrainingGate* Gate)
	{
		return Gate->GetActorLocation() - Gate->GetForwardDirectionWorld() * 300.0f;
	};
	auto ForwardExit = [](const ADroneTrainingGate* Gate)
	{
		return Gate->GetActorLocation() + Gate->GetForwardDirectionWorld() * 300.0f;
	};
	auto AcceptForward = [&](ADroneTrainingGate* Gate)
	{
		const FVector ExitLocation = ForwardExit(Gate);
		Drone->SetActorLocation(ExitLocation, false);
		return Sequence->TryAcceptTraversal(
			Gate,
			Drone,
			ForwardEntry(Gate),
			ExitLocation);
	};
	auto AdvanceWorldBy = [&](const float TotalSeconds)
	{
		// AWorldSettings는 지나치게 큰 한 Frame의 DeltaSeconds를 Clamp한다.
		// 0.1초 단위로 나누면 테스트가 요구한 World Game Time을 정확히 누적한다.
		float RemainingSeconds = TotalSeconds;
		while (RemainingSeconds > UE_KINDA_SMALL_NUMBER)
		{
			const float StepSeconds = FMath::Min(RemainingSeconds, 0.1f);
			if (!WorldWrapper.TickTestWorld(StepSeconds))
			{
				AddError(TEXT("Lap Recorder test World could not advance"));
				return false;
			}
			RemainingSeconds -= StepSeconds;
		}
		return true;
	};
	auto AdvanceAndRefreshAt = [&](const float DeltaSeconds, const FVector& Location)
	{
		if (!AdvanceWorldBy(DeltaSeconds))
		{
			return false;
		}

		Drone->SetActorLocation(Location, false);
		Telemetry->RefreshTelemetry();
		return true;
	};
	auto TestNear = [&](const TCHAR* What, const double Actual, const double Expected, const double Tolerance)
	{
		TestTrue(What, FMath::IsNearlyEqual(Actual, Expected, Tolerance));
	};

	// 정상 승인 Event가 없으면 미래 Gate와 역방향 시도는 Recorder를 시작하거나 바꾸지 않는다.
	TestEqual(
		TEXT("Future Gate is rejected before Lap start"),
		Sequence->TryAcceptTraversal(
			Gates[1],
			Drone,
			ForwardEntry(Gates[1]),
			ForwardExit(Gates[1])),
		EDroneTrainingGatePassResult::WrongOrder);
	TestEqual(
		TEXT("Reverse start Gate is rejected"),
		Sequence->TryAcceptTraversal(
			Gates[0],
			Drone,
			ForwardExit(Gates[0]),
			ForwardEntry(Gates[0])),
		EDroneTrainingGatePassResult::WrongDirection);
	TestFalse(TEXT("Rejected traversal leaves Recorder inactive"), Recorder->IsLapRecording());
	TestEqual(TEXT("Rejected traversal leaves history empty"), Recorder->GetSuccessfulLapCount(), 0);

	// Gate 0은 시간과 위치의 기준점만 만든다. Gate 0 이전 이동은 기록에 포함되지 않는다.
	TestEqual(
		TEXT("Gate 0 starts the Lap"),
		AcceptForward(Gates[0]),
		EDroneTrainingGatePassResult::Accepted);
	TestTrue(TEXT("Gate 0 changes Recorder to Recording"), Recorder->IsLapRecording());
	TestEqual(TEXT("Gate 0 creates no completed Segment"), Recorder->GetRecordedSegmentCount(), 0);
	TestNear(TEXT("Lap distance starts at zero"), Recorder->GetCurrentLapTravelDistanceMeters(), 0.0, 0.001);

	// 진행 중 미래 Gate와 이미 완료한 Gate는 Segment 경계나 거리 값을 만들지 않는다.
	const double DistanceBeforeRejectedAttempts = Recorder->GetCurrentLapTravelDistanceMeters();
	TestEqual(
		TEXT("Future finish Gate is rejected during the Lap"),
		Sequence->TryAcceptTraversal(
			Gates[2],
			Drone,
			ForwardEntry(Gates[2]),
			ForwardExit(Gates[2])),
		EDroneTrainingGatePassResult::WrongOrder);
	TestEqual(
		TEXT("Completed Gate 0 is rejected as a duplicate"),
		Sequence->TryAcceptTraversal(
			Gates[0],
			Drone,
			ForwardEntry(Gates[0]),
			ForwardExit(Gates[0])),
		EDroneTrainingGatePassResult::AlreadyCompleted);
	TestEqual(TEXT("Rejected attempts do not advance the Sequence"), Sequence->GetAcceptedGateCount(), 1);
	TestEqual(TEXT("Rejected attempts create no Segment"), Recorder->GetRecordedSegmentCount(), 0);
	TestNear(
		TEXT("Rejected attempts leave accumulated distance unchanged"),
		Recorder->GetCurrentLapTravelDistanceMeters(),
		DistanceBeforeRejectedAttempts,
		0.001);

	const double LapStartWorldSeconds = TestWorld->GetTimeSeconds();

	// 각 Segment에서 두 번 Telemetry를 갱신해 Gate 간 직선거리가 아닌 꺾인 경로를 합산한다.
	// 300-400-500 이동 두 번과 마지막 sqrt(400^2 + 800^2) 이동이 한 Segment다.
	TestTrue(TEXT("First Segment sample 1 is recorded"), AdvanceAndRefreshAt(0.5f, FVector(600.0f, 400.0f, 0.0f)));
	TestTrue(TEXT("First Segment sample 2 is recorded"), AdvanceAndRefreshAt(0.5f, FVector(900.0f, 800.0f, 0.0f)));
	TestTrue(TEXT("First Segment final time advances"), AdvanceWorldBy(1.0f));
	TestEqual(
		TEXT("Gate 1 completes the first Segment"),
		AcceptForward(Gates[1]),
		EDroneTrainingGatePassResult::Accepted);
	TestEqual(TEXT("One Segment is retained during the active Lap"), Recorder->GetRecordedSegmentCount(), 1);

	TestTrue(TEXT("Second Segment sample 1 is recorded"), AdvanceAndRefreshAt(0.5f, FVector(1600.0f, 400.0f, 0.0f)));
	TestTrue(TEXT("Second Segment sample 2 is recorded"), AdvanceAndRefreshAt(0.5f, FVector(1900.0f, 800.0f, 0.0f)));
	TestTrue(TEXT("Second Segment final time advances"), AdvanceWorldBy(1.0f));
	TestEqual(
		TEXT("Gate 2 completes the normal Lap"),
		AcceptForward(Gates[2]),
		EDroneTrainingGatePassResult::Accepted);

	const double LapEndWorldSeconds = TestWorld->GetTimeSeconds();
	const double ExpectedSegmentDistanceCentimeters =
		500.0 + 500.0 + FMath::Sqrt(400.0 * 400.0 + 800.0 * 800.0);
	const double ExpectedSegmentDistanceMeters = ExpectedSegmentDistanceCentimeters * 0.01;
	const double ExpectedLapDistanceMeters = ExpectedSegmentDistanceMeters * 2.0;
	const double ExpectedSegmentElapsedSeconds = 2.0;
	const double ExpectedLapElapsedSeconds = LapEndWorldSeconds - LapStartWorldSeconds;
	const double ExpectedSegmentSpeedKilometersPerHour =
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(
			ExpectedSegmentDistanceCentimeters,
			ExpectedSegmentElapsedSeconds);
	const double ExpectedLapSpeedKilometersPerHour =
		UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(
			ExpectedLapDistanceMeters * 100.0,
			ExpectedLapElapsedSeconds);

	TestEqual(
		TEXT("Normal Lap finishes in Completed state"),
		Recorder->GetRecordState(),
		EDroneTrainingLapRecordState::Completed);
	TestTrue(TEXT("Normal Lap is stored as successful"), Recorder->HasCompletedLap());
	TestEqual(TEXT("Exactly one successful Lap is stored"), Recorder->GetSuccessfulLapCount(), 1);
	TestFalse(TEXT("Completed Sequence cannot start another Lap before Reset"), Recorder->IsRecordingReady());

	const FDroneTrainingLapRecord CompletedLap = Recorder->GetLastCompletedLap();
	TestTrue(TEXT("Stored Lap is marked completed"), CompletedLap.bCompleted);
	TestEqual(TEXT("Three Gates produce exactly two Segments"), CompletedLap.Segments.Num(), 2);
	TestNear(TEXT("Lap uses World Game Time"), CompletedLap.ElapsedSeconds, ExpectedLapElapsedSeconds, 0.02);
	TestNear(TEXT("Normal Lap lasts four seconds"), CompletedLap.ElapsedSeconds, 4.0, 0.02);
	TestNear(TEXT("Lap sums actual Telemetry path distance"), CompletedLap.TravelDistanceMeters, ExpectedLapDistanceMeters, 0.01);
	TestNear(TEXT("Lap average speed uses total distance over total time"), CompletedLap.AverageSpeedKilometersPerHour, ExpectedLapSpeedKilometersPerHour, 0.05);

	if (CompletedLap.Segments.Num() == 2)
	{
		for (int32 SegmentIndex = 0; SegmentIndex < CompletedLap.Segments.Num(); ++SegmentIndex)
		{
			const FDroneTrainingSegmentRecord& Segment = CompletedLap.Segments[SegmentIndex];
			TestEqual(
				*FString::Printf(TEXT("Segment %d stores its array index"), SegmentIndex),
				Segment.SegmentIndex,
				SegmentIndex);
			TestEqual(
				*FString::Printf(TEXT("Segment %d starts at the previous Gate"), SegmentIndex),
				Segment.FromGateIndex,
				SegmentIndex);
			TestEqual(
				*FString::Printf(TEXT("Segment %d ends at the accepted Gate"), SegmentIndex),
				Segment.ToGateIndex,
				SegmentIndex + 1);
			TestNear(
				*FString::Printf(TEXT("Segment %d uses World Game Time"), SegmentIndex),
				Segment.ElapsedSeconds,
				ExpectedSegmentElapsedSeconds,
				0.02);
			TestNear(
				*FString::Printf(TEXT("Segment %d sums its sampled 3D path"), SegmentIndex),
				Segment.TravelDistanceMeters,
				ExpectedSegmentDistanceMeters,
				0.01);
			TestNear(
				*FString::Printf(TEXT("Segment %d converts its average speed"), SegmentIndex),
				Segment.AverageSpeedKilometersPerHour,
				ExpectedSegmentSpeedKilometersPerHour,
				0.05);
		}

		TestNear(
			TEXT("Segment times sum to the Lap time"),
			CompletedLap.Segments[0].ElapsedSeconds + CompletedLap.Segments[1].ElapsedSeconds,
			CompletedLap.ElapsedSeconds,
			0.02);
		TestNear(
			TEXT("Segment distances sum to the Lap distance"),
			CompletedLap.Segments[0].TravelDistanceMeters
				+ CompletedLap.Segments[1].TravelDistanceMeters,
			CompletedLap.TravelDistanceMeters,
			0.01);
	}

	// 성공 뒤 Restart는 완료 History를 보존하면서 새 부분 시도만 초기화한다.
	Sequence->ResetSequence();
	TestEqual(TEXT("Restart returns Sequence to Gate 0"), Sequence->GetCurrentGateIndex(), 0);
	TestEqual(
		TEXT("Restart returns Recorder to Idle"),
		Recorder->GetRecordState(),
		EDroneTrainingLapRecordState::Idle);
	TestEqual(TEXT("Restart preserves one successful Lap"), Recorder->GetSuccessfulLapCount(), 1);
	TestTrue(TEXT("Restart makes the Recorder ready at Gate 0"), Recorder->IsRecordingReady());
	TestNear(
		TEXT("Restart preserves the successful Lap value"),
		Recorder->GetLastCompletedLap().TravelDistanceMeters,
		CompletedLap.TravelDistanceMeters,
		0.001);

	TestEqual(
		TEXT("Gate 0 starts a second partial attempt"),
		AcceptForward(Gates[0]),
		EDroneTrainingGatePassResult::Accepted);
	TestTrue(TEXT("Second attempt records a distance sample"), AdvanceAndRefreshAt(0.25f, FVector(600.0f, 400.0f, 0.0f)));
	TestTrue(TEXT("Second attempt has partial distance"), Recorder->GetCurrentLapTravelDistanceMeters() > 0.0);
	Sequence->ResetSequence();
	TestFalse(TEXT("Mid-Lap Reset cancels the partial attempt"), Recorder->IsLapRecording());
	TestEqual(TEXT("Mid-Lap Reset clears partial Segments"), Recorder->GetRecordedSegmentCount(), 0);
	TestNear(TEXT("Mid-Lap Reset clears partial distance"), Recorder->GetCurrentLapTravelDistanceMeters(), 0.0, 0.001);
	TestEqual(TEXT("Mid-Lap Reset still preserves successful history"), Recorder->GetSuccessfulLapCount(), 1);

	// 활성 Drone 파괴는 Telemetry 구독과 부분 시도를 함께 정리하며 성공 기록을 만들지 않는다.
	TestEqual(
		TEXT("Gate 0 starts the destroy-cancellation attempt"),
		AcceptForward(Gates[0]),
		EDroneTrainingGatePassResult::Accepted);
	TestTrue(TEXT("Destroy-cancellation attempt becomes active"), Recorder->IsLapRecording());
	TestTrue(TEXT("Destroy-cancellation attempt samples movement"), AdvanceAndRefreshAt(0.25f, FVector(600.0f, 400.0f, 0.0f)));
	TestTrue(TEXT("Active Drone can be destroyed"), Drone->Destroy());
	WorldWrapper.TickTestWorld();
	TestEqual(
		TEXT("Pawn destruction returns Recorder to Idle"),
		Recorder->GetRecordState(),
		EDroneTrainingLapRecordState::Idle);
	TestFalse(TEXT("Pawn destruction leaves no active Lap"), Recorder->IsLapRecording());
	TestEqual(TEXT("Pawn destruction does not create another successful Lap"), Recorder->GetSuccessfulLapCount(), 1);
	TestNear(TEXT("Pawn destruction clears partial distance"), Recorder->GetCurrentLapTravelDistanceMeters(), 0.0, 0.001);

	// Course 재구성은 이전 Gate 기준의 성공 기록을 새 비교 기준에 섞지 않는다.
	Course->ConfigureOrderedGates(Gates);
	TestEqual(TEXT("Course reconfiguration clears incompatible successful history"), Recorder->GetSuccessfulLapCount(), 0);
	TestTrue(TEXT("Valid reconfiguration returns the Recorder to Gate 0 ready state"), Recorder->IsRecordingReady());

	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
