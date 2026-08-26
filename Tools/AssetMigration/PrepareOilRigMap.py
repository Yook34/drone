"""Create a cleaned project-owned OilRig map copy in a disposable stage."""

import json
import os

import unreal


SOURCE_MAP = "/Game/Liope_Tr/Maps/Overview"
TARGET_MAP = "/Game/Drone/Maps/Lvl_OilRig"


if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    raise RuntimeError(f"Missing OilRig source map: {SOURCE_MAP}")

unreal.EditorAssetLibrary.make_directory("/Game/Drone/Maps")
if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    cleaned_map = unreal.EditorAssetLibrary.load_asset(TARGET_MAP)
else:
    cleaned_map = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP)
if cleaned_map is None:
    raise RuntimeError(f"Failed to duplicate OilRig map: {SOURCE_MAP}")

world = unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP)
if world is None:
    raise RuntimeError(f"Failed to load OilRig map: {TARGET_MAP}")

# The imported environment is a location, not a replacement game framework.
# Clearing the vendor GameMode lets DroneGameMode own pawn/controller/UI behavior.
world.get_world_settings().set_editor_property("default_game_mode", None)

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
demo_actors = []
for actor in actor_subsystem.get_all_level_actors():
    class_path = actor.get_class().get_path_name()
    # The downloaded files are mounted below /Game/Liope_Tr in staging and
    # below /Game/Drone/ThirdParty/OilRig after namespacing.  Match the Demo
    # segment instead of assuming either mount path.
    # BP_Simple_Door casts to the vendor FirstPersonCharacter and drags the
    # obsolete sample pawn/input/arms chain into an otherwise environment-only
    # map.  Remove those door actors instead of importing a second game stack.
    is_first_person_demo = "/Demo/" in class_path
    is_sample_door = "BP_Simple_Door" in class_path
    if is_first_person_demo or is_sample_door:
        demo_actors.append(actor)

removed_actor_labels = [actor.get_actor_label() for actor in demo_actors]
if demo_actors and not actor_subsystem.destroy_actors(demo_actors):
    raise RuntimeError("Failed to remove vendor demo actors from OilRig map")

if not unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save cleaned OilRig map: {TARGET_MAP}")

result = {
    "source_map": SOURCE_MAP,
    "target_map": TARGET_MAP,
    "default_game_mode_cleared": True,
    "removed_demo_actors": removed_actor_labels,
}
report_dir = os.path.join(unreal.Paths.project_saved_dir(), "AssetMigration")
os.makedirs(report_dir, exist_ok=True)
report_path = os.path.join(report_dir, "oilrig_prepare_report.json")
with open(report_path, "w", encoding="utf-8") as report_file:
    json.dump(result, report_file, ensure_ascii=False, indent=2)

unreal.log(f"DRONE_OILRIG_PREPARE_REPORT={report_path}")
unreal.log(json.dumps(result, ensure_ascii=False, indent=2))
