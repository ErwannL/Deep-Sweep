"""Carte du menu principal (/Game/Maps/L_Menu) : fond sous-marin simple + mode de jeu menu."""
import unreal

MAP = "/Game/Maps/L_Menu"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
V = unreal.Vector
R = unreal.Rotator

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    les.load_level(MAP)
else:
    les.new_level(MAP)
for a in eas.get_all_level_actors():
    eas.destroy_actor(a)

floor = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(0, 0, -300))
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
floor.static_mesh_component.set_material(0, unreal.load_asset("/Game/Materials/M_DSSand"))
floor.set_actor_scale3d(V(100, 100, 1))
sun = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1000), R(0, -60, 30))
sun.light_component.set_editor_property("intensity", 6.0)
fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0))
fog.component.set_editor_property("fog_density", 0.2)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.01, 0.12, 0.22, 1))
sky = eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500))
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
eas.spawn_actor_from_class(unreal.PlayerStart, V(0, 0, 0))

import math, random
rng = random.Random(7)
MI = lambda n: unreal.load_asset(f"/Game/Materials/Instances/{n}")
def deco(shape, loc, scale, mat, rot=R(0, 0, 0)):
    m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    m.static_mesh_component.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{shape}"))
    m.static_mesh_component.set_material(0, mat)
    m.set_actor_scale3d(scale)
    return m
floor.set_actor_location(V(0, 0, 0), False, False)
corals = [MI(f"MI_Coral{i}") for i in range(6)]
for i in range(16):
    a = i / 16 * math.tau + rng.uniform(-0.2, 0.2); d = rng.uniform(350, 900)
    x, y = math.cos(a) * d, math.sin(a) * d
    m = rng.choice(corals)
    for j in range(rng.randint(3, 5)):
        h = rng.uniform(1.2, 3.5)
        ox, oy = rng.uniform(-110, 110), rng.uniform(-110, 110)
        if rng.random() < 0.5:
            deco("Cone", V(x + ox, y + oy, h * 50), V(0.5, 0.5, h), m, R(rng.uniform(-15, 15), rng.uniform(-15, 15), 0))
        else:
            deco("Cylinder", V(x + ox, y + oy, h * 40), V(0.3, 0.3, h * 0.8), m)
            deco("Sphere", V(x + ox, y + oy, h * 80), V(0.6, 0.6, 0.6), m)
for i in range(10):
    a = rng.uniform(0, math.tau); d = rng.uniform(1800, 2600); s = rng.uniform(3, 7)
    deco("Sphere", V(math.cos(a) * d, math.sin(a) * d, s * 15), V(s, s, s * 0.8), MI("MI_Rock"), R(0, 0, rng.uniform(0, 360)))
for i in range(14):
    a = rng.uniform(0, math.tau); d = rng.uniform(1600, 2400); h = rng.uniform(5, 11)
    deco("Cylinder", V(math.cos(a) * d, math.sin(a) * d, h * 50), V(0.18, 0.35, h), MI("MI_Kelp"), R(rng.uniform(-6, 6), 0, 0))
ray = unreal.load_asset("/Game/Materials/M_DSLightRay")
for i in range(7):
    w = rng.uniform(2.5, 5)
    deco("Cone", V(rng.uniform(-1500, 1500), rng.uniform(-1500, 1500), 1000), V(w, w, 20), ray, R(-10, 0, 35))
surf = deco("Plane", V(0, 0, 2000), V(80, 80, 1), unreal.load_asset("/Game/Materials/M_DSSurface"), R(180, 0, 0))
col = eas.spawn_actor_from_class(unreal.DSWaterVolume, V(300, -300, 1000))
col.set_editor_property("kind", unreal.DSWaterVolumeKind.BUBBLE_COLUMN)
col.set_editor_property("acceleration", V(0, 0, 1100))
col.get_component_by_class(unreal.BoxComponent).set_box_extent(V(200, 200, 1000))
# Les vedettes : quelques dechets colores et un plongeur
for i, (shape, sc, mi) in enumerate([("Cylinder", V(0.42, 0.42, 0.62), "MI_Coral3"), ("Cube", V(0.8, 0.6, 0.7), "MI_Coral1"),
                                     ("Sphere", V(1.0, 1.0, 1.25), "MI_Coral4"), ("Cylinder", V(1.5, 1.5, 0.5), "MI_RockDark")]):
    deco(shape, V(-200 + i * 150, 250 - i * 90, 40 + i * 30), sc, MI(mi), R(rng.uniform(-30, 30), rng.uniform(0, 360), 0))
diver = eas.spawn_actor_from_class(unreal.DSDiver, V(0, 0, 260), R(0, 200, 0))
sun.light_component.set_editor_property("intensity", 9.0)
sun.light_component.set_editor_property("light_color", unreal.Color(r=200, g=240, b=255, a=255))
fog.component.set_editor_property("fog_density", 0.14)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.01, 0.12, 0.22, 1))
fog.component.set_editor_property("enable_volumetric_fog", True)
fog.component.set_editor_property("volumetric_fog_albedo", unreal.Color(r=40, g=140, b=210, a=255))
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("light_color", unreal.Color(r=90, g=190, b=255, a=255))
eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0))
pp = eas.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0))
pp.set_editor_property("unbound", True)
ps_ = pp.settings
ps_.set_editor_property("override_auto_exposure_method", True)
ps_.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
ps_.set_editor_property("override_auto_exposure_bias", True)
ps_.set_editor_property("auto_exposure_bias", 8.5)
ps_.set_editor_property("override_color_saturation", True)
ps_.set_editor_property("color_saturation", unreal.Vector4(1.25, 1.25, 1.25, 1))
ps_.set_editor_property("override_vignette_intensity", True)
ps_.set_editor_property("vignette_intensity", 0.5)
pp.set_editor_property("settings", ps_)
eas.spawn_actor_from_class(unreal.CameraActor, V(1700, 0, 500), R(-12, 180, 0))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.DSMenuGameMode)
les.save_current_level()
unreal.log("DS_MAP_MENU_OK")
