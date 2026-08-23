#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypePawn.h"
#include "Tests/AutomationCommon.h"
#include "Tutorial/DroneTrainingCourse.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingCourseTest,
	"Drone.Tutorial.TrainingCourse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingCourseTest::RunTest(const FString& Parameters)
{
	// 1) CDO 검사는 실수로 Course Tick이나 Actor Collision을 켜는 회귀를 빠르게 잡는다.
	const ADroneTrainingCourse* CourseDefaults = GetDefault<ADroneTrainingCourse>();
	TestNotNull(TEXT("Training Course CDO exists"), CourseDefaults);
	if (CourseDefaults)
	{
		TestFalse(TEXT("Training Course class is concrete"), CourseDefaults->GetClass()->HasAnyClassFlags(CLASS_Abstract));
		TestFalse(TEXT("Training Course avoids per-frame ticking"), CourseDefaults->PrimaryActorTick.bCanEverTick);
		TestFalse(TEXT("Training Course Actor collision is disabled"), CourseDefaults->GetActorEnableCollision());

		const USplineComponent* DefaultSpline = CourseDefaults->GetCourseSpline();
		TestNotNull(TEXT("Training Course owns an editable Spline"), DefaultSpline);
		if (DefaultSpline)
		{
			TestTrue(TEXT("Default Spline has at least two points"), DefaultSpline->GetNumberOfSplinePoints() >= 2);
			TestTrue(TEXT("Default Spline has a positive length"), DefaultSpline->GetSplineLength() > UE_SMALL_NUMBER);
			TestEqual(TEXT("Spline collision is disabled"), DefaultSpline->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestFalse(TEXT("Spline overlap generation is disabled"), DefaultSpline->GetGenerateOverlapEvents());
			TestFalse(TEXT("Spline cannot affect Navigation"), DefaultSpline->CanEverAffectNavigation());
		}
	}

	// 2) 실제 World에 Spawn해야 OnConstruction이 만든 SplineMesh와 물리 질의를 함께 검사할 수 있다.
	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	UWorld* TestWorld = WorldWrapper.GetTestWorld();
	TestNotNull(TEXT("Transient Course test world exists"), TestWorld);
	if (!TestWorld)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADroneTrainingCourse* Course = TestWorld->SpawnActor<ADroneTrainingCourse>(
		ADroneTrainingCourse::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	TestNotNull(TEXT("Training Course spawns in a runtime World"), Course);
	if (!Course)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	const int32 ExpectedSegmentCount = Course->GetCourseSpline()
		? FMath::Max(0, Course->GetCourseSpline()->GetNumberOfSplinePoints() - 1)
		: 0;
	TestEqual(
		TEXT("Construction creates one visible guide Segment between each Spline point"),
		Course->GetCourseLineSegmentCount(),
		ExpectedSegmentCount);

	// 모든 Primitive를 포괄 검사해 재구성 중 남은 옛 Segment도 안전 계약을 우회하지 못하게 한다.
	TInlineComponentArray<UPrimitiveComponent*> CoursePrimitives;
	Course->GetComponents(CoursePrimitives);
	int32 GeneratedSegmentCount = 0;
	for (UPrimitiveComponent* Primitive : CoursePrimitives)
	{
		if (!Primitive)
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s has no collision"), *Primitive->GetName()),
			Primitive->GetCollisionEnabled(),
			ECollisionEnabled::NoCollision);
		TestFalse(
			*FString::Printf(TEXT("%s creates no overlap events"), *Primitive->GetName()),
			Primitive->GetGenerateOverlapEvents());
		TestFalse(
			*FString::Printf(TEXT("%s does not simulate physics"), *Primitive->GetName()),
			Primitive->IsSimulatingPhysics());
		TestFalse(
			*FString::Printf(TEXT("%s cannot affect Navigation"), *Primitive->GetName()),
			Primitive->CanEverAffectNavigation());

		if (USplineMeshComponent* Segment = Cast<USplineMeshComponent>(Primitive);
			Segment && Segment->ComponentHasTag(ADroneTrainingCourse::GetGeneratedSegmentTag()))
		{
			++GeneratedSegmentCount;
			TestNotNull(TEXT("Generated guide Segment has a visible Static Mesh"), Segment->GetStaticMesh().Get());
			TestTrue(TEXT("Generated guide Segment is visible"), Segment->IsVisible());
			TestFalse(TEXT("Generated guide Segment is not hidden in game"), Segment->bHiddenInGame);
		}
	}
	TestEqual(TEXT("Primitive scan finds every generated guide Segment"), GeneratedSegmentCount, ExpectedSegmentCount);

	// 3) BP/Level에서 안전 값을 잘못 바꿔도 Construction이 복원하며 Segment를 누적하지 않아야 한다.
	Course->SetActorEnableCollision(true);
	if (USplineComponent* MutableSpline = Course->GetCourseSpline())
	{
		MutableSpline->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MutableSpline->SetGenerateOverlapEvents(true);
		MutableSpline->SetCanEverAffectNavigation(true);
	}
	Course->RerunConstructionScripts();
	Course->RerunConstructionScripts();
	TestFalse(TEXT("Construction restores disabled Actor collision"), Course->GetActorEnableCollision());
	if (const USplineComponent* RestoredSpline = Course->GetCourseSpline())
	{
		TestEqual(
			TEXT("Construction restores disabled Spline collision"),
			RestoredSpline->GetCollisionEnabled(),
			ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Construction restores disabled Spline overlaps"), RestoredSpline->GetGenerateOverlapEvents());
		TestFalse(TEXT("Construction restores disabled Spline Navigation impact"), RestoredSpline->CanEverAffectNavigation());
	}
	TestEqual(
		TEXT("Repeated Construction does not duplicate guide Segments"),
		Course->GetCourseLineSegmentCount(),
		ExpectedSegmentCount);

	// 4) Drone Collision 크기로 안내선을 가로질러 Sweep해 실제 Blocking Hit가 없음을 확인한다.
	ADronePrototypePawn* Drone = TestWorld->SpawnActor<ADronePrototypePawn>(
		ADronePrototypePawn::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	TestNotNull(TEXT("Prototype Drone spawns for non-interference sweep"), Drone);
	if (Drone && Course->GetCourseSpline())
	{
		const float ProbeDistance = Course->GetCourseSpline()->GetSplineLength() * 0.12f;
		const FVector ProbeCenter = Course->GetCourseSpline()->GetLocationAtDistanceAlongSpline(
			ProbeDistance,
			ESplineCoordinateSpace::World);
		const FVector ProbeDirection = Course->GetCourseSpline()->GetDirectionAtDistanceAlongSpline(
			ProbeDistance,
			ESplineCoordinateSpace::World);
		FVector CrossDirection = FVector::CrossProduct(FVector::UpVector, ProbeDirection).GetSafeNormal();
		if (CrossDirection.IsNearlyZero())
		{
			CrossDirection = FVector::RightVector;
		}

		const FVector SweepStart = ProbeCenter - CrossDirection * 250.0f;
		const FVector SweepEnd = ProbeCenter + CrossDirection * 250.0f;
		Drone->SetActorLocation(SweepStart, false);

		FHitResult SweepHit;
		Drone->SetActorLocation(SweepEnd, true, &SweepHit);
		TestFalse(TEXT("Drone sweep across the guide line has no Blocking Hit"), SweepHit.bBlockingHit);
		TestTrue(
			TEXT("Drone reaches the other side of the guide line"),
			Drone->GetActorLocation().Equals(SweepEnd, 1.0f));
		TestTrue(
			TEXT("Guide line never reports itself as a sweep obstacle"),
			SweepHit.GetActor() != Course);
	}

	WorldWrapper.TickTestWorld();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
