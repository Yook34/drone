#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Telemetry/DroneTelemetryTypes.h"
#include "Tutorial/DroneTrainingRecordTypes.h"
#include "DroneTrainingLapRecorderComponent.generated.h"

class ADroneTrainingGate;
class UDroneTelemetryComponent;
class UDroneTrainingGateSequenceComponent;

/**
 * 정상 Gate 승인 Event를 구독해 Segment/Lap의 시간, 실제 이동 거리, 평균 속도를 기록한다.
 *
 * Gate Sequence는 순서 판정만 유지한다. 이 Component는 Gate 0 승인 뒤 같은 Drone의
 * 기존 Telemetry 10Hz Event에서 World 위치를 표본화하므로 별도 Tick이나 Timer가 없다.
 * 비교, Best, 점수, 결과 UI, SaveGame은 TUT-04 이후 책임이다.
 */
UCLASS(ClassGroup=(Drone), BlueprintType, meta=(BlueprintSpawnableComponent))
class UDroneTrainingLapRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneTrainingLapRecorderComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Course가 소유한 Sequence 한 곳을 기록 입력원으로 연결한다. 같은 Source 재연결은 중복 구독하지 않는다. */
	void InitializeRecorder(UDroneTrainingGateSequenceComponent* InGateSequence);

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	EDroneTrainingLapRecordState GetRecordState() const { return RecordState; }

	/** 유효한 Gate가 두 개 이상이고 Gate 0 시작 전일 때만 true다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	bool IsRecordingReady() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	bool IsLapRecording() const { return RecordState == EDroneTrainingLapRecordState::Recording; }

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	bool HasCompletedLap() const { return !SuccessfulLaps.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	int32 GetSuccessfulLapCount() const { return SuccessfulLaps.Num(); }

	/** 현재 실행 중 완료된 원본 기록이다. 이전 평균과 Best 계산은 TUT-04에서 이 배열을 사용한다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	TArray<FDroneTrainingLapRecord> GetSuccessfulLaps() const { return SuccessfulLaps; }

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	FDroneTrainingLapRecord GetLastCompletedLap() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	double GetCurrentLapElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	double GetCurrentSegmentElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	double GetCurrentLapTravelDistanceMeters() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	double GetCurrentSegmentTravelDistanceMeters() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	int32 GetRecordedSegmentCount() const { return CurrentSegments.Num(); }

	/** 진행 중 이미 확정된 구간 원본. HUD는 Event를 놓친 뒤 연결돼도 이 값으로 복구한다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Recording")
	TArray<FDroneTrainingSegmentRecord> GetCurrentSegments() const { return CurrentSegments; }

	/** 거리(cm)와 시간(s)을 안전하게 km/h로 바꾼다. 0/음수/비정상 입력은 0이다. */
	static double CalculateAverageSpeedKilometersPerHour(
		double DistanceCentimeters,
		double ElapsedSeconds);

	/** Gate 0 정상 승인 직후 발생한다. TUT-04 Blueprint UI가 이 Event를 구독할 수 있다. */
	UPROPERTY(BlueprintAssignable, Category="Tutorial|Recording")
	FDroneTrainingLapStartedSignature OnLapStarted;

	/** Gate 1 이후 정상 Gate마다 완성된 원본 Segment를 전달한다. */
	UPROPERTY(BlueprintAssignable, Category="Tutorial|Recording")
	FDroneTrainingSegmentRecordedSignature OnSegmentRecorded;

	/** 마지막 Gate 승인 뒤 완성된 원본 Lap을 전달한다. */
	UPROPERTY(BlueprintAssignable, Category="Tutorial|Recording")
	FDroneTrainingLapCompletedSignature OnLapCompleted;

private:
	UFUNCTION()
	void HandleGateAccepted(
		ADroneTrainingGate* Gate,
		AActor* PassingActor,
		int32 AcceptedGateCount,
		FVector AcceptedWorldLocation);

	UFUNCTION()
	void HandleSequenceReset();

	UFUNCTION()
	void HandleSequenceReconfigured();

	UFUNCTION()
	void HandleTelemetryUpdated(FDroneTelemetrySnapshot Snapshot);

	UFUNCTION()
	void HandleActiveDroneDestroyed(AActor* DestroyedActor);

	void StartLap(AActor* PassingActor, int32 StartGateIndex, const FVector& StartWorldLocation, double StartTimeSeconds);
	void AddLocationSample(const FVector& WorldLocation);
	bool FinalizeSegment(int32 ToGateIndex, double EndTimeSeconds);
	void CompleteLap(double EndTimeSeconds);
	void CancelCurrentAttempt();
	void UnbindActiveDrone();
	void UnbindGateSequence();
	bool IsRecordingConfigurationValid() const;
	double GetWorldGameTimeSeconds() const;

	UPROPERTY(Transient)
	TObjectPtr<UDroneTrainingGateSequenceComponent> GateSequence;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ActiveDrone;

	UPROPERTY(Transient)
	TObjectPtr<UDroneTelemetryComponent> ActiveTelemetry;

	UPROPERTY(Transient)
	EDroneTrainingLapRecordState RecordState = EDroneTrainingLapRecordState::Idle;

	UPROPERTY(Transient)
	TArray<FDroneTrainingSegmentRecord> CurrentSegments;

	/** 성공 기록만 실행 중 보존한다. Reset은 부분 시도만 버리고 이 배열은 유지한다. */
	UPROPERTY(Transient)
	TArray<FDroneTrainingLapRecord> SuccessfulLaps;

	double LapStartTimeSeconds = 0.0;
	double SegmentStartTimeSeconds = 0.0;
	double LapDistanceCentimeters = 0.0;
	double SegmentDistanceCentimeters = 0.0;
	int32 LastAcceptedGateIndex = INDEX_NONE;
	FVector LastSampleWorldLocation = FVector::ZeroVector;
	bool bHasLocationSample = false;
};
