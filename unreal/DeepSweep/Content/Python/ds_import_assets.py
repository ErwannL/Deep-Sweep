"""Import des assets CC0 telecharges (unreal/SourceAssets) + modeles composes des dechets.

- Textures PBR Poly Haven (sable, sable+rochers, roche) -> /Game/Textures
- Sons Kenney (impacts, interface) -> /Game/Audio/Sfx
- Modeles lisibles des dechets (caddie a roues, pneu en anneau, bouteille avec goulot...) construits
  a partir de formes de base fusionnees -> /Game/Meshes/Pickups (avec collision simple)
"""
import math
import os
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", "SourceAssets"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
V = unreal.Vector
R = unreal.Rotator


def import_files(files, dest):
    tasks = []
    for f in files:
        t = unreal.AssetImportTask()
        t.filename = f
        t.destination_path = dest
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    return [p for t in tasks for p in t.imported_object_paths]


SKIP_MEDIA = True  # textures et sons deja importes
# ------------------------------------------------------------------ Textures
tex_dir = os.path.join(ROOT, "polyhaven")
imported = import_files([os.path.join(tex_dir, f) for f in sorted(os.listdir(tex_dir)) if f.endswith(".jpg")], "/Game/Textures")
for p in imported:
    tex = unreal.load_asset(p)
    name = p.lower()
    if "_nor_" in name:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif "_arm_" in name:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    EAL.save_loaded_asset(tex)
unreal.log(f"DS_IMPORT textures : {len(imported)}")

# ------------------------------------------------------------------ Sons
SFX = {
    "impact-sounds": ["impactMetal_heavy_000", "impactMetal_heavy_001", "impactPlate_heavy_000", "impactPlate_heavy_001",
                      "impactGlass_heavy_000", "impactGlass_heavy_001", "impactSoft_medium_000", "impactSoft_medium_001",
                      "impactSoft_medium_002", "impactSoft_heavy_000", "impactSoft_heavy_001", "impactPunch_heavy_000",
                      "impactGeneric_light_000", "impactGeneric_light_001", "impactTin_medium_000", "impactTin_medium_001"],
    "interface-sounds": ["pluck_001", "pluck_002", "select_001", "select_002", "select_003", "confirmation_001",
                         "confirmation_002", "confirmation_004", "error_001", "error_006", "drop_001", "drop_002",
                         "click_001", "switch_002", "maximize_006", "glitch_002", "question_001"],
    "ui-audio": ["rollover2", "click3"],
}
files = []
for pack, names in SFX.items():
    for dirpath, _, fnames in os.walk(os.path.join(ROOT, "kenney", pack)):
        for n in names:
            if n + ".ogg" in fnames:
                files.append(os.path.join(dirpath, n + ".ogg"))
sounds = import_files(files, "/Game/Audio/Sfx")
unreal.log(f"DS_IMPORT sons : {len(sounds)} / {sum(len(v) for v in SFX.values())}")

# ------------------------------------------------------------------ Modeles des dechets (Geometry Script)
GP = unreal.GeometryScript_Primitives
OPT = unreal.GeometryScriptPrimitiveOptions()
ORIGIN = unreal.GeometryScriptPrimitiveOriginMode.CENTER


def T(x=0, y=0, z=0, pitch=0, yaw=0, roll=0):
    return unreal.Transform(V(x, y, z), R(roll=roll, pitch=pitch, yaw=yaw), V(1, 1, 1))


def box(dm, sx, sy, sz, **t): GP.append_box(dm, OPT, T(**t), sx, sy, sz, 0, 0, 0, ORIGIN)
def cyl(dm, r, h, **t): GP.append_cylinder(dm, OPT, T(**t), r, h, 16, 0, True, ORIGIN)
def sph(dm, r, **t): GP.append_sphere_lat_long(dm, OPT, T(**t), r, 12, 16, ORIGIN)
def torus(dm, R_, r, **t): GP.append_torus(dm, OPT, T(**t), unreal.GeometryScriptRevolveOptions(), R_, r, 20, 10, ORIGIN)


def build(name, fn, collision=unreal.ScriptCollisionShapeType.BOX, scale_xyz=None):
    path = f"/Game/Meshes/Pickups/SM_{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    dm = unreal.DynamicMesh()
    fn(dm)
    if scale_xyz:
        unreal.GeometryScript_MeshTransforms.scale_mesh(dm, V(*scale_xyz), V(0, 0, 0))
    unreal.GeometryScript_Normals.compute_split_normals(dm, unreal.GeometryScriptSplitNormalsOptions(), unreal.GeometryScriptCalculateNormalsOptions())
    unreal.GeometryScript_UVs.set_mesh_u_vs_from_box_projection(dm, 0, T(), unreal.GeometryScriptMeshSelection(), 2) if hasattr(unreal.GeometryScript_UVs, "set_mesh_u_vs_from_box_projection") else None
    opts = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    opts.set_editor_property("enable_collision", False)
    mesh, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dm, path, opts)
    mesh.set_material(0, COLOR)
    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, collision)
    EAL.save_loaded_asset(mesh)
    unreal.log(f"DS_IMPORT modele {name} : {outcome}")


COLOR = unreal.load_asset("/Game/Materials/M_DSColor")

build("Canette", lambda d: (cyl(d, 21, 58), cyl(d, 18, 4, z=30), box(d, 12, 6, 2, x=6, z=33)), unreal.ScriptCollisionShapeType.NDOP10_Z)
build("Bouteille", lambda d: (cyl(d, 17, 60, z=-12), sph(d, 17, z=18), cyl(d, 6.5, 24, z=36), cyl(d, 8, 7, z=50)), unreal.ScriptCollisionShapeType.NDOP10_Z)
build("Ferraille", lambda d: (box(d, 85, 60, 10), box(d, 60, 45, 8, x=20, y=5, z=14, pitch=32, yaw=15), cyl(d, 6, 80, x=-10, y=-12, z=10, yaw=70, roll=90)))
build("Pneu", lambda d: (torus(d, 52, 24), cyl(d, 32, 14)), unreal.ScriptCollisionShapeType.NDOP26)


def cart(d):
    box(d, 110, 70, 5)
    box(d, 110, 4, 60, y=35, z=28); box(d, 110, 4, 60, y=-35, z=28)
    box(d, 4, 70, 60, x=55, z=28); box(d, 4, 70, 60, x=-55, z=28)
    cyl(d, 3, 75, x=-70, z=62, roll=90)
    for x in (-45, 45):
        for y in (-28, 28):
            cyl(d, 2.5, 42, x=x, y=y, z=-22)
            cyl(d, 8, 6, x=x, y=y, z=-45, roll=90)


build("Caddie", cart)
build("Vase", lambda d: (sph(d, 48, z=0), cyl(d, 25, 10, z=-50), cyl(d, 16, 32, z=58), cyl(d, 24, 7, z=76)), unreal.ScriptCollisionShapeType.NDOP26)

# Data Assets : nouveaux modeles, deja a la bonne taille
for n in ("Canette", "Bouteille", "Ferraille", "Pneu", "Caddie", "Vase"):
    da = unreal.load_asset(f"/Game/Data/Pickups/DA_{n}")
    da.set_editor_property("mesh", unreal.load_asset(f"/Game/Meshes/Pickups/SM_{n}"))
    da.set_editor_property("mesh_scale", V(1, 1, 1))
    EAL.save_loaded_asset(da)

unreal.log("DS_IMPORT_OK")
