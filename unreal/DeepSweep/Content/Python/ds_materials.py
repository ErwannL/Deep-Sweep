"""Materiaux du projet, generes par script (style cartoon, couleurs unies).

- M_DSColor   : couleur unie parametrable (plongeur, objets).
- MPC_DS      : collection globale ; 'Clean' (0..1) = % de proprete -> le monde passe de sale a colore.
- M_DSWorld   : couleur qui se desature/assombrit quand Clean = 0 (rochers, coraux, algues).
- M_DSSand    : sable avec variation de bruit, gris-boue quand sale.
- M_DSBubble  : bulle translucide brillante.
- M_DSSurface : surface de l'eau vue d'en dessous (non eclairee, lumineuse).
"""
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Materials"
MP = unreal.MaterialProperty


def fresh(name, cls, factory, folder=FOLDER):
    path = f"{folder}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return tools.create_asset(name, folder, cls, factory)


def expr(mat, cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def vparam(mat, name, color, x, y):
    return expr(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=color)


def sparam(mat, name, value, x, y):
    return expr(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def const(mat, value, x, y):
    return expr(mat, unreal.MaterialExpressionConstant, x, y, r=value)


def finish(mat):
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def make_mpc():
    path = f"{FOLDER}/MPC_DS"
    mpc = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
        tools.create_asset("MPC_DS", FOLDER, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    p = unreal.CollectionScalarParameter()
    p.set_editor_property("parameter_name", "Clean")
    p.set_editor_property("default_value", 0.0)
    mpc.set_editor_property("scalar_parameters", [p])
    EAL.save_loaded_asset(mpc)
    return mpc


def clean_alpha(mat, mpc, x, y):
    c = expr(mat, unreal.MaterialExpressionCollectionParameter, x, y, collection=mpc, parameter_name="Clean")
    return c


def dirty_of(mat, color_expr, x, y, fraction=0.85, darken=0.6):
    d = expr(mat, unreal.MaterialExpressionDesaturation, x, y)
    MEL.connect_material_expressions(color_expr, "", d, "")
    MEL.connect_material_expressions(const(mat, fraction, x - 150, y + 80), "", d, "Fraction")
    m = expr(mat, unreal.MaterialExpressionMultiply, x + 150, y)
    MEL.connect_material_expressions(d, "", m, "A")
    MEL.connect_material_expressions(const(mat, darken, x, y + 120), "", m, "B")
    return m


def tex(path):
    return unreal.load_asset(path)


def world_uv(mat, axes, scale, x, y):
    """UV projetees en espace monde (axes 'xy', 'xz' ou 'yz') / scale cm."""
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, x - 300, y)
    m = expr(mat, unreal.MaterialExpressionComponentMask, x - 150, y,
             r="x" in axes, g="y" in axes, b="z" in axes, a=False)
    MEL.connect_material_expressions(wp, "", m, "")
    d = expr(mat, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(m, "", d, "A")
    MEL.connect_material_expressions(const(mat, 1.0 / scale, x - 150, y + 60), "", d, "B")
    return d


def sample(mat, texture, uv, x, y, kind="color"):
    t = expr(mat, unreal.MaterialExpressionTextureSample, x, y, texture=texture)
    if kind == "normal":
        t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif kind == "mask":
        t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    MEL.connect_material_expressions(uv, "", t, "UVs")
    return t


def lerp3(mat, a_, b_, alpha, x, y):
    l = expr(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    MEL.connect_material_expressions(a_, "", l, "A")
    MEL.connect_material_expressions(b_, "", l, "B")
    MEL.connect_material_expressions(alpha, "", l, "Alpha")
    return l


def triplanar(mat, texture, scale, x, y, kind="color"):
    """Projection 3 plans (rochers, coraux) : melange XY / XZ / YZ selon la normale monde."""
    n = expr(mat, unreal.MaterialExpressionVertexNormalWS, x - 600, y + 400)
    ab = expr(mat, unreal.MaterialExpressionAbs, x - 450, y + 400)
    MEL.connect_material_expressions(n, "", ab, "")
    ny = expr(mat, unreal.MaterialExpressionComponentMask, x - 300, y + 380, r=False, g=True, b=False, a=False)
    nz = expr(mat, unreal.MaterialExpressionComponentMask, x - 300, y + 440, r=False, g=False, b=True, a=False)
    MEL.connect_material_expressions(ab, "", ny, "")
    MEL.connect_material_expressions(ab, "", nz, "")
    txy = sample(mat, texture, world_uv(mat, "xy", scale, x - 300, y), x, y, kind)
    txz = sample(mat, texture, world_uv(mat, "xz", scale, x - 300, y + 150), x, y + 150, kind)
    tyz = sample(mat, texture, world_uv(mat, "yz", scale, x - 300, y + 300), x, y + 300, kind)
    side = lerp3(mat, tyz, txz, ny, x + 200, y + 200)
    return lerp3(mat, side, txy, nz, x + 350, y + 100)


SAND_D = "/Game/Textures/sand_03_diff_2k"
SAND_N = "/Game/Textures/sand_03_nor_gl_2k"
SAND_M = "/Game/Textures/sand_03_arm_2k"
CSR_D = "/Game/Textures/coast_sand_rocks_02_diff_2k"
CSR_N = "/Game/Textures/coast_sand_rocks_02_nor_gl_2k"
ROCK_D = "/Game/Textures/rocks_ground_02_col_2k"
ROCK_N = "/Game/Textures/rocks_ground_02_nor_gl_2k"
ROCK_M = "/Game/Textures/rocks_ground_02_arm_2k"


def color_material():
    """Objets : couleur du code couleur + metal/rugosite, avec un grain de surface (normal de roche tres attenue)."""
    mat = fresh("M_DSColor", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    col = vparam(mat, "Color", unreal.LinearColor(0.8, 0.8, 0.8, 1), -500, 0)
    MEL.connect_material_property(col, "", MP.MP_BASE_COLOR)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.55, -500, 200), "", MP.MP_ROUGHNESS)
    MEL.connect_material_property(sparam(mat, "Metallic", 0.0, -500, 280), "", MP.MP_METALLIC)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -250, 350)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(sparam(mat, "Emissive", 0.0, -500, 400), "", mul, "B")
    MEL.connect_material_property(mul, "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


def world_material(mpc):
    """Rochers / coraux / algues : roche Poly Haven projetee en 3 plans, teintee par la couleur de l'instance."""
    mat = fresh("M_DSWorld", unreal.Material, unreal.MaterialFactoryNew())
    col = vparam(mat, "Color", unreal.LinearColor(1, 0.4, 0.5, 1), -1500, -300)
    rock = triplanar(mat, tex(ROCK_D), 220.0, -1200, 0)
    gray = expr(mat, unreal.MaterialExpressionDesaturation, -700, 0)
    MEL.connect_material_expressions(rock, "", gray, "")
    MEL.connect_material_expressions(const(mat, 1.0, -850, 80), "", gray, "Fraction")
    boost = expr(mat, unreal.MaterialExpressionMultiply, -550, 0)
    MEL.connect_material_expressions(gray, "", boost, "A")
    MEL.connect_material_expressions(sparam(mat, "Detail", 2.6, -700, 120), "", boost, "B")
    tinted = expr(mat, unreal.MaterialExpressionMultiply, -400, -150)
    MEL.connect_material_expressions(col, "", tinted, "A")
    MEL.connect_material_expressions(boost, "", tinted, "B")
    dirty = dirty_of(mat, tinted, -350, 150, fraction=0.75, darken=0.85)
    out = lerp3(mat, dirty, tinted, clean_alpha(mat, mpc, -350, 350), -100, 0)
    MEL.connect_material_property(out, "", MP.MP_BASE_COLOR)
    nrm = triplanar(mat, tex(ROCK_N), 220.0, -1200, 700, "normal")
    MEL.connect_material_property(nrm, "", MP.MP_NORMAL)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.7, -400, 500), "", MP.MP_ROUGHNESS)
    emi = expr(mat, unreal.MaterialExpressionMultiply, -100, 600)
    k = expr(mat, unreal.MaterialExpressionMultiply, -300, 650)
    MEL.connect_material_expressions(clean_alpha(mat, mpc, -500, 650), "", k, "A")
    MEL.connect_material_expressions(sparam(mat, "CleanGlow", 0.0, -500, 750), "", k, "B")
    MEL.connect_material_expressions(col, "", emi, "A")
    MEL.connect_material_expressions(k, "", emi, "B")
    MEL.connect_material_property(emi, "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


def world_instance(name, parent, color, glow=0.0, rough=0.7):
    folder = f"{FOLDER}/Instances"
    mi = fresh(name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew(), folder)
    mi.set_editor_property("parent", parent)
    MEL.set_material_instance_vector_parameter_value(mi, "Color", color)
    MEL.set_material_instance_scalar_parameter_value(mi, "CleanGlow", glow)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    EAL.save_loaded_asset(mi)


def sand_material(mpc):
    """Fond : sable Poly Haven + plaques de sable rocheux (bruit grande echelle), grisatre quand c'est sale."""
    mat = fresh("M_DSSand", unreal.Material, unreal.MaterialFactoryNew())
    uv = world_uv(mat, "xy", 260.0, -1300, 0)
    uv2 = world_uv(mat, "xy", 520.0, -1300, 400)
    sand = sample(mat, tex(SAND_D), uv, -1000, 0)
    rocks = sample(mat, tex(CSR_D), uv2, -1000, 300)
    patch = expr(mat, unreal.MaterialExpressionNoise, -1000, 600, scale=0.00035, output_min=-0.6, output_max=1.4, levels=2)
    sat = expr(mat, unreal.MaterialExpressionSaturate, -850, 600)
    MEL.connect_material_expressions(patch, "", sat, "")
    base = lerp3(mat, sand, rocks, sat, -700, 100)
    tint = vparam(mat, "CleanTint", unreal.LinearColor(1.05, 0.95, 0.8, 1), -700, -150)
    clean = expr(mat, unreal.MaterialExpressionMultiply, -500, -50)
    MEL.connect_material_expressions(base, "", clean, "A")
    MEL.connect_material_expressions(tint, "", clean, "B")
    dirty = dirty_of(mat, base, -500, 200, fraction=0.6, darken=0.75)
    out = lerp3(mat, dirty, clean, clean_alpha(mat, mpc, -500, 400), -200, 0)
    MEL.connect_material_property(out, "", MP.MP_BASE_COLOR)
    n1 = sample(mat, tex(SAND_N), uv, -1000, 800, "normal")
    n2 = sample(mat, tex(CSR_N), uv2, -1000, 1000, "normal")
    MEL.connect_material_property(lerp3(mat, n1, n2, sat, -600, 900), "", MP.MP_NORMAL)
    arm = sample(mat, tex(SAND_M), uv, -1000, 1200, "mask")
    rgh = expr(mat, unreal.MaterialExpressionComponentMask, -750, 1250, r=False, g=True, b=False, a=False)
    MEL.connect_material_expressions(arm, "", rgh, "")
    MEL.connect_material_property(rgh, "", MP.MP_ROUGHNESS)
    ao = expr(mat, unreal.MaterialExpressionComponentMask, -750, 1350, r=True, g=False, b=False, a=False)
    MEL.connect_material_expressions(arm, "", ao, "")
    MEL.connect_material_property(ao, "", MP.MP_AMBIENT_OCCLUSION)
    return finish(mat)


def bubble_material():
    mat = fresh("M_DSBubble", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fres = expr(mat, unreal.MaterialExpressionFresnel, -600, 100, exponent=2.0, base_reflect_fraction=0.15)
    col = vparam(mat, "Color", unreal.LinearColor(0.75, 0.95, 1.0, 1), -600, -100)
    emi = expr(mat, unreal.MaterialExpressionMultiply, -300, 0)
    MEL.connect_material_expressions(col, "", emi, "A")
    MEL.connect_material_expressions(const(mat, 8.0, -600, 0), "", emi, "B")
    MEL.connect_material_property(emi, "", MP.MP_EMISSIVE_COLOR)
    op = expr(mat, unreal.MaterialExpressionAdd, -300, 150)
    MEL.connect_material_expressions(fres, "", op, "A")
    MEL.connect_material_expressions(const(mat, 0.45, -600, 250), "", op, "B")
    MEL.connect_material_property(op, "", MP.MP_OPACITY)
    return finish(mat)


def surface_material():
    mat = fresh("M_DSSurface", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    noise = expr(mat, unreal.MaterialExpressionNoise, -700, 150, scale=0.002, output_min=0.6, output_max=1.3, levels=2)
    col = vparam(mat, "Color", unreal.LinearColor(0.55, 0.95, 1.0, 1), -700, -50)
    emi = expr(mat, unreal.MaterialExpressionMultiply, -350, 0)
    MEL.connect_material_expressions(col, "", emi, "A")
    MEL.connect_material_expressions(noise, "", emi, "B")
    MEL.connect_material_property(emi, "", MP.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(const(mat, 0.75, -350, 250), "", MP.MP_OPACITY)
    return finish(mat)


def light_ray_material():
    """Rayon de lumiere stylise : cone additif, plus intense en haut, s'efface en bordure."""
    mat = fresh("M_DSLightRay", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    fres = expr(mat, unreal.MaterialExpressionFresnel, -700, 350, exponent=2.0, base_reflect_fraction=0.0)
    m1 = expr(mat, unreal.MaterialExpressionOneMinus, -550, 350)
    MEL.connect_material_expressions(fres, "", m1, "")
    col = vparam(mat, "Color", unreal.LinearColor(0.5, 0.9, 1.0, 1), -700, 0)
    k = sparam(mat, "Intensity", 0.18, -700, 100)
    m2 = expr(mat, unreal.MaterialExpressionMultiply, -400, 50)
    MEL.connect_material_expressions(col, "", m2, "A")
    MEL.connect_material_expressions(k, "", m2, "B")
    m3 = expr(mat, unreal.MaterialExpressionMultiply, -250, 100)
    MEL.connect_material_expressions(m2, "", m3, "A")
    MEL.connect_material_expressions(m1, "", m3, "B")
    MEL.connect_material_property(m3, "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


def outline_material():
    """Surbrillance au survol (overlay material) : lueur additive pulsee + liserÃ© fresnel."""
    mat = fresh("M_DSOutline", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fres = expr(mat, unreal.MaterialExpressionFresnel, -900, 100, exponent=3.0, base_reflect_fraction=0.0)
    rim = expr(mat, unreal.MaterialExpressionMultiply, -700, 100)
    MEL.connect_material_expressions(fres, "", rim, "A")
    MEL.connect_material_expressions(const(mat, 2.5, -900, 200), "", rim, "B")
    base = expr(mat, unreal.MaterialExpressionAdd, -550, 100)
    MEL.connect_material_expressions(rim, "", base, "A")
    MEL.connect_material_expressions(const(mat, 0.3, -700, 250), "", base, "B")
    # pulsation 0.7..1.0
    t = expr(mat, unreal.MaterialExpressionTime, -1100, 400)
    tk = expr(mat, unreal.MaterialExpressionMultiply, -950, 400)
    MEL.connect_material_expressions(t, "", tk, "A")
    MEL.connect_material_expressions(const(mat, 6.0, -1100, 500), "", tk, "B")
    sn = expr(mat, unreal.MaterialExpressionSine, -800, 400, period=6.2831853)
    MEL.connect_material_expressions(tk, "", sn, "")
    sk = expr(mat, unreal.MaterialExpressionMultiply, -650, 400)
    MEL.connect_material_expressions(sn, "", sk, "A")
    MEL.connect_material_expressions(const(mat, 0.15, -800, 500), "", sk, "B")
    pulse = expr(mat, unreal.MaterialExpressionAdd, -500, 400)
    MEL.connect_material_expressions(sk, "", pulse, "A")
    MEL.connect_material_expressions(const(mat, 0.85, -650, 500), "", pulse, "B")
    col = vparam(mat, "Color", unreal.LinearColor(1.0, 0.95, 0.55, 1), -550, -100)
    m1 = expr(mat, unreal.MaterialExpressionMultiply, -350, 0)
    MEL.connect_material_expressions(col, "", m1, "A")
    MEL.connect_material_expressions(base, "", m1, "B")
    m2 = expr(mat, unreal.MaterialExpressionMultiply, -200, 150)
    MEL.connect_material_expressions(m1, "", m2, "A")
    MEL.connect_material_expressions(pulse, "", m2, "B")
    MEL.connect_material_property(m2, "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


mpc = make_mpc()
outline_material()
light_ray_material()
color_material()
world = world_material(mpc)
sand_material(mpc)
bubble_material()
surface_material()

LC = unreal.LinearColor
world_instance("MI_Rock", world, LC(0.42, 0.45, 0.42, 1), rough=0.85)
world_instance("MI_RockDark", world, LC(0.25, 0.28, 0.3, 1), rough=0.85)
world_instance("MI_Kelp", world, LC(0.15, 0.7, 0.25, 1), glow=0.05)
for i, c in enumerate([LC(1, 0.3, 0.45, 1), LC(1, 0.55, 0.1, 1), LC(0.65, 0.3, 1, 1),
                       LC(0.2, 0.9, 0.85, 1), LC(1, 0.85, 0.2, 1), LC(1, 0.45, 0.75, 1)]):
    world_instance(f"MI_Coral{i}", world, c, glow=0.25, rough=0.5)

unreal.log("DS_MATERIALS_OK")
