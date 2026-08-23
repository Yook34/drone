#include "Tutorial/DroneTrainingGateSequenceComponent.h"

#include "Prototype/DronePrototypePawn.h"
#include "Tutorial/DroneTrainingGate.h"

namespace DroneTrainingGateSequence
{
// 위치가 사실상 같은 두 점이나 Gate 평면 한쪽에서 끝난 이동은 통과로 취급하지 않는다.
constexpr float MinimumTravelDistanceCentimeters = 1.0f;
constexpr float MinimumPlaneSideDistanceCentimeters = 1.0f;
}

UDroneTrainingGateSequenceComponent::UDroneTrainingGateSequenceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneTrainingGateSequenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromConfiguredGates();
	bConfigurationValid = false;
	Super::EndPlay(EndPlayReason);
}

bool UDroneTrainingGateSequenceComponent::ConfigureSequence(
	const FName InCourseId,
	const TArray<TObjectPtr<ADroneTrainingGate>>& InOrderedGates)
{
	// 이전 Gate가 이 Sequence를 계속 가리키지 않도록 먼저 관계를 끊는다.
	DetachFromConfiguredGates();

	CourseId = InCourseId;
	OrderedGates = InOrderedGates;
	NextExpectedGatePosition = 0;
	bConfigurationValid = ValidateConfiguration(InCourseId);

	for (ADroneTrainingGate* Gate : OrderedGates)
	{
		if (IsValid(Gate))
		{
			Gate->AssignGateSequence(bConfigurationValid ? this : nullptr);
		}
	}

	RefreshGateVisualStates();
	return bConfigurationValid;
}

void UDroneTrainingGateSequenceComponent::ResetSequence()
{
	NextExpectedGatePosition = 0;
	for (ADroneTrainingGate* Gate : OrderedGates)
	{
		if (IsValid(Gate))
		{
			Gate->CancelPendingTraversals();
		}
	}

	if (bConfigurationValid && !IsConfigurationValid())
	{
		bConfigurationValid = false;
		DetachFromConfiguredGates();
	}
	RefreshGateVisualStates();
}

EDroneTrainingGatePassResult UDroneTrainingGateSequenceComponent::TryAcceptTraversal(
	ADroneTrainingGate* Gate,
	AActor* PassingActor,
	const FVector& EntryWorldLocation,
	const FVector& ExitWorldLocation)
{
	// 구성 뒤 Gate가 파괴되거나 교체된 경우에도 오래된 상태로 진행하지 않는다.
	if (!IsConfigurationValid())
	{
		bConfigurationValid = false;
		DetachFromConfiguredGates();
		RefreshGateVisualStates();
		return EDroneTrainingGatePassResult::InvalidConfiguration;
	}

	// 현재 Vertical Slice에서 유효 Drone 계약은 ADronePrototypePawn/BP 자식이다.
	if (!IsValid(PassingActor) || !PassingActor->IsA<ADronePrototypePawn>())
	{
		return EDroneTrainingGatePassResult::InvalidActor;
	}

	const int32 SequencePosition = OrderedGates.IndexOfByKey(Gate);
	if (SequencePosition == INDEX_NONE || !Gate || Gate->GetAssignedGateSequence() != this)
	{
		return EDroneTrainingGatePassResult::GateNotInSequence;
	}

	if (SequencePosition < NextExpectedGatePosition)
	{
		return EDroneTrainingGatePassResult::AlreadyCompleted;
	}

	if (SequencePosition != NextExpectedGatePosition)
	{
		return EDroneTrainingGatePassResult::WrongOrder;
	}

	if (!IsForwardTraversal(Gate, EntryWorldLocation, ExitWorldLocation))
	{
		return EDroneTrainingGatePassResult::WrongDirection;
	}

	++NextExpectedGatePosition;
	RefreshGateVisualStates();
	OnGateAccepted.Broadcast(Gate, NextExpectedGatePosition);
	return EDroneTrainingGatePassResult::Accepted;
}

ADroneTrainingGate* UDroneTrainingGateSequenceComponent::GetCurrentGate() const
{
	if (!IsConfigurationValid() || !OrderedGates.IsValidIndex(NextExpectedGatePosition))
	{
		return nullptr;
	}

	ADroneTrainingGate* CurrentGate = OrderedGates[NextExpectedGatePosition].Get();
	return IsValid(CurrentGate) && CurrentGate->GetAssignedGateSequence() == this
		? CurrentGate
		: nullptr;
}

int32 UDroneTrainingGateSequenceComponent::GetCurrentGateIndex() const
{
	const ADroneTrainingGate* CurrentGate = GetCurrentGate();
	return CurrentGate ? CurrentGate->GetGateIndex() : INDEX_NONE;
}

bool UDroneTrainingGateSequenceComponent::IsSequenceComplete() const
{
	return IsConfigurationValid()
		&& !OrderedGates.IsEmpty()
		&& NextExpectedGatePosition >= OrderedGates.Num();
}

bool UDroneTrainingGateSequenceComponent::IsConfigurationValid() const
{
	if (!bConfigurationValid || !ValidateConfiguration(CourseId))
	{
		return false;
	}

	for (const ADroneTrainingGate* Gate : OrderedGates)
	{
		if (!IsValid(Gate) || Gate->GetAssignedGateSequence() != this)
		{
			return false;
		}
	}

	return true;
}

bool UDroneTrainingGateSequenceComponent::ValidateConfiguration(const FName InCourseId) const
{
	if (InCourseId.IsNone() || OrderedGates.IsEmpty())
	{
		return false;
	}

	TSet<const ADroneTrainingGate*> UniqueGates;
	for (int32 GatePosition = 0; GatePosition < OrderedGates.Num(); ++GatePosition)
	{
		const ADroneTrainingGate* Gate = OrderedGates[GatePosition];
		if (!IsValid(Gate)
			|| UniqueGates.Contains(Gate)
			|| Gate->GetCourseId() != InCourseId
			|| Gate->GetGateIndex() != GatePosition)
		{
			return false;
		}

		UniqueGates.Add(Gate);
	}

	return true;
}

void UDroneTrainingGateSequenceComponent::DetachFromConfiguredGates()
{
	for (ADroneTrainingGate* Gate : OrderedGates)
	{
		if (IsValid(Gate) && Gate->GetAssignedGateSequence() == this)
		{
			Gate->AssignGateSequence(nullptr);
		}
	}
}

void UDroneTrainingGateSequenceComponent::RefreshGateVisualStates()
{
	for (int32 GatePosition = 0; GatePosition < OrderedGates.Num(); ++GatePosition)
	{
		ADroneTrainingGate* Gate = OrderedGates[GatePosition];
		if (!IsValid(Gate))
		{
			continue;
		}

		EDroneTrainingGateVisualState NewState = EDroneTrainingGateVisualState::Inactive;
		if (bConfigurationValid && GatePosition < NextExpectedGatePosition)
		{
			NewState = EDroneTrainingGateVisualState::Completed;
		}
		else if (bConfigurationValid && GatePosition == NextExpectedGatePosition)
		{
			NewState = EDroneTrainingGateVisualState::Current;
		}

		Gate->SetGateVisualState(NewState);
	}
}

bool UDroneTrainingGateSequenceComponent::IsForwardTraversal(
	const ADroneTrainingGate* Gate,
	const FVector& EntryWorldLocation,
	const FVector& ExitWorldLocation) const
{
	if (!Gate)
	{
		return false;
	}

	const FVector Travel = ExitWorldLocation - EntryWorldLocation;
	if (Travel.SizeSquared() < FMath::Square(DroneTrainingGateSequence::MinimumTravelDistanceCentimeters))
	{
		return false;
	}

	const FVector ForwardDirection = Gate->GetForwardDirectionWorld();
	if (ForwardDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector GateLocation = Gate->GetActorLocation();
	const FVector GateScale = Gate->GetActorScale3D().GetAbs();
	if (GateScale.X <= UE_SMALL_NUMBER || GateScale.Y <= UE_SMALL_NUMBER || GateScale.Z <= UE_SMALL_NUMBER)
	{
		return false;
	}

	const float EntrySide = FVector::DotProduct(EntryWorldLocation - GateLocation, ForwardDirection);
	const float ExitSide = FVector::DotProduct(ExitWorldLocation - GateLocation, ForwardDirection);
	if (EntrySide >= -DroneTrainingGateSequence::MinimumPlaneSideDistanceCentimeters
		|| ExitSide <= DroneTrainingGateSequence::MinimumPlaneSideDistanceCentimeters)
	{
		return false;
	}

	const float SideDelta = ExitSide - EntrySide;
	if (SideDelta <= UE_SMALL_NUMBER)
	{
		return false;
	}

	// Box는 Overlap 수집용이므로 모서리가 원 바깥으로 튀어나온다. 실제 승인 시에는
	// 이동 선분과 Gate 평면의 교차점이 원형 aperture 안에 있는지 다시 검사한다.
	const float CrossingAlpha = -EntrySide / SideDelta;
	if (!FMath::IsWithinInclusive(CrossingAlpha, 0.0f, 1.0f))
	{
		return false;
	}

	const FVector CrossingPoint = FMath::Lerp(EntryWorldLocation, ExitWorldLocation, CrossingAlpha);
	// Actor Scale을 포함한 역변환 뒤 로컬 YZ 반경을 비교해야 화면 Ring/Trigger와 판정 크기가 같다.
	const FVector CrossingPointLocal = Gate->GetActorTransform().InverseTransformPosition(CrossingPoint);
	const float LocalRadialDistanceSquared = FMath::Square(CrossingPointLocal.Y)
		+ FMath::Square(CrossingPointLocal.Z);
	return LocalRadialDistanceSquared
		<= FMath::Square(Gate->GetTriggerApertureRadiusCentimeters());
}
