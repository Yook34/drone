#include "Tutorial/DroneTrainingCourse.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Tutorial/DroneTrainingGate.h"
#include "Tutorial/DroneTrainingGateSequenceComponent.h"
#include "Tutorial/DroneTrainingLapRecorderComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace DroneTrainingCourse
{
const FName GeneratedSegmentTag(TEXT("DroneTrainingCourse.GeneratedLineSegment"));

// Spline 점을 지나치게 많이 만들었을 때 Editor가 멈추는 실수를 방지하는 안전 상한이다.
constexpr int32 MaximumGeneratedSegmentCount = 256;
}

ADroneTrainingCourse::ADroneTrainingCourse()
{
	// Course는 정적인 경로 데이터이므로 매 Frame Tick하지 않는다.
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);

	CourseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CourseRoot"));
	CourseRoot->SetMobility(EComponentMobility::Static);
	CourseRoot->SetCanEverAffectNavigation(false);
	SetRootComponent(CourseRoot);

	CourseSpline = CreateDefaultSubobject<USplineComponent>(TEXT("CourseSpline"));
	CourseSpline->SetupAttachment(CourseRoot);
	CourseSpline->SetMobility(EComponentMobility::Static);
	CourseSpline->SetClosedLoop(false);
	CourseSpline->SetDrawDebug(true);
	CourseSpline->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	CourseSpline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CourseSpline->SetGenerateOverlapEvents(false);
	CourseSpline->SetCanEverAffectNavigation(false);

	// Gate 순서 상태는 Primitive가 아니므로 Course의 비간섭 규칙과 충돌하지 않는다.
	GateSequenceComponent = CreateDefaultSubobject<UDroneTrainingGateSequenceComponent>(TEXT("GateSequenceComponent"));
	LapRecorderComponent = CreateDefaultSubobject<UDroneTrainingLapRecorderComponent>(TEXT("LapRecorderComponent"));

	// 처음 Map에 배치하자마자 비행 가능한 S자형 Greybox 경로가 보이게 한다.
	// 이 좌표는 최종 코스가 아니며 Level/BP Viewport에서 자유롭게 수정한다.
	const TArray<FVector> InitialCoursePoints = {
		FVector(0.0f, 0.0f, 250.0f),
		FVector(1200.0f, 0.0f, 350.0f),
		FVector(2400.0f, 700.0f, 500.0f),
		FVector(3600.0f, -700.0f, 400.0f),
		FVector(5000.0f, 0.0f, 300.0f)
	};
	CourseSpline->SetSplinePoints(InitialCoursePoints, ESplineCoordinateSpace::Local, true);

	// 구매 에셋 전에도 재현할 수 있도록 표시 Mesh는 Engine 기본 Cube를 사용한다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CourseLineMesh = CubeMeshFinder.Object;
	}

	// SplineMesh 사용 Flag가 저장된 프로젝트 전용 Unlit 발광 재질을 사용한다.
	// Flag가 없는 일반 Material은 런타임에서 World 기본 Material로 교체되므로 경로를 임의로 바꾸지 않는다.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GuideMaterialFinder(
		TEXT("/Game/Drone/Tutorial/Materials/M_DroneTrainingGuide.M_DroneTrainingGuide"));
	if (GuideMaterialFinder.Succeeded())
	{
		CourseLineMaterial = GuideMaterialFinder.Object;
	}
}

void ADroneTrainingCourse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyNonInterferenceRules();
	RebuildCourseLineSegments();
	ConfigureGateSequence();
}

void ADroneTrainingCourse::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// BeginPlay의 BP Event나 초기 Overlap보다 먼저 정상 Gate Event를 받을 준비를 끝낸다.
	if (GetWorld() && GetWorld()->IsGameWorld() && LapRecorderComponent)
	{
		LapRecorderComponent->InitializeRecorder(GateSequenceComponent);
	}
}

void ADroneTrainingCourse::BeginPlay()
{
	Super::BeginPlay();

	// BP/Level 직렬화 값이나 Cook 방식이 달라도 런타임 비간섭 계약을 다시 고정한다.
	ApplyNonInterferenceRules();
	RebuildCourseLineSegments();
	ConfigureGateSequence();
}

void ADroneTrainingCourse::ConfigureOrderedGates(const TArray<ADroneTrainingGate*>& InOrderedGates)
{
	OrderedGates.Reset(InOrderedGates.Num());
	for (ADroneTrainingGate* Gate : InOrderedGates)
	{
		OrderedGates.Add(Gate);
	}

	ConfigureGateSequence();
}

int32 ADroneTrainingCourse::GetCourseLineSegmentCount() const
{
	TInlineComponentArray<USplineMeshComponent*> SplineMeshComponents;
	GetComponents(SplineMeshComponents);

	int32 GeneratedSegmentCount = 0;
	for (const USplineMeshComponent* SplineMeshComponent : SplineMeshComponents)
	{
		if (SplineMeshComponent
			&& SplineMeshComponent->ComponentHasTag(DroneTrainingCourse::GeneratedSegmentTag))
		{
			++GeneratedSegmentCount;
		}
	}

	return GeneratedSegmentCount;
}

FName ADroneTrainingCourse::GetGeneratedSegmentTag()
{
	return DroneTrainingCourse::GeneratedSegmentTag;
}

void ADroneTrainingCourse::ApplyNonInterferenceRules()
{
	// 생성자 뒤에 BP CDO와 Level Instance 값이 역직렬화될 수 있으므로 매 Construction/Play 때 복원한다.
	SetActorEnableCollision(false);
	if (CourseRoot)
	{
		CourseRoot->SetCanEverAffectNavigation(false);
	}

	// Course에 나중에 표시용 Primitive가 추가되더라도 모두 같은 안전 계약을 따르게 한다.
	TInlineComponentArray<UPrimitiveComponent*> CoursePrimitives;
	GetComponents(CoursePrimitives);
	for (UPrimitiveComponent* Primitive : CoursePrimitives)
	{
		if (!Primitive)
		{
			continue;
		}

		Primitive->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Primitive->SetGenerateOverlapEvents(false);
		Primitive->SetSimulatePhysics(false);
		Primitive->SetCanEverAffectNavigation(false);
	}
}

void ADroneTrainingCourse::ConfigureGateSequence()
{
	if (GateSequenceComponent)
	{
		GateSequenceComponent->ConfigureSequence(CourseId, OrderedGates);
	}
}

void ADroneTrainingCourse::RebuildCourseLineSegments()
{
	// Construction을 여러 번 실행해도 옛 Segment가 겹쳐 남지 않도록 Tag 기준으로 먼저 제거한다.
	TInlineComponentArray<USplineMeshComponent*> ExistingSplineMeshes;
	GetComponents(ExistingSplineMeshes);
	for (USplineMeshComponent* ExistingSplineMesh : ExistingSplineMeshes)
	{
		if (ExistingSplineMesh
			&& ExistingSplineMesh->ComponentHasTag(DroneTrainingCourse::GeneratedSegmentTag))
		{
			BlueprintCreatedComponents.Remove(ExistingSplineMesh);
			ExistingSplineMesh->DestroyComponent();
		}
	}

	DynamicCourseLineMaterial = nullptr;

	if (!CourseSpline || !CourseLineMesh)
	{
		return;
	}

	// 열린 Spline은 점 N개 사이에 N-1개의 표시 Segment가 필요하다.
	const int32 SplinePointCount = CourseSpline->GetNumberOfSplinePoints();
	const int32 RequestedSegmentCount = FMath::Max(0, SplinePointCount - 1);
	const int32 SegmentCount = FMath::Min(RequestedSegmentCount, DroneTrainingCourse::MaximumGeneratedSegmentCount);
	if (SegmentCount == 0)
	{
		return;
	}

	UMaterialInterface* MaterialToUse = CreateCourseLineMaterial();
	const FVector MeshSize = CourseLineMesh->GetBounds().BoxExtent * 2.0f;
	const float MeshWidth = FMath::Max(MeshSize.Y, 1.0f);
	const float MeshThickness = FMath::Max(MeshSize.Z, 1.0f);
	const FVector2D CourseLineScale(
		FMath::Max(CourseLineWidthCentimeters, 1.0f) / MeshWidth,
		FMath::Max(CourseLineThicknessCentimeters, 1.0f) / MeshThickness);

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const FName SegmentName = MakeUniqueObjectName(
			this,
			USplineMeshComponent::StaticClass(),
			*FString::Printf(TEXT("CourseLineSegment_%02d"), SegmentIndex));
		USplineMeshComponent* CourseLineSegment = NewObject<USplineMeshComponent>(
			this,
			SegmentName,
			RF_Transactional);
		if (!CourseLineSegment)
		{
			continue;
		}

		// UserConstructionScript 방식으로 등록하면 Map 저장과 Construction 재실행 수명주기를 따른다.
		PostCreateBlueprintComponent(CourseLineSegment);
		CourseLineSegment->ComponentTags.Add(DroneTrainingCourse::GeneratedSegmentTag);
		CourseLineSegment->SetupAttachment(CourseSpline);
		CourseLineSegment->SetMobility(EComponentMobility::Static);
		CourseLineSegment->SetStaticMesh(CourseLineMesh);
		CourseLineSegment->SetForwardAxis(ESplineMeshAxis::X, false);
		CourseLineSegment->SetSplineUpDir(FVector::UpVector, false);
		CourseLineSegment->SetStartScale(CourseLineScale, false);
		CourseLineSegment->SetEndScale(CourseLineScale, false);

		// 표시선은 비행 판정 대상이 아니다. 네 항목을 Segment마다 강제로 고정한다.
		CourseLineSegment->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		CourseLineSegment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CourseLineSegment->SetGenerateOverlapEvents(false);
		CourseLineSegment->SetSimulatePhysics(false);
		CourseLineSegment->SetCanEverAffectNavigation(false);
		CourseLineSegment->SetbNeverNeedsCookedCollisionData(true);
		CourseLineSegment->SetCastShadow(false);
		CourseLineSegment->SetReceivesDecals(false);
		CourseLineSegment->SetVisibility(true);
		CourseLineSegment->SetHiddenInGame(false);

		if (MaterialToUse)
		{
			CourseLineSegment->SetMaterial(0, MaterialToUse);
		}

		FVector StartPosition;
		FVector StartTangent;
		CourseSpline->GetLocationAndTangentAtSplinePoint(
			SegmentIndex,
			StartPosition,
			StartTangent,
			ESplineCoordinateSpace::Local);

		FVector EndPosition;
		FVector EndTangent;
		CourseSpline->GetLocationAndTangentAtSplinePoint(
			SegmentIndex + 1,
			EndPosition,
			EndTangent,
			ESplineCoordinateSpace::Local);

		const FVector VerticalOffset(0.0f, 0.0f, CourseLineVerticalOffsetCentimeters);
		CourseLineSegment->SetStartAndEnd(
			StartPosition + VerticalOffset,
			StartTangent,
			EndPosition + VerticalOffset,
			EndTangent,
			false);
		CourseLineSegment->RegisterComponent();
		CourseLineSegment->UpdateMesh();
	}
}

UMaterialInterface* ADroneTrainingCourse::CreateCourseLineMaterial()
{
	if (!CourseLineMaterial)
	{
		return nullptr;
	}

	// M_DroneTrainingGuide의 Color Parameter를 인스턴스마다 바꿔 BP 노출 색을 반영한다.
	DynamicCourseLineMaterial = UMaterialInstanceDynamic::Create(CourseLineMaterial, this);
	if (DynamicCourseLineMaterial)
	{
		DynamicCourseLineMaterial->SetVectorParameterValue(TEXT("Color"), CourseLineColor);
		return DynamicCourseLineMaterial;
	}

	return CourseLineMaterial;
}
