#pragma once

#include "NativeGameplayTags.h"

enum class EDroneSmartObjectActivity : uint8;

/**
 * Smart Object Definition, NPC Profile, StateTree Event가 공유하는 Native Gameplay Tag다.
 * 문자열을 Blueprint마다 다시 입력하지 않아 오타와 Friendly/Hostile 혼선을 막는다.
 */
namespace DroneAITags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Friendly);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Hostile);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Rifle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Shotgun);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_MGTurretOperator);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_EnemyPatrol);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_FriendlyBasePatrol);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Ambient);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Cover);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_MGTurret);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DroneDetected);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DroneLost);

	/** Editor 표시용 Station enum을 실제 Smart Object Activity Tag로 변환한다. */
	DRONE_API FGameplayTag GetActivityTag(EDroneSmartObjectActivity Activity);
}
