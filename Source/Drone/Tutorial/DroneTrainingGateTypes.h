#pragma once

#include "CoreMinimal.h"
#include "DroneTrainingGateTypes.generated.h"

class ADroneTrainingGate;
class AActor;

/** Ring Gate가 현재 Course 진행에서 어떤 의미로 보이는지 나타낸다. */
UENUM(BlueprintType)
enum class EDroneTrainingGateVisualState : uint8
{
	Inactive UMETA(DisplayName="Inactive"),
	Current UMETA(DisplayName="Current"),
	Completed UMETA(DisplayName="Completed")
};

/** Gate 통과 시도가 진행 상태를 바꾸었는지, 바꾸지 않았다면 그 이유가 무엇인지 나타낸다. */
UENUM(BlueprintType)
enum class EDroneTrainingGatePassResult : uint8
{
	Accepted UMETA(DisplayName="Accepted"),
	InvalidActor UMETA(DisplayName="Invalid Actor"),
	InvalidConfiguration UMETA(DisplayName="Invalid Configuration"),
	GateNotInSequence UMETA(DisplayName="Gate Not In Sequence"),
	WrongOrder UMETA(DisplayName="Wrong Order"),
	WrongDirection UMETA(DisplayName="Wrong Direction"),
	AlreadyCompleted UMETA(DisplayName="Already Completed")
};

/** 정상 통과 한 번마다 실제 통과 Actor와 승인 위치를 함께 전달한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FDroneTrainingGateAcceptedSignature,
	ADroneTrainingGate*, Gate,
	AActor*, PassingActor,
	int32, AcceptedGateCount,
	FVector, AcceptedWorldLocation);

/** Restart 또는 런타임 구성 무효화로 현재 진행을 더 이어 갈 수 없을 때 발생한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDroneTrainingGateSequenceResetSignature);

/** Gate 배열 또는 Course 구성이 다시 적용되어 기록 비교 기준도 새로 시작할 때 발생한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDroneTrainingGateSequenceReconfiguredSignature);
