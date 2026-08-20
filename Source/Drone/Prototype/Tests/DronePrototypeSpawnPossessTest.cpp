#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Prototype/DronePrototypeGameMode.h"
#include "Prototype/DronePrototypePawn.h"
#include "Tests/AutomationCommon.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDronePrototypeSpawnPossessTest,
	"Drone.Prototype.SpawnPossess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDronePrototypeSpawnPossessTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	UWorld* TestWorld = WorldWrapper.GetTestWorld();
	TestNotNull(TEXT("Transient game world exists"), TestWorld);
	if (!TestWorld)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	AWorldSettings* WorldSettings = TestWorld->GetWorldSettings();
	TestNotNull(TEXT("World settings exist"), WorldSettings);
	if (!WorldSettings)
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	WorldSettings->DefaultGameMode = ADronePrototypeGameMode::StaticClass();
	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	AGameModeBase* GameMode = TestWorld->GetAuthGameMode();
	TestNotNull(TEXT("Prototype GameMode spawned"), GameMode);
	TestTrue(TEXT("World uses the Prototype GameMode"), GameMode && GameMode->IsA<ADronePrototypeGameMode>());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerController* PlayerController = TestWorld->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	TestNotNull(TEXT("Transient PlayerController spawned"), PlayerController);

	if (GameMode && PlayerController)
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 200.0f));
		GameMode->RestartPlayerAtTransform(PlayerController, SpawnTransform);

		APawn* SpawnedPawn = PlayerController->GetPawn();
		TestNotNull(TEXT("GameMode spawned a Pawn"), SpawnedPawn);
		TestTrue(TEXT("Spawned Pawn is the Drone Prototype"), SpawnedPawn && SpawnedPawn->IsA<ADronePrototypePawn>());
		TestTrue(TEXT("PlayerController possesses the spawned Pawn"), SpawnedPawn && SpawnedPawn->GetController() == PlayerController);
		TestTrue(TEXT("Spawned Pawn is transient in the test world"), SpawnedPawn && SpawnedPawn->HasAnyFlags(RF_Transient));
	}

	WorldWrapper.TickTestWorld();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
