#include "Tutorial/DroneTrainingLapRecorderComponent.h"

#include "Prototype/DronePrototypePawn.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "Tutorial/DroneTrainingGate.h"
#include "Tutorial/DroneTrainingGateSequenceComponent.h"

namespace DroneTrainingLapRecorder
{
constexpr double CentimetersToMeters = 0.01;
constexpr double MetersPerSecondToKilometersPerHour = 3.6;
}

UDroneTrainingLapRecorderComponent::UDroneTrainingLapRecorderComponent()
{
	// 실제 이동 위치는 기존 Telemetry 10Hz Event가 공급하므로 별도 Tick을 만들지 않는다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneTrainingLapRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelCurrentAttempt();
	UnbindGateSequence();
	Super::EndPlay(EndPlayReason);
}

void UDroneTrainingLapRecorderComponent::InitializeRecorder(
	UDroneTrainingGateSequenceComponent* InGateSequence)
{
	if (GateSequence == InGateSequence)
	{
		if (GateSequence)
		{
			GateSequence->OnGateAccepted.AddUniqueDynamic(
				this,
				&UDroneTrainingLapRecorderComponent::HandleGateAccepted);
			GateSequence->OnSequenceReset.AddUniqueDynamic(
				this,
				&UDroneTrainingLapRecorderComponent::HandleSequenceReset);
			GateSequence->OnSequenceReconfigured.AddUniqueDynamic(
				this,
				&UDroneTrainingLapRecorderComponent::HandleSequenceReconfigured);
		}
		return;
	}

	CancelCurrentAttempt();
	UnbindGateSequence();
	// 다른 Sequence의 Lap은 같은 비교 History로 사용할 수 없다.
	SuccessfulLaps.Reset();
	GateSequence = InGateSequence;
	if (GateSequence)
	{
		GateSequence->OnGateAccepted.AddUniqueDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleGateAccepted);
		GateSequence->OnSequenceReset.AddUniqueDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleSequenceReset);
		GateSequence->OnSequenceReconfigured.AddUniqueDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleSequenceReconfigured);
	}
}

bool UDroneTrainingLapRecorderComponent::IsRecordingReady() const
{
	return RecordState == EDroneTrainingLapRecordState::Idle
		&& IsRecordingConfigurationValid()
		&& GateSequence->GetAcceptedGateCount() == 0;
}

FDroneTrainingLapRecord UDroneTrainingLapRecorderComponent::GetLastCompletedLap() const
{
	return SuccessfulLaps.IsEmpty()
		? FDroneTrainingLapRecord()
		: SuccessfulLaps.Last();
}

double UDroneTrainingLapRecorderComponent::GetCurrentLapElapsedSeconds() const
{
	return IsLapRecording()
		? FMath::Max(0.0, GetWorldGameTimeSeconds() - LapStartTimeSeconds)
		: 0.0;
}

double UDroneTrainingLapRecorderComponent::GetCurrentSegmentElapsedSeconds() const
{
	return IsLapRecording()
		? FMath::Max(0.0, GetWorldGameTimeSeconds() - SegmentStartTimeSeconds)
		: 0.0;
}

double UDroneTrainingLapRecorderComponent::GetCurrentLapTravelDistanceMeters() const
{
	return IsLapRecording()
		? LapDistanceCentimeters * DroneTrainingLapRecorder::CentimetersToMeters
		: 0.0;
}

double UDroneTrainingLapRecorderComponent::GetCurrentSegmentTravelDistanceMeters() const
{
	return IsLapRecording()
		? SegmentDistanceCentimeters * DroneTrainingLapRecorder::CentimetersToMeters
		: 0.0;
}

double UDroneTrainingLapRecorderComponent::CalculateAverageSpeedKilometersPerHour(
	const double DistanceCentimeters,
	const double ElapsedSeconds)
{
	if (!FMath::IsFinite(DistanceCentimeters)
		|| !FMath::IsFinite(ElapsedSeconds)
		|| DistanceCentimeters <= 0.0
		|| ElapsedSeconds <= UE_DOUBLE_SMALL_NUMBER)
	{
		return 0.0;
	}

	const double DistanceMeters = DistanceCentimeters * DroneTrainingLapRecorder::CentimetersToMeters;
	const double SpeedKilometersPerHour = DistanceMeters / ElapsedSeconds
		* DroneTrainingLapRecorder::MetersPerSecondToKilometersPerHour;
	return FMath::IsFinite(SpeedKilometersPerHour)
		? SpeedKilometersPerHour
		: 0.0;
}

void UDroneTrainingLapRecorderComponent::HandleGateAccepted(
	ADroneTrainingGate* Gate,
	AActor* PassingActor,
	const int32 AcceptedGateCount,
	const FVector AcceptedWorldLocation)
{
	if (!IsValid(Gate) || !IsValid(PassingActor) || !IsRecordingConfigurationValid())
	{
		CancelCurrentAttempt();
		return;
	}

	const double AcceptedTimeSeconds = GetWorldGameTimeSeconds();
	const int32 AcceptedGatePosition = AcceptedGateCount - 1;
	const TArray<TObjectPtr<ADroneTrainingGate>>& OrderedGates = GateSequence->GetOrderedGates();
	if (GateSequence->GetAcceptedGateCount() != AcceptedGateCount
		|| !OrderedGates.IsValidIndex(AcceptedGatePosition)
		|| OrderedGates[AcceptedGatePosition] != Gate)
	{
		// 다른 Delegate가 Reset/Reconfigure한 뒤 도착한 오래된 승인 Event는 사용하지 않는다.
		CancelCurrentAttempt();
		return;
	}

	if (AcceptedGateCount == 1)
	{
		StartLap(PassingActor, Gate->GetGateIndex(), AcceptedWorldLocation, AcceptedTimeSeconds);
		return;
	}

	// 한 Lap은 Gate 0을 통과한 같은 Drone만 이어 쓴다. 멀티플레이 규칙은 현재 범위가 아니다.
	if (!IsLapRecording() || ActiveDrone.Get() != PassingActor)
	{
		CancelCurrentAttempt();
		return;
	}

	AddLocationSample(AcceptedWorldLocation);
	if (!FinalizeSegment(Gate->GetGateIndex(), AcceptedTimeSeconds))
	{
		return;
	}

	if (GateSequence && GateSequence->IsSequenceComplete())
	{
		CompleteLap(AcceptedTimeSeconds);
	}
}

void UDroneTrainingLapRecorderComponent::HandleSequenceReset()
{
	// Restart/구성 무효화는 실패 Event를 만들지 않고 현재 부분 시도만 폐기한다.
	CancelCurrentAttempt();
}

void UDroneTrainingLapRecorderComponent::HandleSequenceReconfigured()
{
	// Gate 배열이 바뀌면 예전 Course 기준 기록을 TUT-04 비교 원본으로 섞지 않는다.
	CancelCurrentAttempt();
	SuccessfulLaps.Reset();
}

void UDroneTrainingLapRecorderComponent::HandleTelemetryUpdated(FDroneTelemetrySnapshot Snapshot)
{
	(void)Snapshot;

	if (!IsLapRecording()
		|| !IsRecordingConfigurationValid()
		|| !ActiveDrone.IsValid())
	{
		CancelCurrentAttempt();
		return;
	}

	AddLocationSample(ActiveDrone->GetActorLocation());
}

void UDroneTrainingLapRecorderComponent::HandleActiveDroneDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == ActiveDrone.Get())
	{
		CancelCurrentAttempt();
	}
}

void UDroneTrainingLapRecorderComponent::StartLap(
	AActor* PassingActor,
	const int32 StartGateIndex,
	const FVector& StartWorldLocation,
	const double StartTimeSeconds)
{
	CancelCurrentAttempt();

	ADronePrototypePawn* PrototypeDrone = Cast<ADronePrototypePawn>(PassingActor);
	UDroneTelemetryComponent* Telemetry = PrototypeDrone
		? PrototypeDrone->GetTelemetryComponent()
		: nullptr;
	if (!IsValid(PrototypeDrone) || !IsValid(Telemetry) || StartWorldLocation.ContainsNaN())
	{
		return;
	}

	ActiveDrone = PrototypeDrone;
	ActiveTelemetry = Telemetry;
	ActiveTelemetry->OnTelemetryUpdated.AddUniqueDynamic(
		this,
		&UDroneTrainingLapRecorderComponent::HandleTelemetryUpdated);
	PrototypeDrone->OnDestroyed.AddUniqueDynamic(
		this,
		&UDroneTrainingLapRecorderComponent::HandleActiveDroneDestroyed);

	RecordState = EDroneTrainingLapRecordState::Recording;
	LapStartTimeSeconds = StartTimeSeconds;
	SegmentStartTimeSeconds = StartTimeSeconds;
	LapDistanceCentimeters = 0.0;
	SegmentDistanceCentimeters = 0.0;
	LastAcceptedGateIndex = StartGateIndex;
	LastSampleWorldLocation = StartWorldLocation;
	bHasLocationSample = true;
	CurrentSegments.Reset();
	OnLapStarted.Broadcast();
}

void UDroneTrainingLapRecorderComponent::AddLocationSample(const FVector& WorldLocation)
{
	if (!IsLapRecording() || WorldLocation.ContainsNaN())
	{
		return;
	}

	if (!bHasLocationSample)
	{
		LastSampleWorldLocation = WorldLocation;
		bHasLocationSample = true;
		return;
	}

	const double SampleDistanceCentimeters = FVector::Distance(LastSampleWorldLocation, WorldLocation);
	if (FMath::IsFinite(SampleDistanceCentimeters) && SampleDistanceCentimeters > 0.0)
	{
		LapDistanceCentimeters += SampleDistanceCentimeters;
		SegmentDistanceCentimeters += SampleDistanceCentimeters;
	}
	LastSampleWorldLocation = WorldLocation;
}

bool UDroneTrainingLapRecorderComponent::FinalizeSegment(
	const int32 ToGateIndex,
	const double EndTimeSeconds)
{
	const double SegmentElapsedSeconds = EndTimeSeconds - SegmentStartTimeSeconds;
	if (!FMath::IsFinite(SegmentElapsedSeconds)
		|| SegmentElapsedSeconds <= UE_DOUBLE_SMALL_NUMBER)
	{
		// 같은 Frame 순간이동은 유효한 비행 기록으로 저장하지 않는다.
		CancelCurrentAttempt();
		return false;
	}

	FDroneTrainingSegmentRecord SegmentRecord;
	SegmentRecord.SegmentIndex = CurrentSegments.Num();
	SegmentRecord.FromGateIndex = LastAcceptedGateIndex;
	SegmentRecord.ToGateIndex = ToGateIndex;
	SegmentRecord.ElapsedSeconds = SegmentElapsedSeconds;
	SegmentRecord.TravelDistanceMeters = SegmentDistanceCentimeters
		* DroneTrainingLapRecorder::CentimetersToMeters;
	SegmentRecord.AverageSpeedKilometersPerHour = CalculateAverageSpeedKilometersPerHour(
		SegmentDistanceCentimeters,
		SegmentElapsedSeconds);

	CurrentSegments.Add(SegmentRecord);
	SegmentStartTimeSeconds = EndTimeSeconds;
	SegmentDistanceCentimeters = 0.0;
	LastAcceptedGateIndex = ToGateIndex;
	// 구독자가 Current getter를 읽어도 이미 다음 Segment 기준을 보도록 commit 뒤 Broadcast한다.
	OnSegmentRecorded.Broadcast(SegmentRecord);
	return true;
}

void UDroneTrainingLapRecorderComponent::CompleteLap(const double EndTimeSeconds)
{
	const double LapElapsedSeconds = EndTimeSeconds - LapStartTimeSeconds;
	const int32 ExpectedSegmentCount = GateSequence
		? GateSequence->GetConfiguredGateCount() - 1
		: 0;
	if (!FMath::IsFinite(LapElapsedSeconds)
		|| LapElapsedSeconds <= UE_DOUBLE_SMALL_NUMBER
		|| CurrentSegments.Num() != ExpectedSegmentCount)
	{
		CancelCurrentAttempt();
		return;
	}

	FDroneTrainingLapRecord LapRecord;
	LapRecord.bCompleted = true;
	LapRecord.ElapsedSeconds = LapElapsedSeconds;
	LapRecord.TravelDistanceMeters = LapDistanceCentimeters
		* DroneTrainingLapRecorder::CentimetersToMeters;
	LapRecord.AverageSpeedKilometersPerHour = CalculateAverageSpeedKilometersPerHour(
		LapDistanceCentimeters,
		LapRecord.ElapsedSeconds);
	LapRecord.Segments = CurrentSegments;
	SuccessfulLaps.Add(LapRecord);

	UnbindActiveDrone();
	RecordState = EDroneTrainingLapRecordState::Completed;
	LapStartTimeSeconds = 0.0;
	SegmentStartTimeSeconds = 0.0;
	LapDistanceCentimeters = 0.0;
	SegmentDistanceCentimeters = 0.0;
	LastAcceptedGateIndex = INDEX_NONE;
	LastSampleWorldLocation = FVector::ZeroVector;
	bHasLocationSample = false;
	CurrentSegments.Reset();
	// 성공 History와 완료 상태를 모두 commit한 뒤 Blueprint에 불변 Record를 전달한다.
	OnLapCompleted.Broadcast(LapRecord);
}

void UDroneTrainingLapRecorderComponent::CancelCurrentAttempt()
{
	UnbindActiveDrone();
	RecordState = EDroneTrainingLapRecordState::Idle;
	LapStartTimeSeconds = 0.0;
	SegmentStartTimeSeconds = 0.0;
	LapDistanceCentimeters = 0.0;
	SegmentDistanceCentimeters = 0.0;
	LastAcceptedGateIndex = INDEX_NONE;
	LastSampleWorldLocation = FVector::ZeroVector;
	bHasLocationSample = false;
	CurrentSegments.Reset();
}

void UDroneTrainingLapRecorderComponent::UnbindActiveDrone()
{
	if (ActiveTelemetry)
	{
		ActiveTelemetry->OnTelemetryUpdated.RemoveDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleTelemetryUpdated);
	}

	if (AActor* Drone = ActiveDrone.Get())
	{
		Drone->OnDestroyed.RemoveDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleActiveDroneDestroyed);
	}

	ActiveTelemetry = nullptr;
	ActiveDrone.Reset();
}

void UDroneTrainingLapRecorderComponent::UnbindGateSequence()
{
	if (GateSequence)
	{
		GateSequence->OnGateAccepted.RemoveDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleGateAccepted);
		GateSequence->OnSequenceReset.RemoveDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleSequenceReset);
		GateSequence->OnSequenceReconfigured.RemoveDynamic(
			this,
			&UDroneTrainingLapRecorderComponent::HandleSequenceReconfigured);
	}
	GateSequence = nullptr;
}

bool UDroneTrainingLapRecorderComponent::IsRecordingConfigurationValid() const
{
	return IsValid(GateSequence)
		&& GateSequence->IsConfigurationValid()
		&& GateSequence->GetConfiguredGateCount() >= 2;
}

double UDroneTrainingLapRecorderComponent::GetWorldGameTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}
