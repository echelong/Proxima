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
    unreal.log("PROXIMA_BOOTSTRAP_REVISION=residential-lot-1")
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

    # ------------------------------------------------------------------
    # NORMALIZE THE EXISTING WORKSHOP LOT
    # ------------------------------------------------------------------
    #
    # The original prototype used a 2.2 km x 2.2 km cube as temporary
    # ground. It was useful during the earliest tests but looks like an
    # enormous artificial floor when the build camera pulls upward.
    #
    # Keep the construction plane itself at Z=0, but present a deliberate
    # residential lot instead.
    #
    # LOT:
    #   40 m x 40 m
    #   surface exactly at Z=0
    #
    # The build cursor does NOT depend on collision with this mesh.
    # ProximaBuildPlaneTrace intersects the mathematical horizontal plane.
    #
    cube = require(
        ASSETS.load_asset("/Engine/BasicShapes/Cube"),
        "Engine cube missing"
    )

    def find_level_actor(*labels):
        wanted = set(labels)

        for actor in actors.get_all_level_actors():
            if actor.get_actor_label() in wanted:
                return actor

        return None

    def ensure_workshop_box(
        name,
        center,
        size,
        finish,
        aliases=(),
        collision_profile="BlockAll",
    ):
        actor = find_level_actor(
            name,
            *aliases
        )

        if actor is None:
            actor = require(
                actors.spawn_actor_from_class(
                    unreal.StaticMeshActor,
                    unreal.Vector(*center),
                    unreal.Rotator(0, 0, 0)
                ),
                "Could not create Workshop actor: " + name
            )

        if actor.get_actor_label() != name:
            actor.set_actor_label(name)

        actor.set_actor_location(
            unreal.Vector(*center),
            False,
            False
        )

        actor.set_actor_scale3d(
            unreal.Vector(
                *(value / 100.0 for value in size)
            )
        )

        mesh = require(
            actor.get_component_by_class(
                unreal.StaticMeshComponent
            ),
            "Static mesh component missing: " + name
        )

        mesh.set_static_mesh(cube)
        mesh.set_material(0, finish)
        mesh.set_collision_profile_name(
            collision_profile
        )

        return actor

    # The old actor is deliberately reused and renamed.
    #
    # 20 cm thick:
    # centre Z=-10 -> top surface Z=0.
    ensure_workshop_box(
        "Residential lot",
        (0, 0, -10),
        (4000, 4000, 20),
        grass,
        aliases=("Landscape base",),
        collision_profile="BlockAll",
    )

    # Simple 2.4 m wide stone approach.
    ensure_workshop_box(
        "Entry path",
        (0, -1500, 1),
        (240, 1000, 2),
        stone,
        collision_profile="NoCollision",
    )

    # Thin perimeter strips give Build Mode a readable property boundary
    # without creating physical walls or interfering with placement.
    boundary_specs = (
        (
            "Lot boundary north",
            (0, 1996, 1),
            (4000, 8, 2),
        ),
        (
            "Lot boundary south",
            (0, -1996, 1),
            (4000, 8, 2),
        ),
        (
            "Lot boundary east",
            (1996, 0, 1),
            (8, 4000, 2),
        ),
        (
            "Lot boundary west",
            (-1996, 0, 1),
            (8, 4000, 2),
        ),
    )

    for boundary_name, boundary_center, boundary_size in boundary_specs:
        ensure_workshop_box(
            boundary_name,
            boundary_center,
            boundary_size,
            stone,
            collision_profile="NoCollision",
        )

    require(
        level.save_current_level(),
        "Could not save normalized Workshop lot"
    )

    required = [MAP] + [ASSET_ROOT + "/Materials/" + name
                        for name in ("M_Surface", "M_Glass", "M_Grass", "M_Stone")]
    for path in required:
        require(ASSETS.does_asset_exist(path), "Required asset absent: " + path)
    report = Path(unreal.Paths.project_saved_dir()) / "WorkshopBootstrap.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({"success": True, "assets": required}, indent=2), encoding="utf-8")
    unreal.log("PROXIMA_WORKSHOP_BOOTSTRAP_OK")


build()
