#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypeGameMode.h"
#include "Prototype/DronePrototypePawn.h"
#include "Tutorial/DroneTrainingCourse.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

namespace DroneTrainingAssets
{
constexpr const TCHAR* CourseClassPath =
	TEXT("/Game/Drone/Tutorial/Blueprints/BP_DroneTrainingCourse.BP_DroneTrainingCourse_C");
constexpr const TCHAR* GameModeClassPath =
	TEXT("/Game/Drone/Prototype/Blueprints/BP_DronePrototypeGameMode.BP_DronePrototypeGameMode_C");
constexpr const TCHAR* TrainingMapObjectPath =
	TEXT("/Game/Drone/Tutorial/Maps/Lvl_DroneTraining.Lvl_DroneTraining");
constexpr const TCHAR* GuideMaterialObjectPath =
	TEXT("/Game/Drone/Tutorial/Materials/M_DroneTrainingGuide.M_DroneTrainingGuide");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingAssetTest,
	"Drone.Tutorial.TrainingAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingAssetTest::RunTest(const FString& Parameters)
{
	// 1) 실제 BP가 native 안전 규칙을 상속하는지 확인한다.
	UClass* CourseClass = LoadClass<ADroneTrainingCourse>(nullptr, DroneTrainingAssets::CourseClassPath);
	TestNotNull(TEXT("BP_DroneTrainingCourse generated Class loads"), CourseClass);
	if (CourseClass)
	{
		TestTrue(
			TEXT("BP_DroneTrainingCourse derives from the native Training Course"),
			CourseClass->IsChildOf(ADroneTrainingCourse::StaticClass()));
	}

	UClass* GameModeClass = LoadClass<ADronePrototypeGameMode>(nullptr, DroneTrainingAssets::GameModeClassPath);
	TestNotNull(TEXT("Prototype BP GameMode used by the Training Map loads"), GameModeClass);

	UMaterialInterface* GuideMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		DroneTrainingAssets::GuideMaterialObjectPath);
	TestNotNull(TEXT("M_DroneTrainingGuide Material loads"), GuideMaterial);
	if (GuideMaterial)
	{
		const UMaterial* BaseMaterial = GuideMaterial->GetMaterial();
		TestNotNull(TEXT("Guide Material has a base Material"), BaseMaterial);
		TestTrue(
			TEXT("Guide Material compiles SplineMesh shaders"),
			BaseMaterial && BaseMaterial->GetUsageByFlag(MATUSAGE_SplineMesh));
		TestTrue(
			TEXT("Guide Material is Unlit"),
			BaseMaterial && BaseMaterial->GetShadingModels().HasShadingModel(MSM_Unlit));
		TestEqual(TEXT("Guide Material is opaque"), GuideMaterial->GetBlendMode(), BLEND_Opaque);
	}

	// 2) 정확한 Map 경로와 WorldSettings 계약을 검사해 전역 ThirdPerson 설정 의존을 막는다.
	UWorld* TrainingWorld = LoadObject<UWorld>(nullptr, DroneTrainingAssets::TrainingMapObjectPath);
	TestNotNull(TEXT("Lvl_DroneTraining loads from the Tutorial Map path"), TrainingWorld);
	if (!TrainingWorld)
	{
		return false;
	}

	const AWorldSettings* WorldSettings = TrainingWorld->GetWorldSettings();
	TestNotNull(TEXT("Training Map has WorldSettings"), WorldSettings);
	if (WorldSettings && GameModeClass)
	{
		TestEqual(
			TEXT("Training Map overrides GameMode with BP_DronePrototypeGameMode"),
			WorldSettings->DefaultGameMode.Get(),
			GameModeClass);
	}

	int32 PlayerStartCount = 0;
	int32 PlacedPrototypePawnCount = 0;
	int32 CourseCount = 0;
	ADroneTrainingCourse* PlacedCourse = nullptr;
	for (TActorIterator<AActor> ActorIt(TrainingWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor)
		{
			continue;
		}

		PlayerStartCount += Actor->IsA<APlayerStart>() ? 1 : 0;
		PlacedPrototypePawnCount += Actor->IsA<ADronePrototypePawn>() ? 1 : 0;
		if (Actor->IsA<ADroneTrainingCourse>())
		{
			++CourseCount;
			PlacedCourse = Cast<ADroneTrainingCourse>(Actor);
		}

		// 새 Map에 기존 Template/Variant Actor를 직접 배치하는 회귀를 잡는다.
		const FString ActorClassPath = Actor->GetClass()->GetPathName();
		TestFalse(
			*FString::Printf(TEXT("%s is not a placed ThirdPerson/Variant Actor"), *Actor->GetName()),
			ActorClassPath.StartsWith(TEXT("/Game/ThirdPerson"))
				|| ActorClassPath.StartsWith(TEXT("/Game/Variant_")));
	}

	TestEqual(TEXT("Training Map has exactly one PlayerStart"), PlayerStartCount, 1);
	TestEqual(TEXT("Training Map has no pre-placed Prototype Pawn"), PlacedPrototypePawnCount, 0);
	TestEqual(TEXT("Training Map has exactly one Training Course"), CourseCount, 1);
	TestNotNull(TEXT("Training Map contains a Training Course instance"), PlacedCourse);

	if (PlacedCourse)
	{
		TestTrue(TEXT("Placed Course uses BP_DroneTrainingCourse"), PlacedCourse->GetClass() == CourseClass);
		TestTrue(
			TEXT("Placed Course uses M_DroneTrainingGuide"),
			PlacedCourse->GetCourseLineMaterial() == GuideMaterial);

		// 로드된 Map Actor에서도 Construction 결과를 재현한 뒤 전체 Primitive 안전 계약을 검사한다.
		PlacedCourse->RerunConstructionScripts();
		const USplineComponent* CourseSpline = PlacedCourse->GetCourseSpline();
		TestNotNull(TEXT("Placed Course owns its Spline"), CourseSpline);
		const int32 ExpectedSegmentCount = CourseSpline
			? FMath::Max(0, CourseSpline->GetNumberOfSplinePoints() - 1)
			: 0;
		TestTrue(TEXT("Placed Course Spline has a positive length"), CourseSpline && CourseSpline->GetSplineLength() > 0.0f);
		TestEqual(
			TEXT("Placed Course creates every runtime guide Segment"),
			PlacedCourse->GetCourseLineSegmentCount(),
			ExpectedSegmentCount);

		TInlineComponentArray<UPrimitiveComponent*> CoursePrimitives;
		PlacedCourse->GetComponents(CoursePrimitives);
		for (UPrimitiveComponent* Primitive : CoursePrimitives)
		{
			if (!Primitive)
			{
				continue;
			}

			TestEqual(
				*FString::Printf(TEXT("Map component %s has no collision"), *Primitive->GetName()),
				Primitive->GetCollisionEnabled(),
				ECollisionEnabled::NoCollision);
			TestFalse(
				*FString::Printf(TEXT("Map component %s creates no overlaps"), *Primitive->GetName()),
				Primitive->GetGenerateOverlapEvents());
			TestFalse(
				*FString::Printf(TEXT("Map component %s cannot affect Navigation"), *Primitive->GetName()),
				Primitive->CanEverAffectNavigation());

			if (const USplineMeshComponent* Segment = Cast<USplineMeshComponent>(Primitive);
				Segment && Segment->ComponentHasTag(ADroneTrainingCourse::GetGeneratedSegmentTag()))
			{
				TestNotNull(TEXT("Map guide Segment has a runtime Mesh"), Segment->GetStaticMesh().Get());
				const UMaterialInterface* SegmentMaterial = Segment->GetMaterial(0);
				TestNotNull(TEXT("Map guide Segment has a Material"), SegmentMaterial);
				TestTrue(
					TEXT("Map guide Segment resolves to M_DroneTrainingGuide"),
					SegmentMaterial && SegmentMaterial->GetMaterial() == GuideMaterial);
				TestTrue(TEXT("Map guide Segment is runtime visible"), Segment->IsVisible() && !Segment->bHiddenInGame);
			}
		}
	}

	return !HasAnyErrors();
}

#endif
