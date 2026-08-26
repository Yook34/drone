"""Import a small, reviewable raw-FBX drone candidate set in UE 5.8.

The source paths are read-only inputs.  This script runs in a disposable stage
and writes only project-owned assets below /Game/Drone/ThirdParty/RawDrones.
"""

import json
import os

import unreal


TARGET_ROOT = "/Game/Drone/ThirdParty/RawDrones"
IMPORTS = [
    {
        "filename": r"C:\에셋\Non-Pilot Drones KITBASH SET\FBX\Drones_Quad_v4.fbx",
        "destination_path": TARGET_ROOT + "/NonPilot/QuadV4",
        "destination_name": "SM_QuadV4_Body",
    },
    {
        "filename": r"C:\에셋\Non-Pilot Drones KITBASH SET\FBX\Drones_Quad_v4_cam1.fbx",
        "destination_path": TARGET_ROOT + "/NonPilot/QuadV4",
        "destination_name": "SM_QuadV4_CameraA",
    },
    {
        "filename": r"C:\에셋\Non-Pilot Drones KITBASH SET\FBX\Drones_Quad_v4_cam2.fbx",
        "destination_path": TARGET_ROOT + "/NonPilot/QuadV4",
        "destination_name": "SM_QuadV4_CameraB",
    },
    {
        "filename": r"C:\에셋\Non-Pilot Drones KITBASH SET\FBX\Drones_Quad_v4_prop_v2.fbx",
        "destination_path": TARGET_ROOT + "/NonPilot/QuadV4",
        "destination_name": "SM_QuadV4_Propeller",
    },
    {
        "filename": (
            "C:/에셋/PBR_Sting_Counter-Drone_Interceptor_UAV___Anti-Drone___"
            "Loitering_Munition-62a9ca6e/fbx/sting_interceptor_drone__extracted/"
            "STING_INTERCEPTOR_DRONE_UA_FBX/STING_INTERCEPTOR_DRONE_UA.fbx"
        ),
        "destination_path": TARGET_ROOT + "/StingInterceptor",
        "destination_name": "SM_StingInterceptor",
    },
]


def make_fbx_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property(
        "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH
    )
    static_options = options.get_editor_property("static_mesh_import_data")
    static_options.set_editor_property("combine_meshes", True)
    static_options.set_editor_property("generate_lightmap_u_vs", True)
    static_options.set_editor_property("auto_generate_collision", True)
    return options


tasks = []
for item in IMPORTS:
    if not os.path.isfile(item["filename"]):
        raise RuntimeError(f"Raw FBX source is missing: {item['filename']}")
    unreal.EditorAssetLibrary.make_directory(item["destination_path"])
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", item["filename"])
    task.set_editor_property("destination_path", item["destination_path"])
    task.set_editor_property("destination_name", item["destination_name"])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", make_fbx_options())
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
if not unreal.EditorAssetLibrary.save_directory(
    TARGET_ROOT, only_if_is_dirty=False, recursive=True
):
    raise RuntimeError("Failed to save imported raw drone candidates")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous([TARGET_ROOT], force_rescan=True)
registry.wait_for_completion()
options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)

assets = registry.get_assets_by_path(TARGET_ROOT, recursive=True)
packages = sorted(str(item.package_name) for item in assets)
class_counts = {}
external_dependencies = set()
missing_dependencies = set()
for asset_data in assets:
    class_name = str(asset_data.asset_class_path.asset_name)
    class_counts[class_name] = class_counts.get(class_name, 0) + 1
    for dependency in registry.get_dependencies(str(asset_data.package_name), options) or []:
        dependency_name = str(dependency)
        if dependency_name.startswith("/Game/") and not dependency_name.startswith(
            TARGET_ROOT + "/"
        ):
            external_dependencies.add(dependency_name)
        if dependency_name.startswith("/Game/") and not registry.get_assets_by_package_name(
            dependency_name, True
        ):
            missing_dependencies.add(dependency_name)

result = {
    "imports": IMPORTS,
    "imported_paths": [list(task.imported_object_paths) for task in tasks],
    "asset_count": len(assets),
    "class_counts": dict(sorted(class_counts.items())),
    "packages": packages,
    "external_game_dependencies": sorted(external_dependencies),
    "missing_game_dependencies": sorted(missing_dependencies),
}
report_dir = os.path.join(unreal.Paths.project_saved_dir(), "AssetMigration")
os.makedirs(report_dir, exist_ok=True)
report_path = os.path.join(report_dir, "raw_drone_import_report.json")
with open(report_path, "w", encoding="utf-8") as report_file:
    json.dump(result, report_file, ensure_ascii=False, indent=2)

unreal.log(f"DRONE_RAW_IMPORT_REPORT={report_path}")
unreal.log(json.dumps(result, ensure_ascii=False, indent=2))

if external_dependencies or missing_dependencies:
    raise RuntimeError("Raw drone imports have external or missing /Game dependencies")
