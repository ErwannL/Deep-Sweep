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

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.DSMenuGameMode)
les.save_current_level()
unreal.log("DS_MAP_MENU_OK")
