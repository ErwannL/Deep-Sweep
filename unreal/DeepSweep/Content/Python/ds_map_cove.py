"""Genere la crique du Lagon (/Game/Maps/L_Cove), ~80 x 80 m, GDD 10.1 / 15.

Reperes (cm) : arene X,Y in [-4000, 4000], fond z=0, surface z=2000.
La base (comptoir, aspirateur...) sera au sud : X ~ -3200.
Relancer ce script regenere entierement la carte (deterministe, graine fixe).
"""
import math
import random
import unreal

MAP = "/Game/Maps/L_Cove"
ARENA = 4000.0
SURFACE_Z = 2000.0
BASE_X = -3200.0

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
V = unreal.Vector
R = unreal.Rotator
rng = random.Random(1337)

if EAL.does_asset_exist(MAP):
    les.load_level(MAP)
else:
    les.new_level(MAP)
for a in eas.get_all_level_actors():
    eas.destroy_actor(a)

MESH = {n: unreal.load_asset(f"/Engine/BasicShapes/{n}") for n in ("Plane", "Cube", "Sphere", "Cylinder", "Cone")}
MAT = lambda n: unreal.load_asset(f"/Game/Materials/{n}")
MI = lambda n: unreal.load_asset(f"/Game/Materials/Instances/{n}")


def mesh(name, shape, loc, scale, mat=None, rot=R(0, 0, 0), collide=True, folder="Decor"):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    smc = a.static_mesh_component
    smc.set_static_mesh(MESH[shape])
    if mat:
        smc.set_material(0, mat)
    a.set_actor_scale3d(scale)
    a.set_actor_label(name)
    a.set_folder_path(folder)
    if not collide:
        smc.set_collision_profile_name("NoCollision")
    return a


# ---------------------------------------------------------------- Sol & surface
mesh("SeaFloor", "Plane", V(0, 0, 0), V(120, 120, 1), MAT("M_DSSand"), folder="Terrain")
# Dunes douces (spheres tres aplaties, a moitie enterrees)
for i in range(14):
    x, y = rng.uniform(-3400, 3400), rng.uniform(-3400, 3400)
    if x < BASE_X + 1200 and abs(y) < 1500:
        continue
    s = rng.uniform(8, 18)
    mesh(f"Dune{i}", "Sphere", V(x, y, -s * 50 * 0.75), V(s, s * rng.uniform(0.7, 1.3), s * 0.22), MAT("M_DSSand"), R(0, 0, rng.uniform(0, 360)), folder="Terrain")
surf = mesh("WaterSurface", "Plane", V(0, 0, SURFACE_Z), V(160, 160, 1), MAT("M_DSSurface"), R(180, 0, 0), collide=False, folder="Terrain")
surf.static_mesh_component.set_cast_shadow(False)

# Limites invisibles (murs + plafond)
for i, (loc, sc) in enumerate([(V(ARENA + 50, 0, 1000), V(1, 84, 22)), (V(-ARENA - 50, 0, 1000), V(1, 84, 22)),
                               (V(0, ARENA + 50, 1000), V(84, 1, 22)), (V(0, -ARENA - 50, 1000), V(84, 1, 22)),
                               (V(0, 0, SURFACE_Z - 80), V(84, 84, 1))]):
    w = mesh(f"Bound{i}", "Cube", loc, sc, folder="Bounds")
    w.set_actor_hidden_in_game(True)
    w.static_mesh_component.set_cast_shadow(False)
    w.static_mesh_component.set_visibility(False)

# ---------------------------------------------------------------- Rochers
rock_mats = [MI("MI_Rock"), MI("MI_RockDark")]
for i in range(46):  # anneau autour de l'arene (comme le prototype)
    a = i / 46 * math.tau + rng.uniform(-0.05, 0.05)
    d = ARENA + rng.uniform(100, 700)
    s = rng.uniform(4, 9)
    mesh(f"RockRing{i}", "Sphere", V(math.cos(a) * d, math.sin(a) * d, s * 15), V(s, s * rng.uniform(0.7, 1.2), s * rng.uniform(0.6, 1.1)),
         rng.choice(rock_mats), R(rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(0, 360)), folder="Decor/Rocks")
for i in range(18):  # rochers isoles a l'interieur
    x, y = rng.uniform(-3600, 3600), rng.uniform(-3600, 3600)
    if x < BASE_X + 1000 and abs(y) < 1800:
        continue
    s = rng.uniform(1.5, 3.5)
    mesh(f"Rock{i}", "Sphere", V(x, y, s * 25), V(s, s * rng.uniform(0.6, 1.3), s * 0.7), rng.choice(rock_mats),
         R(rng.uniform(-25, 25), 0, rng.uniform(0, 360)), folder="Decor/Rocks")

# ---------------------------------------------------------------- Coraux & algues
corals = [MI(f"MI_Coral{i}") for i in range(6)]
for i in range(26):
    x, y = rng.uniform(-3600, 3600), rng.uniform(-3600, 3600)
    if x < BASE_X + 1000 and abs(y) < 1800:
        continue
    m = rng.choice(corals)
    for j in range(rng.randint(3, 6)):
        ox, oy = rng.uniform(-120, 120), rng.uniform(-120, 120)
        h = rng.uniform(1.2, 3.2)
        kind = rng.random()
        if kind < 0.45:   # cone (corail corne)
            mesh(f"Coral{i}_{j}", "Cone", V(x + ox, y + oy, h * 50), V(0.5, 0.5, h), m, R(rng.uniform(-15, 15), rng.uniform(-15, 15), 0), folder="Decor/Coral")
        elif kind < 0.8:  # tube + boule
            mesh(f"Coral{i}_{j}", "Cylinder", V(x + ox, y + oy, h * 40), V(0.3, 0.3, h * 0.8), m, R(rng.uniform(-20, 20), 0, 0), folder="Decor/Coral")
            mesh(f"Coral{i}_{j}b", "Sphere", V(x + ox, y + oy, h * 80), V(0.6, 0.6, 0.6), m, folder="Decor/Coral")
        else:             # corail cerveau
            s = rng.uniform(1, 1.8)
            mesh(f"Coral{i}_{j}", "Sphere", V(x + ox, y + oy, s * 25), V(s, s, s * 0.7), m, folder="Decor/Coral")
kelp = MI("MI_Kelp")
for i in range(40):  # algues hautes pres des bords
    a = rng.uniform(0, math.tau)
    d = rng.uniform(2800, 3800)
    x, y = math.cos(a) * d, math.sin(a) * d
    if x < BASE_X + 600:
        continue
    h = rng.uniform(5, 12)
    mesh(f"Kelp{i}", "Cylinder", V(x, y, h * 50), V(0.18, 0.35, h), kelp, R(rng.uniform(-6, 6), rng.uniform(-6, 6), rng.uniform(0, 360)), collide=False, folder="Decor/Kelp")

# ---------------------------------------------------------------- Base (emplacement)
mesh("BasePad", "Cube", V(BASE_X, 0, 10), V(14, 34, 0.3), MI("MI_RockDark"), folder="Base")

# ---------------------------------------------------------------- Courant & colonne de bulles
cur = eas.spawn_actor_from_class(unreal.DSWaterVolume, V(800, 0, 900), R(0, 0, 0))
cur.set_actor_label("Current_East")
cur.set_folder_path("Water")
cur.set_editor_property("kind", unreal.DSWaterVolumeKind.CURRENT)
cur.set_editor_property("acceleration", V(0, 350, 0))
cur.get_component_by_class(unreal.BoxComponent).set_box_extent(V(300, 3900, 900))

col = eas.spawn_actor_from_class(unreal.DSWaterVolume, V(2000, -2000, 1000), R(0, 0, 0))
col.set_actor_label("BubbleColumn")
col.set_folder_path("Water")
col.set_editor_property("kind", unreal.DSWaterVolumeKind.BUBBLE_COLUMN)
col.set_editor_property("acceleration", V(0, 0, 1100))
col.get_component_by_class(unreal.BoxComponent).set_box_extent(V(220, 220, 1000))
mesh("BubbleVent", "Sphere", V(2000, -2000, 30), V(3, 3, 0.9), MI("MI_RockDark"), folder="Water")

# ---------------------------------------------------------------- Rayons de lumiere (stylises)
ray_mat = MAT("M_DSLightRay")
for i in range(16):
    x, y = rng.uniform(-3200, 3600), rng.uniform(-3500, 3500)
    w = rng.uniform(2.5, 6)
    r = mesh(f"LightRay{i}", "Cone", V(x, y, SURFACE_Z - 1000), V(w, w * rng.uniform(0.6, 1), 20), ray_mat,
             R(rng.uniform(-14, -8), 0, 35), collide=False, folder="Decor/LightRays")
    r.static_mesh_component.set_cast_shadow(False)

# ---------------------------------------------------------------- Lumiere & atmosphere sous-marine
sun = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 3000), R(0, -62, 35))
sun.set_actor_label("Sun")
lc = sun.light_component
lc.set_editor_property("intensity", 7.0)
lc.set_editor_property("light_color", unreal.Color(r=200, g=240, b=255, a=255))
lc.set_editor_property("atmosphere_sun_light", True)
lc.set_editor_property("enable_light_shaft_bloom", True)
lc.set_editor_property("bloom_scale", 0.35)
lc.set_editor_property("bloom_threshold", 0.05)
lc.set_editor_property("volumetric_scattering_intensity", 4.0)

eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0)).set_actor_label("SkyAtmosphere")
sky = eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500))
sky.set_actor_label("SkyLight")
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("intensity", 1.0)
sky.light_component.set_editor_property("light_color", unreal.Color(r=90, g=190, b=255, a=255))

fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0))
fog.set_actor_label("UnderwaterFog")
fc = fog.component
fc.set_editor_property("fog_density", 0.25)
fc.set_editor_property("fog_height_falloff", 0.0002)
fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.01, 0.10, 0.18, 1))
fc.set_editor_property("fog_max_opacity", 1.0)
fc.set_editor_property("start_distance", 0.0)
fc.set_editor_property("enable_volumetric_fog", True)
fc.set_editor_property("volumetric_fog_albedo", unreal.Color(r=40, g=140, b=210, a=255))
fc.set_editor_property("volumetric_fog_extinction_scale", 2.5)
fc.set_editor_property("volumetric_fog_distance", 6000.0)

pp = eas.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0))
pp.set_actor_label("UnderwaterPost")
pp.set_editor_property("unbound", True)
s = pp.settings
s.set_editor_property("override_auto_exposure_method", True)
s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
s.set_editor_property("override_auto_exposure_bias", True)
s.set_editor_property("auto_exposure_bias", 8.5)
s.set_editor_property("override_color_saturation", True)
s.set_editor_property("color_saturation", unreal.Vector4(1.15, 1.15, 1.15, 1))
s.set_editor_property("override_color_gain", True)
s.set_editor_property("color_gain", unreal.Vector4(0.92, 1.0, 1.08, 1))
s.set_editor_property("override_bloom_intensity", True)
s.set_editor_property("bloom_intensity", 0.8)
s.set_editor_property("override_vignette_intensity", True)
s.set_editor_property("vignette_intensity", 0.45)
pp.set_editor_property("settings", s)

# ---------------------------------------------------------------- Objets (150, 6 types) : manager dormant/actif
# Tas repris du prototype (x,z three.js -> X=-z, Y=x, en cm).
pm = eas.spawn_actor_from_class(unreal.DSPickupManager, V(0, 0, 0))
pm.set_actor_label("PickupManager")
pm.set_folder_path("Gameplay")
pm.set_editor_property("types", [unreal.load_asset(f"/Game/Data/Pickups/DA_{n}") for n in
                                 ("Canette", "Bouteille", "Ferraille", "Pneu", "Caddie", "Vase")])
pm.set_editor_property("pile_centers", [V(2200, -2200, 0), V(1800, 2000, 0), V(2500, -500, 0),
                                        V(-800, 2500, 0), V(-800, -2500, 0), V(-500, 500, 0)])
pm.set_editor_property("pile_radius", 700.0)
pm.set_editor_property("surface_z", SURFACE_Z - 100)

# ---------------------------------------------------------------- Base : comptoir (prototype : x=+14, z=30)
counter = eas.spawn_actor_from_class(unreal.DSCounter, V(BASE_X + 200, 1400, 0), R(0, 180, 0))
counter.set_actor_label("Counter")
counter.set_folder_path("Base")

# ---------------------------------------------------------------- Base : aspirateur a 2 postes (prototype : cuve x=-14)
noz = eas.spawn_actor_from_class(unreal.DSVacuumNozzle, V(BASE_X + 600, -1200, 45), R(0, 0, 0))
noz.set_actor_label("VacuumNozzle")
noz.set_folder_path("Base/Vacuum")
vac = eas.spawn_actor_from_class(unreal.DSVacuum, V(BASE_X + 200, -1400, 0), R(0, 0, 0))
vac.set_actor_label("Vacuum")
vac.set_folder_path("Base/Vacuum")
vac.set_editor_property("nozzle", noz)
lever = eas.spawn_actor_from_class(unreal.DSVacuumLever, V(BASE_X - 50, -1050, 0), R(0, 0, 0))
lever.set_actor_label("VacuumLever")
lever.set_folder_path("Base/Vacuum")
lever.set_editor_property("vacuum", vac)
cons = eas.spawn_actor_from_class(unreal.DSVacuumConsole, V(BASE_X - 200, -1750, 0), R(0, 0, 0))
cons.set_actor_label("VacuumConsole")
cons.set_folder_path("Base/Vacuum")
cons.set_editor_property("vacuum", vac)
vac.set_editor_property("lever_actor", lever)
vac.set_editor_property("console_actor", cons)

# ---------------------------------------------------------------- Base : convoyeur cuve -> comptoir (rampe vers la tremie)
belt = eas.spawn_actor_from_class(unreal.DSConveyor, V(BASE_X + 200, -1050, 60), R(0, 0, 0))
belt.set_actor_label("Conveyor")
belt.set_folder_path("Base")
belt.set_editor_property("counter", counter)
spl = belt.get_component_by_class(unreal.SplineComponent)
spl.clear_spline_points(False)
for p in (V(BASE_X + 200, -1080, 60), V(BASE_X + 200, 400, 60), V(BASE_X + 200, 1000, 230), V(BASE_X + 200, 1250, 330)):
    spl.add_spline_point(p, unreal.SplineCoordinateSpace.WORLD, False)
spl.update_spline()

# ---------------------------------------------------------------- Ambiance sale -> coloree (proprete)
amb = eas.spawn_actor_from_class(unreal.DSAmbienceController, V(0, 0, 0))
amb.set_actor_label("AmbienceController")
amb.set_folder_path("Gameplay")
amb.set_editor_property("fog", fog)
amb.set_editor_property("post_process", pp)
amb.set_editor_property("sun", sun)
amb.set_editor_property("collection", unreal.load_asset("/Game/Materials/MPC_DS"))

# ---------------------------------------------------------------- Departs joueurs (4)
for i in range(4):
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, V(BASE_X + 800, -450 + i * 300, 250), R(0, 0, 0))
    ps.set_actor_label(f"PlayerStart{i}")
    ps.set_folder_path("Base")

les.save_current_level()
unreal.log("DS_MAP_COVE_OK")
