#include "AI/DroneNPCProfileComponent.h"

#include "AI/DroneAITags.h"

UDroneNPCProfileComponent::UDroneNPCProfileComponent()
{
	// 역할 데이터만 보관하므로 매 프레임 갱신하지 않는다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDroneNPCProfileComponent::SetProfile(const FDroneNPCProfile& NewProfile)
{
	Profile = NewProfile;
}

FGameplayTagContainer UDroneNPCProfileComponent::BuildSmartObjectUserTags() const
{
	FGameplayTagContainer Tags;

	switch (Profile.Faction)
	{
	case EDroneNPCFaction::Friendly:
		Tags.AddTag(DroneAITags::Faction_Friendly);
		break;
	case EDroneNPCFaction::Hostile:
		Tags.AddTag(DroneAITags::Faction_Hostile);
		break;
	default:
		break;
	}

	switch (Profile.WeaponType)
	{
	case EDroneNPCWeaponType::Rifle:
		Tags.AddTag(DroneAITags::Weapon_Rifle);
		break;
	case EDroneNPCWeaponType::Shotgun:
		Tags.AddTag(DroneAITags::Weapon_Shotgun);
		break;
	default:
		break;
	}

	if (Profile.bCanUseMGTurret && Profile.Faction == EDroneNPCFaction::Hostile)
	{
		Tags.AddTag(DroneAITags::Role_MGTurretOperator);
	}

	return Tags;
}
