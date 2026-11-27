"""Etape 0 : cree une carte minimale /Game/Maps/L_Cove (sol + lumiere + ciel)."""
import unreal

MAP = "/Game/Maps/L_Cove"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    les.load_level(MAP)
else:
    les.new_level(MAP)

for a in eas.get_all_level_actors():
    eas.destroy_actor(a)

floor = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
floor.set_actor_scale3d(unreal.Vector(80, 80, 1))
floor.set_actor_label("Floor")

sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, 30))
sun.set_actor_label("Sun")
eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500)).set_actor_label("SkyLight")
eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 200)).set_actor_label("PlayerStart")

les.save_current_level()
unreal.log("DS_STEP0_OK map saved: " + MAP)
