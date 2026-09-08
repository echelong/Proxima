"""Run inside Unreal Editor. Creates owned starter assets only when missing.

No external downloads. Existing materials and levels are preserved.
See Docs/HOUSE_WORKSHOP.md for the build and validation workflow.
"""
import json
from pathlib import Path
import unreal

ASSET_ROOT = "/Game/Proxima"
MAP = ASSET_ROOT + "/Maps/Workshop"
ASSETS = unreal.EditorAssetLibrary
MATERIALS = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def expression(material, cls, **properties):
    node = require(MATERIALS.create_material_expression(material, cls), "Could not create material expression")
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def connect(source, destination, input_name=""):
    # An empty name is Unreal's documented way to select the first input/output.
    if not MATERIALS.connect_material_expressions(source, "", destination, input_name):
        available = list(MATERIALS.get_material_expression_input_names(destination))
        raise RuntimeError(
            "Material connection failed: " + (input_name or "<first input>")
            + "; destination=" + destination.get_class().get_name()
            + "; available inputs=" + repr(available))


def output(source, property_name):
    require(MATERIALS.connect_material_property(source, "", property_name), "Material output connection failed")


def material(name, glass=False, default_tint=(0.82, 0.80, 0.74, 1.0), default_roughness=0.75):
    path = ASSET_ROOT + "/Materials/" + name
    unreal.log("Proxima bootstrap material: " + path)
    if ASSETS.does_asset_exist(path):
        return require(ASSETS.load_asset(path), "Existing material could not be loaded: " + path)
    result = require(TOOLS.create_asset(name, ASSET_ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew()), "Could not create " + path)
    result.set_editor_property("used_with_instanced_static_meshes", True)
    tint = expression(result, unreal.MaterialExpressionVectorParameter,
                      parameter_name="Tint", default_value=unreal.LinearColor(*default_tint))
    roughness = expression(result, unreal.MaterialExpressionScalarParameter,
                           parameter_name="Roughness", default_value=default_roughness)
    output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    if glass:
        result.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        result.set_editor_property("two_sided", True)
        opacity = expression(result, unreal.MaterialExpressionConstant, r=0.25)
        output(opacity, unreal.MaterialProperty.MP_OPACITY)
        output(tint, unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        # Subtle world-space plaster variation avoids texture stretching when
        # a wall changes length. One octave keeps the generated shader modest.
        noise = expression(result, unreal.MaterialExpressionNoise, levels=1, quality=1,
                           scale=0.65, output_min=0.94, output_max=1.0)
        position = expression(result, unreal.MaterialExpressionWorldPosition)
        # Noise's first input is its position. The graph pin's displayed name
        # can differ from the C++ member name (Position), as on UE 5.8.2.
        connect(position, noise)
        multiply = expression(result, unreal.MaterialExpressionMultiply)
        connect(tint, multiply, "A")
        connect(noise, multiply, "B")
        output(multiply, unreal.MaterialProperty.MP_BASE_COLOR)
    MATERIALS.recompile_material(result)
    require(ASSETS.save_loaded_asset(result), "Could not save material " + path)
    return result


def build():
    unreal.log("PROXIMA_BOOTSTRAP_REVISION=direct-materials-1")
    ASSETS.make_directory(ASSET_ROOT + "/Materials")
    ASSETS.make_directory(ASSET_ROOT + "/Maps")
    material("M_Surface")
    material("M_Glass", glass=True)
    # Fixed starter finishes do not need material-instance parameter lookups.
    # Put their colour/roughness directly in the same graph used by M_Surface.
    grass = material("M_Grass", default_tint=(0.12, 0.19, 0.085, 1.0), default_roughness=0.95)
    stone = material("M_Stone", default_tint=(0.33, 0.35, 0.30, 1.0), default_roughness=0.9)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not ASSETS.does_asset_exist(MAP):
        require(level.new_level(MAP), "Could not create Workshop level")
        cube = require(ASSETS.load_asset("/Engine/BasicShapes/Cube"), "Engine cube missing")

        def spawn(cls, name, position, rotation=(0, 0, 0)):
            actor = require(actors.spawn_actor_from_class(cls, unreal.Vector(*position), unreal.Rotator(*rotation)), "Actor spawn failed: " + name)
            actor.set_actor_label(name)
            return actor

        def box(name, center, size, finish):
            actor = spawn(unreal.StaticMeshActor, name, center)
            mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
            mesh.set_static_mesh(cube)
            mesh.set_material(0, finish)
            mesh.set_collision_profile_name("BlockAll")
            actor.set_actor_scale3d(unreal.Vector(*(v / 100.0 for v in size)))
            return actor

        # Ground is below the 20 cm foundation. The room floor top is exactly Z=0.
        box("Landscape base", (0, 0, -70), (220000, 220000, 100), grass)
        box("Entry path", (0, -800, -22), (240, 700, 4), stone)
        spawn(unreal.PlayerStart, "Home entrance", (0, -1100, 100), (0, 90, 0))
        sun = spawn(unreal.DirectionalLight, "Afternoon sun", (0, 0, 3000), (-38, -35, 0))
        light = sun.get_component_by_class(unreal.DirectionalLightComponent)
        light.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity", 50000.0)
        light.set_editor_property("atmosphere_sun_light", True)
        spawn(unreal.SkyAtmosphere, "Atmosphere", (0, 0, 0))
        skylight = spawn(unreal.SkyLight, "Sky fill", (0, 0, 1000))
        sky = skylight.get_component_by_class(unreal.SkyLightComponent)
        sky.set_mobility(unreal.ComponentMobility.MOVABLE)
        sky.set_editor_property("real_time_capture", True)
        sky.set_editor_property("intensity", 1.0)
        require(level.save_current_level(), "Workshop level save failed")
    else:
        require(level.load_level(MAP), "Existing Workshop level could not be loaded")
    required = [MAP] + [ASSET_ROOT + "/Materials/" + name
                        for name in ("M_Surface", "M_Glass", "M_Grass", "M_Stone")]
    for path in required:
        require(ASSETS.does_asset_exist(path), "Required asset absent: " + path)
    report = Path(unreal.Paths.project_saved_dir()) / "WorkshopBootstrap.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({"success": True, "assets": required}, indent=2), encoding="utf-8")
    unreal.log("PROXIMA_WORKSHOP_BOOTSTRAP_OK")


build()
