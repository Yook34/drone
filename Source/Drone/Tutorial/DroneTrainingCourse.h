#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneTrainingCourse.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UDroneTrainingGateSequenceComponent;
class USceneComponent;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class ADroneTrainingGate;

/**
 * Tutorial 비행 경로 표시와 명시적 Gate 구성 묶음을 담당하는 Course Actor.
 *
 * TUT-01의 편집 가능한 Spline과 안내선을 유지한다. TUT-02에서는 CourseId와
 * 명시적 Gate 배열이라는 구성 데이터만 추가한다. Gate Trigger는 별도 Actor,
 * 순서·방향 상태는 비-Primitive Component가 맡아 표시 Actor와 판정을 분리한다.
 *
 * 안내선은 Drone이 실제로 통과하는 공간에 놓이므로 Actor, Spline,
 * 생성되는 모든 SplineMesh의 Collision·Overlap·Physics·Navigation 영향을
 * 명시적으로 끈다. 외형 Mesh와 색은 Blueprint 자식에서 바꿀 수 있지만
 * 비간섭 설정은 C++이 매번 다시 적용한다.
 */
UCLASS(Blueprintable)
class ADroneTrainingCourse : public AActor
{
	GENERATED_BODY()

public:
	ADroneTrainingCourse();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Level 또는 BP Viewport에서 점을 움직여 초기 Greybox 경로를 편집한다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Course")
	USplineComponent* GetCourseSpline() const { return CourseSpline; }

	/** 현재 Spline 점 사이에 생성된 표시 Segment 수다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Course")
	int32 GetCourseLineSegmentCount() const;

	/** 현재 표시 Segment에 적용할 Material. 기본값은 프로젝트 전용 Unlit 발광 재질이다. */
	UFUNCTION(BlueprintPure, Category="Tutorial|Course")
	UMaterialInterface* GetCourseLineMaterial() const { return CourseLineMaterial; }

	UFUNCTION(BlueprintPure, Category="Tutorial|Course|Gates")
	FName GetCourseId() const { return CourseId; }

	UFUNCTION(BlueprintPure, Category="Tutorial|Course|Gates")
	UDroneTrainingGateSequenceComponent* GetGateSequenceComponent() const { return GateSequenceComponent; }

	const TArray<TObjectPtr<ADroneTrainingGate>>& GetOrderedGates() const { return OrderedGates; }

	/** 자동화나 후속 Editor 도구가 명시적 배열을 설정한 뒤 같은 검증 경로를 사용한다. */
	void ConfigureOrderedGates(const TArray<ADroneTrainingGate*>& InOrderedGates);

	/** 자동화와 후속 Gate 배치가 같은 이름 계약을 사용할 수 있게 공개한다. */
	static FName GetGeneratedSegmentTag();

protected:
	/** Course Actor의 고정 기준점. 경로 자체는 이동하지 않으므로 Tick이 없다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Course", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> CourseRoot;

	/** Designer가 점과 Tangent를 편집하는 경로 데이터. 이 Component 자체도 충돌하지 않는다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Course", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineComponent> CourseSpline;

	/** Gate 진행 상태는 Primitive가 아닌 이 Component 한 곳에서만 바뀐다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Gates", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDroneTrainingGateSequenceComponent> GateSequenceComponent;

	/** Gate와 Course가 같은 구성에 속하는지 확인하는 명시적 식별자다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|Course|Gates", meta=(AllowPrivateAccess="true"))
	FName CourseId = TEXT("DroneTrainingCourse");

	/** 배열 위치가 통과 순서의 단일 기준이며 각 GateIndex는 같은 위치를 미러링해야 한다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|Course|Gates", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<ADroneTrainingGate>> OrderedGates;

	/**
	 * SplineMesh에 사용할 임시 Greybox Mesh.
	 * 현재 기본값은 Engine Cube이며 구매 에셋이나 최종 코스 외형으로 확정된 것이 아니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMesh> CourseLineMesh;

	/** 기본값은 프로젝트의 발광 Material이며 BP에서 다른 표시 Material로 교체할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMaterialInterface> CourseLineMaterial;

	/** 안내선 폭. Drone Collision보다 얇게 보여 주기 위한 초기 Greybox 값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual",
		meta=(ClampMin="1.0", UIMin="1.0", Units="cm", AllowPrivateAccess="true"))
	float CourseLineWidthCentimeters = 32.0f;

	/** 안내선 두께. 지면이나 환경 Mesh와 Z-fighting이 생기지 않도록 별도로 둔다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual",
		meta=(ClampMin="1.0", UIMin="1.0", Units="cm", AllowPrivateAccess="true"))
	float CourseLineThicknessCentimeters = 10.0f;

	/** Spline 점보다 안내선을 위로 띄우는 로컬 Z Offset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual",
		meta=(Units="cm", AllowPrivateAccess="true"))
	float CourseLineVerticalOffsetCentimeters = 0.0f;

	/** 기본 발광 Material의 Color Parameter에 적용되는 초기 안내색이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Course|Visual", meta=(AllowPrivateAccess="true"))
	FLinearColor CourseLineColor = FLinearColor(0.02f, 0.70f, 1.0f, 1.0f);

private:
	/** BP/Level에서 값을 잘못 바꿔도 Construction과 BeginPlay에서 비간섭 계약을 복원한다. */
	void ApplyNonInterferenceRules();

	/** 구성 데이터와 비-Primitive Sequence 상태를 연결하고 Gate 외형을 초기화한다. */
	void ConfigureGateSequence();

	/** OnConstruction/BeginPlay에서 같은 안전 규칙으로 Segment를 재생성한다. */
	void RebuildCourseLineSegments();

	/** 표시 Material을 한 번 만들고 모든 Segment가 공유하게 한다. */
	UMaterialInterface* CreateCourseLineMaterial();

	/** Construction 재실행 때 이전 임시 MID가 남지 않도록 GC 추적만 유지한다. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicCourseLineMaterial;
};
