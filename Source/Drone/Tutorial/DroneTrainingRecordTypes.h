#pragma once

#include "CoreMinimal.h"
#include "DroneTrainingRecordTypes.generated.h"

/** Tutorial Course 기록기가 현재 한 Lap을 어떤 상태로 다루는지 나타낸다. */
UENUM(BlueprintType)
enum class EDroneTrainingLapRecordState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Recording UMETA(DisplayName="Recording"),
	Completed UMETA(DisplayName="Completed")
};

/** 직전 정상 Gate부터 현재 정상 Gate까지의 원본 기록이다. */
USTRUCT(BlueprintType)
struct FDroneTrainingSegmentRecord
{
	GENERATED_BODY()

	/** 0부터 시작하는 Segment 배열 위치다. Gate 0은 출발선이므로 첫 Segment는 0 -> 1이다. */
	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording")
	int32 SegmentIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording")
	int32 FromGateIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording")
	int32 ToGateIndex = INDEX_NONE;

	/** World Game Time 기준 구간 시간. Pause와 Time Dilation을 그대로 따른다. */
	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="s"))
	double ElapsedSeconds = 0.0;

	/** Drone의 3차원 World 위치 표본 사이 거리를 합산한 값이다. */
	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="m"))
	double TravelDistanceMeters = 0.0;

	/** TravelDistanceMeters / ElapsedSeconds를 km/h로 변환한 값이다. */
	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="km/h"))
	double AverageSpeedKilometersPerHour = 0.0;
};

/** Gate 0 승인부터 마지막 Gate 승인까지의 성공 Lap 원본 기록이다. */
USTRUCT(BlueprintType)
struct FDroneTrainingLapRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="s"))
	double ElapsedSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="m"))
	double TravelDistanceMeters = 0.0;

	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording", meta=(Units="km/h"))
	double AverageSpeedKilometersPerHour = 0.0;

	/** Gate가 N개라면 Gate 0을 출발선으로 사용하므로 Segment는 N-1개다. */
	UPROPERTY(BlueprintReadOnly, Category="Tutorial|Recording")
	TArray<FDroneTrainingSegmentRecord> Segments;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDroneTrainingLapStartedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FDroneTrainingSegmentRecordedSignature,
	FDroneTrainingSegmentRecord, SegmentRecord);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FDroneTrainingLapCompletedSignature,
	FDroneTrainingLapRecord, LapRecord);
