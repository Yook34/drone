"""Audit recursive dependencies for project-owned staging seeds without edits."""

import json
import os
from collections import deque

import unreal


def read_json_array(name, default=None):
    raw_value = os.environ.get(name)
    if not raw_value:
        return list(default or [])
    value = json.loads(raw_value)
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise RuntimeError(f"{name} must be a JSON array of strings")
    return value


seeds = read_json_array("DRONE_AUDIT_SEEDS")
allowed_prefixes = read_json_array("DRONE_AUDIT_ALLOWED_PREFIXES", ["/Game/Drone"])
report_name = os.environ.get("DRONE_AUDIT_REPORT_NAME", "seed_dependency_audit.json")
if not seeds:
    raise RuntimeError("DRONE_AUDIT_SEEDS is empty")

options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=False,
    include_hard_management_references=False,
)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

visited = set()
queue = deque(seeds)
parent = {seed: None for seed in seeds}
while queue:
    package_name = queue.popleft()
    if package_name in visited:
        continue
    visited.add(package_name)
    for dependency in registry.get_dependencies(package_name, options) or []:
        dependency_name = str(dependency)
        if dependency_name.startswith("/Game/") and dependency_name not in visited:
            parent.setdefault(dependency_name, package_name)
            queue.append(dependency_name)

game_packages = sorted(item for item in visited if item.startswith("/Game/"))
external_packages = sorted(
    item
    for item in game_packages
    if not any(
        item == prefix or item.startswith(prefix.rstrip("/") + "/")
        for prefix in allowed_prefixes
    )
)
missing_packages = sorted(
    item
    for item in game_packages
    if not registry.get_assets_by_package_name(item, True)
)
demo_packages = sorted(item for item in game_packages if "/Demo/" in item)


def dependency_chain(package_name):
    chain = []
    current = package_name
    while current is not None:
        chain.append(current)
        current = parent.get(current)
    chain.reverse()
    return chain

result = {
    "seeds": seeds,
    "allowed_prefixes": allowed_prefixes,
    "game_dependency_count": len(game_packages),
    "external_packages": external_packages,
    "missing_packages": missing_packages,
    "demo_packages": demo_packages,
    "demo_chains": {item: dependency_chain(item) for item in demo_packages},
    "external_chains": {item: dependency_chain(item) for item in external_packages},
    "missing_chains": {item: dependency_chain(item) for item in missing_packages},
    "game_packages": game_packages,
}

report_dir = os.path.join(unreal.Paths.project_saved_dir(), "AssetMigration")
os.makedirs(report_dir, exist_ok=True)
report_path = os.path.join(report_dir, report_name)
with open(report_path, "w", encoding="utf-8") as report_file:
    json.dump(result, report_file, ensure_ascii=False, indent=2)

unreal.log(f"DRONE_ASSET_AUDIT_REPORT={report_path}")
unreal.log(json.dumps(result, ensure_ascii=False, indent=2))

if external_packages or missing_packages:
    raise RuntimeError("Seed closure contains external or missing /Game packages")
