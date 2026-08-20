#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypeGameMode.h"
#include "Prototype/DronePrototypePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDronePrototypeDefaultsTest,
	"Drone.Prototype.PawnDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDronePrototypeDefaultsTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Prototype Pawn class is concrete"), ADronePrototypePawn::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	TestFalse(TEXT("Prototype GameMode class is concrete"), ADronePrototypeGameMode::StaticClass()->HasAnyClassFlags(CLASS_Abstract));

	const ADronePrototypePawn* PawnDefaults = GetDefault<ADronePrototypePawn>();
	TestNotNull(TEXT("Prototype Pawn CDO exists"), PawnDefaults);

	if (PawnDefaults)
	{
		TestNotNull(TEXT("Collision component exists"), PawnDefaults->GetCollisionComponent());
		TestNotNull(TEXT("Visual mesh component exists"), PawnDefaults->GetVisualMeshComponent());
		TestNotNull(TEXT("Camera boom exists"), PawnDefaults->GetCameraBoom());
		TestNotNull(TEXT("Follow camera exists"), PawnDefaults->GetFollowCamera());
		TestNotNull(TEXT("Floating Pawn Movement exists"), PawnDefaults->GetPrototypeMovementComponent());
		TestTrue(TEXT("Collision component is the root"), PawnDefaults->GetRootComponent() == PawnDefaults->GetCollisionComponent());
		TestTrue(TEXT("Pawn exposes the prototype movement component"), PawnDefaults->GetMovementComponent() == PawnDefaults->GetPrototypeMovementComponent());

		if (PawnDefaults->GetCollisionComponent())
		{
			TestEqual(TEXT("Prototype collision radius"), PawnDefaults->GetCollisionComponent()->GetUnscaledSphereRadius(), 45.0f);
			TestEqual(TEXT("Prototype collision profile"), PawnDefaults->GetCollisionComponent()->GetCollisionProfileName(), FName(TEXT("Pawn")));
			TestFalse(TEXT("Prototype collision physics simulation is disabled"), PawnDefaults->GetCollisionComponent()->IsSimulatingPhysics());
		}

		if (PawnDefaults->GetVisualMeshComponent())
		{
			TestTrue(
				TEXT("Prototype visual mesh collision is disabled"),
				PawnDefaults->GetVisualMeshComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestFalse(TEXT("Prototype visual mesh physics simulation is disabled"), PawnDefaults->GetVisualMeshComponent()->IsSimulatingPhysics());
		}

		if (PawnDefaults->GetCameraBoom())
		{
			TestTrue(
				TEXT("Camera boom is attached to the collision root"),
				PawnDefaults->GetCameraBoom()->GetAttachParent() == PawnDefaults->GetCollisionComponent());
			TestTrue(TEXT("Camera boom follows controller rotation"), PawnDefaults->GetCameraBoom()->bUsePawnControlRotation);
		}

		if (PawnDefaults->GetFollowCamera())
		{
			TestTrue(
				TEXT("Follow camera is attached to the camera boom"),
				PawnDefaults->GetFollowCamera()->GetAttachParent() == PawnDefaults->GetCameraBoom());
			TestFalse(TEXT("Follow camera does not add controller rotation twice"), PawnDefaults->GetFollowCamera()->bUsePawnControlRotation);
		}

		if (PawnDefaults->GetPrototypeMovementComponent())
		{
			TestTrue(
				TEXT("Movement component updates the collision root"),
				PawnDefaults->GetPrototypeMovementComponent()->UpdatedComponent == PawnDefaults->GetCollisionComponent());
			TestTrue(TEXT("Prototype max speed is positive"), PawnDefaults->GetPrototypeMovementComponent()->MaxSpeed > 0.0f);
		}
	}

	const ADronePrototypeGameMode* GameModeDefaults = GetDefault<ADronePrototypeGameMode>();
	TestNotNull(TEXT("Prototype GameMode CDO exists"), GameModeDefaults);
	if (GameModeDefaults)
	{
		TestTrue(
			TEXT("Prototype GameMode spawns the Prototype Pawn"),
			GameModeDefaults->DefaultPawnClass == ADronePrototypePawn::StaticClass());
	}

	return true;
}

#endif
