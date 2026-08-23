#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneTrainingCourse.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

/**
 * Tutorial 비행 경로의 위치와 표시만 담당하는 Course Actor.
 *
 * TUT-01에서는 편집 가능한 Spline과 화면에 보이는 안내선까지만 제공한다.
 * Gate Trigger, 통과 순서, Lap/Segment 기록은 다음 카드에서 별도 Actor와
 * 상태 객체로 추가해 이 표시 Actor가 게임 규칙까지 떠맡지 않게 한다.
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

	/** 자동화와 후속 Gate 배치가 같은 이름 계약을 사용할 수 있게 공개한다. */
	static FName GetGeneratedSegmentTag();

protected:
	/** Course Actor의 고정 기준점. 경로 자체는 이동하지 않으므로 Tick이 없다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Course", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> CourseRoot;

	/** Designer가 점과 Tangent를 편집하는 경로 데이터. 이 Component 자체도 충돌하지 않는다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Course", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineComponent> CourseSpline;

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

	/** OnConstruction/BeginPlay에서 같은 안전 규칙으로 Segment를 재생성한다. */
	void RebuildCourseLineSegments();

	/** 표시 Material을 한 번 만들고 모든 Segment가 공유하게 한다. */
	UMaterialInterface* CreateCourseLineMaterial();

	/** Construction 재실행 때 이전 임시 MID가 남지 않도록 GC 추적만 유지한다. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicCourseLineMaterial;
};
