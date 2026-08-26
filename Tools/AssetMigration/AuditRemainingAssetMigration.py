"""Verify the selected remaining-asset migration inside the real Drone project."""

import json
import os

import unreal


TARGETS = {
    "ArmyVFX": ("/Game/Drone/ThirdParty/ArmyVFX", 44),
    "InfantrySFX": ("/Game/Drone/ThirdParty/InfantrySFX", 8),
    "GroundDroneKit": ("/Game/Drone/ThirdParty/GroundDroneKit", 78),
    "ModularSoldier": ("/Game/Drone/ThirdParty/ModularSoldier", 80),
    "ModularInsurgents": ("/Game/Drone/ThirdParty/ModularInsurgents", 46),
    "RawDrones": ("/Game/Drone/ThirdParty/RawDrones", 16),
    "OilRig": ("/Game/Drone/ThirdParty/OilRig", 619),
}

LOAD_TARGETS = [
    "/Game/Drone/ThirdParty/ArmyVFX/Niagara/Destroyed/NS_Expl_Tank_1",
    "/Game/Drone/ThirdParty/InfantrySFX/Explosions/Cues/Cue_Explosion01_Cue",
    "/Game/Drone/ThirdParty/GroundDroneKit/Meshes/Alt_Turrets/MG_Turret/MG_Turret_SK",
    "/Game/Drone/ThirdParty/GroundDroneKit/Meshes/GC_Drone_1/GC_Drone_1_SK",
    "/Game/Drone/ThirdParty/ModularSoldier/Meshes/Body/SKM_Modular_Soldier",
    "/Game/Drone/ThirdParty/ModularInsurgents/Mesh/SK_Preset1",
    "/Game/Drone/ThirdParty/RawDrones/NonPilot/QuadV4/SM_QuadV4_Body",
    "/Game/Drone/ThirdParty/RawDrones/StingInterceptor/SM_StingInterceptor",
]

MAP_PACKAGE = "/Game/Drone/Maps/Lvl_OilRig"


def is_project_owned(package_name):
    return package_name == "/Game/Drone" or package_name.startswith("/Game/Drone/")


registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)

result = {"targets": {}, "load_targets": {}, "oilrig_map": {}}
all_external_dependencies = set()
all_missing_dependencies = set()

for name, (root, expected_count) in TARGETS.items():
    assets = registry.get_assets_by_path(root, recursive=True)
    class_counts = {}
    external_dependencies = set()
    missing_dependencies = set()
    for asset_data in assets:
        class_name = str(asset_data.asset_class_path.asset_name)
        class_counts[class_name] = class_counts.get(class_name, 0) + 1
        package_name = str(asset_data.package_name)
        for dependency in registry.get_dependencies(package_name, options) or []:
            dependency_name = str(dependency)
            if dependency_name.startswith("/Game/") and not is_project_owned(
                dependency_name
            ):
                external_dependencies.add(dependency_name)
            if dependency_name.startswith("/Game/") and not registry.get_assets_by_package_name(
                dependency_name, True
            ):
                missing_dependencies.add(dependency_name)

    all_external_dependencies.update(external_dependencies)
    all_missing_dependencies.update(missing_dependencies)
    result["targets"][name] = {
        "root": root,
        "expected_asset_count": expected_count,
        "asset_count": len(assets),
        "count_matches": len(assets) == expected_count,
        "class_counts": dict(sorted(class_counts.items())),
        "external_game_dependencies": sorted(external_dependencies),
        "missing_game_dependencies": sorted(missing_dependencies),
    }

for object_path in LOAD_TARGETS:
    loaded_asset = unreal.EditorAssetLibrary.load_asset(object_path)
    result["load_targets"][object_path] = {
        "loaded": loaded_asset is not None,
        "class": loaded_asset.get_class().get_path_name() if loaded_asset else None,
    }

oilrig_map = unreal.EditorAssetLibrary.load_asset(MAP_PACKAGE)
world_settings = oilrig_map.get_world_settings() if oilrig_map else None
map_dependencies = registry.get_dependencies(MAP_PACKAGE, options) or []
map_external = sorted(
    str(item)
    for item in map_dependencies
    if str(item).startswith("/Game/") and not is_project_owned(str(item))
)
map_missing = sorted(
    str(item)
    for item in map_dependencies
    if str(item).startswith("/Game/")
    and not registry.get_assets_by_package_name(str(item), True)
)
all_external_dependencies.update(map_external)
all_missing_dependencies.update(map_missing)
result["oilrig_map"] = {
    "package": MAP_PACKAGE,
    "loaded": oilrig_map is not None,
    "default_game_mode": str(
        world_settings.get_editor_property("default_game_mode")
    ) if world_settings else None,
    "external_game_dependencies": map_external,
    "missing_game_dependencies": map_missing,
}

result["summary"] = {
    "all_counts_match": all(
        item["count_matches"] for item in result["targets"].values()
    ),
    "all_load_targets_loaded": all(
        item["loaded"] for item in result["load_targets"].values()
    ),
    "oilrig_map_loaded": oilrig_map is not None,
    "external_game_dependencies": sorted(all_external_dependencies),
    "missing_game_dependencies": sorted(all_missing_dependencies),
}

report_dir = os.path.join(unreal.Paths.project_saved_dir(), "AssetMigration")
os.makedirs(report_dir, exist_ok=True)
report_path = os.path.join(report_dir, "remaining_asset_migration_audit.json")
with open(report_path, "w", encoding="utf-8") as report_file:
    json.dump(result, report_file, ensure_ascii=False, indent=2)

unreal.log(f"DRONE_REMAINING_ASSET_AUDIT={report_path}")
unreal.log(json.dumps(result["summary"], ensure_ascii=False, indent=2))

if (
    not result["summary"]["all_counts_match"]
    or not result["summary"]["all_load_targets_loaded"]
    or not result["summary"]["oilrig_map_loaded"]
    or all_external_dependencies
    or all_missing_dependencies
):
    raise RuntimeError("Remaining asset migration audit failed")
