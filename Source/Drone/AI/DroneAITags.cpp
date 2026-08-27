#include "AI/DroneAITags.h"

#include "AI/DroneAITypes.h"

namespace DroneAITags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Friendly, "Drone.AI.Faction.Friendly", "기지 아군 NPC");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Hostile, "Drone.AI.Faction.Hostile", "드론에 대응하는 적 NPC");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Rifle, "Drone.AI.Weapon.Rifle", "소총 사격 분기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Shotgun, "Drone.AI.Weapon.Shotgun", "샷건 사격 분기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Role_MGTurretOperator, "Drone.AI.Role.MGTurretOperator", "MG Turret 사용 가능 역할");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_EnemyPatrol, "Drone.SmartObject.Activity.EnemyPatrol", "적 경계 순찰 지점");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_FriendlyBasePatrol, "Drone.SmartObject.Activity.FriendlyBasePatrol", "기지 아군 순찰 지점");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Ambient, "Drone.SmartObject.Activity.Ambient", "비전투 대기·생활 지점");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Guard, "Drone.SmartObject.Activity.Guard", "경계 대기 지점");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Cover, "Drone.SmartObject.Activity.Cover", "엄폐 후보 지점");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_MGTurret, "Drone.SmartObject.Activity.MGTurret", "한 명만 점유하는 MG Turret 지점");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_DroneDetected, "Drone.AI.Event.DroneDetected", "Hostile NPC가 드론을 새로 감지함");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_DroneLost, "Drone.AI.Event.DroneLost", "Hostile NPC가 드론 시야를 잃음");

	FGameplayTag GetActivityTag(const EDroneSmartObjectActivity Activity)
	{
		switch (Activity)
		{
		case EDroneSmartObjectActivity::EnemyPatrol:
			return Activity_EnemyPatrol;
		case EDroneSmartObjectActivity::FriendlyBasePatrol:
			return Activity_FriendlyBasePatrol;
		case EDroneSmartObjectActivity::Ambient:
			return Activity_Ambient;
		case EDroneSmartObjectActivity::Guard:
			return Activity_Guard;
		case EDroneSmartObjectActivity::Cover:
			return Activity_Cover;
		case EDroneSmartObjectActivity::MGTurret:
			return Activity_MGTurret;
		default:
			return FGameplayTag();
		}
	}
}
