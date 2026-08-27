#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/DroneAITags.h"
#include "AI/DroneNPCAIController.h"
#include "AI/DroneNPCCharacter.h"
#include "AI/DroneNPCProfileComponent.h"
#include "AI/DroneNPCSpawnPoint.h"
#include "AI/DroneSmartObjectReservationComponent.h"
#include "AI/DroneSmartObjectStation.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Prototype/DronePrototypePawn.h"
#include "SmartObjectComponent.h"
#include "SmartObjectUserComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneSmartObjectFoundationTest,
	"Drone.AI.SmartObjectFoundationDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneSmartObjectFoundationTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Drone NPC Character is concrete"), ADroneNPCCharacter::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	TestFalse(TEXT("Drone NPC AI Controller is concrete"), ADroneNPCAIController::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	TestFalse(TEXT("NPC Spawn Point is concrete"), ADroneNPCSpawnPoint::StaticClass()->HasAnyClassFlags(CLASS_Abstract));
	TestFalse(TEXT("Smart Object Station is concrete"), ADroneSmartObjectStation::StaticClass()->HasAnyClassFlags(CLASS_Abstract));

	const ADroneNPCCharacter* NPCDefaults = GetDefault<ADroneNPCCharacter>();
	TestNotNull(TEXT("NPC Character CDO exists"), NPCDefaults);
	if (NPCDefaults)
	{
		TestTrue(TEXT("NPC uses project AI Controller"), NPCDefaults->AIControllerClass == ADroneNPCAIController::StaticClass());
		TestTrue(TEXT("NPC auto-possesses when placed or spawned"), NPCDefaults->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned);
		TestNotNull(TEXT("NPC owns a Profile Component"), NPCDefaults->GetNPCProfileComponent());
		TestNotNull(TEXT("NPC owns a Smart Object User Component"), NPCDefaults->GetSmartObjectUserComponent());
	}

	const ADroneNPCAIController* ControllerDefaults = GetDefault<ADroneNPCAIController>();
	TestNotNull(TEXT("NPC AI Controller CDO exists"), ControllerDefaults);
	if (ControllerDefaults)
	{
		TestNotNull(TEXT("Controller owns StateTree AI"), ControllerDefaults->GetStateTreeAIComponent());
		TestNotNull(TEXT("Controller owns AI Perception"), ControllerDefaults->GetDronePerceptionComponent());
		TestNotNull(TEXT("Controller owns reservation lifecycle"), ControllerDefaults->GetReservationComponent());
		TestFalse(TEXT("Controller starts without a fake detected Drone"), ControllerDefaults->HasDetectedDrone());
	}

	const ADroneSmartObjectStation* StationDefaults = GetDefault<ADroneSmartObjectStation>();
	TestNotNull(TEXT("Smart Object Station CDO exists"), StationDefaults);
	if (StationDefaults)
	{
		TestFalse(TEXT("Station avoids per-frame Actor Tick"), StationDefaults->PrimaryActorTick.bCanEverTick);
		TestNotNull(TEXT("Station owns Smart Object Component"), StationDefaults->GetSmartObjectComponent());
		TestNotNull(TEXT("Station owns optional Skeletal Mesh Component"), StationDefaults->GetStationMesh());
		TestNotNull(TEXT("Station owns slot facing preview"), StationDefaults->GetSlotFacingPreview());
		TestTrue(TEXT("Default station activity tag is valid"), StationDefaults->GetExpectedActivityTag().IsValid());
		TestFalse(TEXT("Native station does not hide a missing Definition"), StationDefaults->HasSmartObjectDefinition());
	}

	const ADroneNPCSpawnPoint* SpawnDefaults = GetDefault<ADroneNPCSpawnPoint>();
	TestNotNull(TEXT("NPC Spawn Point CDO exists"), SpawnDefaults);
	if (SpawnDefaults)
	{
		TestFalse(TEXT("Spawn Point avoids per-frame Actor Tick"), SpawnDefaults->PrimaryActorTick.bCanEverTick);
		TestFalse(TEXT("Spawn Point does not mutate a Map by default"), SpawnDefaults->ShouldSpawnOnBeginPlay());
		TestEqual(TEXT("Spawn Point defaults to one NPC"), SpawnDefaults->GetConfiguredSpawnCount(), 1);
		TestTrue(TEXT("Single default NPC spawns at the marker"), SpawnDefaults->GetSpawnTransformForIndex(0).Equals(SpawnDefaults->GetActorTransform()));
	}

	UDroneNPCProfileComponent* Profile = NewObject<UDroneNPCProfileComponent>();
	TestNotNull(TEXT("Transient NPC Profile can be created"), Profile);
	if (Profile)
	{
		FDroneNPCProfile HostileRifleProfile;
		HostileRifleProfile.Faction = EDroneNPCFaction::Hostile;
		HostileRifleProfile.WeaponType = EDroneNPCWeaponType::Rifle;
		HostileRifleProfile.bCanUseMGTurret = true;
		Profile->SetProfile(HostileRifleProfile);

		const FGameplayTagContainer UserTags = Profile->BuildSmartObjectUserTags();
		TestTrue(TEXT("Hostile Profile emits Hostile tag"), UserTags.HasTagExact(DroneAITags::Faction_Hostile));
		TestTrue(TEXT("Rifle Profile emits Rifle tag"), UserTags.HasTagExact(DroneAITags::Weapon_Rifle));
		TestTrue(TEXT("Eligible Hostile emits MG operator tag"), UserTags.HasTagExact(DroneAITags::Role_MGTurretOperator));
		TestFalse(TEXT("Hostile Profile never emits Friendly tag"), UserTags.HasTagExact(DroneAITags::Faction_Friendly));
	}

	const ADronePrototypePawn* DroneDefaults = GetDefault<ADronePrototypePawn>();
	TestNotNull(TEXT("Prototype Drone CDO exists"), DroneDefaults);
	if (DroneDefaults)
	{
		TestNotNull(TEXT("Prototype Drone is an explicit perception source"), DroneDefaults->GetPerceptionStimuliSource());
	}

	return true;
}

#endif
