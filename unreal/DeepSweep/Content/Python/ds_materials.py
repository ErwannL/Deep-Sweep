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


def color_material():
    mat = fresh("M_DSColor", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    col = vparam(mat, "Color", unreal.LinearColor(0.8, 0.8, 0.8, 1), -500, 0)
    MEL.connect_material_property(col, "", MP.MP_BASE_COLOR)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.55, -500, 200), "", MP.MP_ROUGHNESS)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -250, 350)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(sparam(mat, "Emissive", 0.0, -500, 400), "", mul, "B")
    MEL.connect_material_property(mul, "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


def world_material(mpc):
    mat = fresh("M_DSWorld", unreal.Material, unreal.MaterialFactoryNew())
    col = vparam(mat, "Color", unreal.LinearColor(1, 0.4, 0.5, 1), -900, 0)
    dirty = dirty_of(mat, col, -650, 150)
    lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -250, 0)
    MEL.connect_material_expressions(dirty, "", lerp, "A")
    MEL.connect_material_expressions(col, "", lerp, "B")
    MEL.connect_material_expressions(clean_alpha(mat, mpc, -500, 350), "", lerp, "Alpha")
    MEL.connect_material_property(lerp, "", MP.MP_BASE_COLOR)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.7, -500, 500), "", MP.MP_ROUGHNESS)
    # Leger eclat quand c'est propre (coraux qui "revivent")
    emi = expr(mat, unreal.MaterialExpressionMultiply, -250, 600)
    k = expr(mat, unreal.MaterialExpressionMultiply, -450, 650)
    MEL.connect_material_expressions(clean_alpha(mat, mpc, -700, 650), "", k, "A")
    MEL.connect_material_expressions(sparam(mat, "CleanGlow", 0.0, -700, 750), "", k, "B")
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
    mat = fresh("M_DSSand", unreal.Material, unreal.MaterialFactoryNew())
    clean = vparam(mat, "CleanColor", unreal.LinearColor(0.78, 0.6, 0.3, 1), -900, -100)
    dirty = vparam(mat, "DirtyColor", unreal.LinearColor(0.28, 0.27, 0.2, 1), -900, 50)
    lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -600, 0)
    MEL.connect_material_expressions(dirty, "", lerp, "A")
    MEL.connect_material_expressions(clean, "", lerp, "B")
    MEL.connect_material_expressions(clean_alpha(mat, mpc, -900, 200), "", lerp, "Alpha")
    noise = expr(mat, unreal.MaterialExpressionNoise, -600, 250, scale=0.004, output_min=0.8, output_max=1.08, levels=3)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -300, 0)
    MEL.connect_material_expressions(lerp, "", mul, "A")
    MEL.connect_material_expressions(noise, "", mul, "B")
    MEL.connect_material_property(mul, "", MP.MP_BASE_COLOR)
    MEL.connect_material_property(const(mat, 0.9, -300, 300), "", MP.MP_ROUGHNESS)
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
    """Surbrillance au survol (overlay material) : lueur additive pulsee + liseré fresnel."""
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
