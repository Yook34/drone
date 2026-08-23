#pragma once

#include "CoreMinimal.h"
#include "DroneTrainingGateTypes.generated.h"

class ADroneTrainingGate;

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

/** 정상 통과 한 번마다 발생한다. Lap과 Timing 계산은 이 Event를 후속 카드에서 구독한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FDroneTrainingGateAcceptedSignature,
	ADroneTrainingGate*, Gate,
	int32, AcceptedGateCount);
