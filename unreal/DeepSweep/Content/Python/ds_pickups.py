"""Data Assets des 6 types d'objets du MVP (GDD 11 / prototype).

Formes simples et surdimensionnees (GDD 13 : une canette fait 2x sa taille).
Code couleur : dechet gris/bleu, recyclable vert, metal orange, precieux dore brillant.
Sink : +1 coule, 0 neutre, -1 remonte.
"""
import unreal

EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Data/Pickups"
LC = unreal.LinearColor
V = unreal.Vector
Cat = unreal.DSPickupCategory
mesh = lambda n: unreal.load_asset(f"/Engine/BasicShapes/{n}")

TYPES = [
    # nom, categorie, forme, echelle, couleur, emissif, valeur, kg, sink, rayon, nombre, fragile, bouche
    ("Canette",   Cat.TRASH,      "Cylinder", V(0.42, 0.42, 0.62), LC(0.62, 0.7, 0.82, 1), 0.0, 1,  2.0,  0.6, 35, 50, False, False),
    ("Bouteille", Cat.RECYCLABLE, "Cylinder", V(0.34, 0.34, 0.95), LC(0.15, 0.85, 0.35, 1), 0.0, 2,  1.5, -0.5, 40, 40, False, False),
    ("Ferraille", Cat.METAL,      "Cube",     V(0.8, 0.6, 0.7),    LC(1.0, 0.42, 0.08, 1),  0.0, 4, 15.0,  1.0, 50, 30, False, False),
    ("Pneu",      Cat.TRASH,      "Cylinder", V(1.5, 1.5, 0.5),    LC(0.08, 0.09, 0.12, 1), 0.0, 8, 25.0,  0.8, 80, 15, False, False),
    ("Caddie",    Cat.METAL,      "Cube",     V(1.6, 1.1, 1.0),    LC(0.85, 0.3, 0.05, 1),  0.0, 12, 60.0, 1.0, 100, 8, False, True),
    ("Vase",      Cat.PRECIOUS,   "Sphere",   V(1.0, 1.0, 1.25),   LC(1.0, 0.78, 0.15, 1),  0.6, 25, 12.0, 0.7, 55, 7, True, False),
]

for (name, cat, shape, scale, color, emi, value, kg, sink, radius, count, fragile, clogs) in TYPES:
    path = f"{FOLDER}/DA_{name}"
    if EAL.does_asset_exist(path):
        da = unreal.load_asset(path)
    else:
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", unreal.DSPickupData)
        da = tools.create_asset(f"DA_{name}", FOLDER, unreal.DSPickupData, f)
    for k, v in dict(type_id=name, display_name=name, category=cat, mesh=mesh(shape), mesh_scale=scale,
                     color=color, emissive=emi, value=value, mass_kg=kg, sink=sink, radius=float(radius),
                     spawn_count=count, fragile=fragile, clogs=clogs).items():
        da.set_editor_property(k, v)
    EAL.save_loaded_asset(da)

total = sum(t[10] for t in TYPES)
unreal.log(f"DS_PICKUPS_OK {len(TYPES)} types, {total} objets")
