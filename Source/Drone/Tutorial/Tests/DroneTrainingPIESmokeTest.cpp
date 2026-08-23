#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypePawn.h"
#include "Prototype/DronePrototypePlayerController.h"
#include "Tutorial/DroneTrainingCourse.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/DroneFlightHUDWidget.h"

namespace DroneTrainingPIESmoke
{
constexpr const TCHAR* MapPackage = TEXT("/Game/Drone/Tutorial/Maps/Lvl_DroneTraining");
constexpr const TCHAR* CourseClassPath =
	TEXT("/Game/Drone/Tutorial/Blueprints/BP_DroneTrainingCourse.BP_DroneTrainingCourse_C");
constexpr const TCHAR* PawnClassPath =
	TEXT("/Game/Drone/Prototype/Blueprints/BP_DronePrototypePawn.BP_DronePrototypePawn_C");
constexpr const TCHAR* ControllerClassPath =
	TEXT("/Game/Drone/Prototype/Blueprints/BP_DronePrototypePlayerController.BP_DronePrototypePlayerController_C");
constexpr const TCHAR* FlightHUDClassPath =
	TEXT("/Game/Drone/Prototype/UI/WBP_DroneFlightHUD.WBP_DroneFlightHUD_C");

UWorld* FindPIEWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			return Context.World();
		}
	}

	return nullptr;
}

FRequestPlaySessionParams MakePlayParams()
{
	// 실제 BP GameMode/Pawn/Controller/WBP 경로를 새 1인 PIE World에서 검증한다.
	ULevelEditorPlaySettings* Settings = NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
	Settings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(1);
	Settings->bLaunchSeparateServer = false;
	Settings->AddToRoot(); // FStartPIEForAutomationCommand가 시작 뒤 Root에서 제거한다.

	FRequestPlaySessionParams Params;
	Params.SessionDestination = EPlaySessionDestinationType::InProcess;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.EditorPlaySettings = Settings;
	Params.bAllowOnlineSubsystem = false;
	return Params;
}

class FValidateTrainingPIECommand final : public IAutomationLatentCommand
{
public:
	explicit FValidateTrainingPIECommand(FAutomationTestBase* InTest)
		: Test(InTest)
	{
	}

	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (StartedAt == 0.0)
		{
			StartedAt = Now;
		}

		UWorld* PIEWorld = FindPIEWorld();
		if (!PIEWorld || !PIEWorld->HasBegunPlay())
		{
			if (Now - StartedAt > 20.0)
			{
				Test->AddError(TEXT("Training PIE World did not begin play within 20 seconds"));
				return true;
			}
			return false;
		}

		UClass* CourseClass = LoadClass<ADroneTrainingCourse>(nullptr, CourseClassPath);
		UClass* PawnClass = LoadClass<ADronePrototypePawn>(nullptr, PawnClassPath);
		UClass* ControllerClass = LoadClass<ADronePrototypePlayerController>(nullptr, ControllerClassPath);
		UClass* FlightHUDClass = LoadClass<UDroneFlightHUDWidget>(nullptr, FlightHUDClassPath);
		Test->TestNotNull(TEXT("Expected BP_DroneTrainingCourse Class loads"), CourseClass);
		Test->TestNotNull(TEXT("Expected BP_DronePrototypePawn Class loads"), PawnClass);
		Test->TestNotNull(TEXT("Expected BP_DronePrototypePlayerController Class loads"), ControllerClass);
		Test->TestNotNull(TEXT("Expected WBP_DroneFlightHUD Class loads"), FlightHUDClass);

		ADronePrototypePlayerController* Controller = Cast<ADronePrototypePlayerController>(
			PIEWorld->GetFirstPlayerController());
		Test->TestNotNull(TEXT("Training PIE spawns the Prototype PlayerController"), Controller);
		if (Controller && ControllerClass)
		{
			Test->TestTrue(TEXT("Training PIE uses the BP Prototype Controller"), Controller->GetClass() == ControllerClass);
		}

		ADronePrototypePawn* Drone = Controller ? Cast<ADronePrototypePawn>(Controller->GetPawn()) : nullptr;
		Test->TestNotNull(TEXT("Training PIE spawns and possesses the Prototype Drone"), Drone);
		if (Drone && PawnClass)
		{
			Test->TestTrue(TEXT("Training PIE uses BP_DronePrototypePawn"), Drone->GetClass() == PawnClass);
		}

		UDroneFlightHUDWidget* FlightHUD = Controller ? Controller->GetFlightHUDWidget() : nullptr;
		Test->TestNotNull(TEXT("Training PIE creates the Flight HUD"), FlightHUD);
		if (FlightHUD && FlightHUDClass)
		{
			Test->TestTrue(TEXT("Training PIE uses WBP_DroneFlightHUD"), FlightHUD->GetClass() == FlightHUDClass);
			Test->TestFalse(TEXT("Training PIE does not use the native fallback HUD layout"), FlightHUD->IsUsingNativeFallbackLayout());
		}

		ADroneTrainingCourse* Course = nullptr;
		int32 CourseCount = 0;
		for (TActorIterator<ADroneTrainingCourse> CourseIt(PIEWorld); CourseIt; ++CourseIt)
		{
			Course = *CourseIt;
			++CourseCount;
		}
		Test->TestEqual(TEXT("Training PIE has exactly one Course"), CourseCount, 1);
		Test->TestNotNull(TEXT("Training PIE contains the Course"), Course);

		int32 RecastNavMeshCount = 0;
		for (TActorIterator<AActor> ActorIt(PIEWorld); ActorIt; ++ActorIt)
		{
			const AActor* Actor = *ActorIt;
			if (Actor && Actor->GetClass()->GetPathName() == TEXT("/Script/NavigationSystem.RecastNavMesh"))
			{
				++RecastNavMeshCount;
			}
		}
		Test->TestTrue(
			TEXT("Training PIE contains saved Recast Navigation data"),
			RecastNavMeshCount > 0);

		if (Course)
		{
			if (CourseClass)
			{
				Test->TestTrue(TEXT("Training PIE uses BP_DroneTrainingCourse"), Course->GetClass() == CourseClass);
			}

			const USplineComponent* CourseSpline = Course->GetCourseSpline();
			Test->TestNotNull(TEXT("Runtime Course owns its Spline"), CourseSpline);
			const int32 ExpectedSegmentCount = CourseSpline
				? FMath::Max(0, CourseSpline->GetNumberOfSplinePoints() - 1)
				: 0;
			Test->TestEqual(
				TEXT("Runtime Course displays every Spline segment"),
				Course->GetCourseLineSegmentCount(),
				ExpectedSegmentCount);

			TInlineComponentArray<UPrimitiveComponent*> CoursePrimitives;
			Course->GetComponents(CoursePrimitives);
			for (UPrimitiveComponent* Primitive : CoursePrimitives)
			{
				if (!Primitive)
				{
					continue;
				}

				Test->TestEqual(
					*FString::Printf(TEXT("PIE component %s has no collision"), *Primitive->GetName()),
					Primitive->GetCollisionEnabled(),
					ECollisionEnabled::NoCollision);
				Test->TestFalse(
					*FString::Printf(TEXT("PIE component %s creates no overlaps"), *Primitive->GetName()),
					Primitive->GetGenerateOverlapEvents());
				Test->TestFalse(
					*FString::Printf(TEXT("PIE component %s cannot affect Navigation"), *Primitive->GetName()),
					Primitive->CanEverAffectNavigation());
			}

			// 실제 PIE Pawn을 안내선 가로 방향으로 Sweep해 표시 Mesh가 이동을 막지 않는지 확인한다.
			if (Drone && CourseSpline)
			{
				const float ProbeDistance = CourseSpline->GetSplineLength() * 0.12f;
				const FVector ProbeCenter = CourseSpline->GetLocationAtDistanceAlongSpline(
					ProbeDistance,
					ESplineCoordinateSpace::World);
				const FVector ProbeDirection = CourseSpline->GetDirectionAtDistanceAlongSpline(
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
				Test->TestFalse(TEXT("Training PIE Drone is not blocked by the guide line"), SweepHit.bBlockingHit);
				Test->TestTrue(
					TEXT("Training PIE Drone reaches the other side of the guide line"),
					Drone->GetActorLocation().Equals(SweepEnd, 1.0f));
				Test->TestTrue(TEXT("Training PIE guide line is never a Hit Actor"), SweepHit.GetActor() != Course);
			}
		}

		return true;
	}

private:
	FAutomationTestBase* Test;
	double StartedAt = 0.0;
};
} // namespace DroneTrainingPIESmoke

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneTrainingPIESmokeTest,
	"Drone.Tutorial.TrainingPIESmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneTrainingPIESmokeTest::RunTest(const FString& Parameters)
{
	using namespace DroneTrainingPIESmoke;

	// In-process PIE는 저장된 Recast Actor를 PIE NavDataSet에 등록하기 직전 CrowdManager를 먼저 만든다.
	// 아래 한 줄은 현재 fresh PIE에서 한 번 발생하는 Engine 초기화 순서 경고만 정확히 허용한다.
	// 검증 본문은 저장된 Recast Actor의 존재와 Course Component의 Navigation 비간섭 Flag를 별도로 검사한다.
	AddExpectedError(
		TEXT("Unable to find RecastNavMesh instance while trying to create UCrowdManager instance"),
		EAutomationExpectedErrorFlags::Contains,
		1);

	if (!GEditor)
	{
		AddError(TEXT("GEditor is unavailable"));
		return false;
	}

	if (GEditor->IsPlaySessionInProgress() || FindPIEWorld())
	{
		AddError(TEXT("Training PIE smoke test requires no pre-existing PIE session"));
		return false;
	}

	FAutomationEditorCommonUtils::LoadMap(MapPackage);
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld || EditorWorld->GetOutermost()->GetName() != MapPackage)
	{
		AddError(FString::Printf(TEXT("Could not open %s"), MapPackage));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakePlayParams()));
	ADD_LATENT_AUTOMATION_COMMAND(FValidateTrainingPIECommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
