#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tutorial/DroneTrainingGateTypes.h"
#include "DroneTrainingGateSequenceComponent.generated.h"

class ADroneTrainingGate;

/**
 * Training Course의 순서형 Gate 진행 상태만 담당하는 비-Primitive Component.
 *
 * Course Actor는 Spline/표시선과 Gate 구성 목록을 보관하고, Gate Actor는 Overlap을 감지한다.
 * 이 Component는 그 사이에서 현재 Gate, 순서, 정방향, 중복 통과만 판정한다.
 * TUT-02에는 Lap, 시간, 평균 속도, SaveGame 책임을 넣지 않는다.
 */
UCLASS(ClassGroup=(Drone), meta=(BlueprintSpawnableComponent))
class UDroneTrainingGateSequenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneTrainingGateSequenceComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Course가 보관한 명시적 배열을 단일 순서 기준으로 사용한다. */
	bool ConfigureSequence(
		FName InCourseId,
		const TArray<TObjectPtr<ADroneTrainingGate>>& InOrderedGates);

	/** 진행 상태만 Gate 0으로 되돌린다. Lap/Timing 데이터는 아직 존재하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="Tutorial|Gate Sequence")
	void ResetSequence();

	/** Gate가 전달한 진입/이탈 위치를 검증하고 정상일 때만 정확히 한 칸 진행한다. */
	EDroneTrainingGatePassResult TryAcceptTraversal(
		ADroneTrainingGate* Gate,
		AActor* PassingActor,
		const FVector& EntryWorldLocation,
		const FVector& ExitWorldLocation);

	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	bool IsConfigurationValid() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	int32 GetConfiguredGateCount() const { return OrderedGates.Num(); }

	/** 정상 통과한 Gate 수이자 다음 Gate의 배열 위치다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	int32 GetAcceptedGateCount() const { return NextExpectedGatePosition; }

	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	ADroneTrainingGate* GetCurrentGate() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	int32 GetCurrentGateIndex() const;

	UFUNCTION(BlueprintPure, Category="Tutorial|Gate Sequence")
	bool IsSequenceComplete() const;

	const TArray<TObjectPtr<ADroneTrainingGate>>& GetOrderedGates() const { return OrderedGates; }

	/** TUT-03 이후 기록 계층이 정상 Gate 통과만 구독할 수 있는 경계 Event다. */
	UPROPERTY(BlueprintAssignable, Category="Tutorial|Gate Sequence")
	FDroneTrainingGateAcceptedSignature OnGateAccepted;

private:
	/** 기존 Gate가 이 Component를 계속 참조하지 않도록 약한 역참조를 정리한다. */
	void DetachFromConfiguredGates();

	/** 구성 오류 시 모든 Gate를 비활성화하고 진행을 막는다. */
	bool ValidateConfiguration(FName InCourseId) const;

	/** Current/Completed/Inactive 외형을 현재 진행 위치에서 다시 계산한다. */
	void RefreshGateVisualStates();

	/** Gate의 로컬 Forward와 실제 진입/이탈 위치로 정방향 관통인지 검사한다. */
	bool IsForwardTraversal(
		const ADroneTrainingGate* Gate,
		const FVector& EntryWorldLocation,
		const FVector& ExitWorldLocation) const;

	UPROPERTY(Transient)
	FName CourseId = NAME_None;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ADroneTrainingGate>> OrderedGates;

	UPROPERTY(Transient)
	int32 NextExpectedGatePosition = 0;

	UPROPERTY(Transient)
	bool bConfigurationValid = false;
};
