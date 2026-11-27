"""Materiaux de base du projet (couleurs unies parametrables, style cartoon)."""
import unreal

MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/Materials"


def color_material(name, roughness=0.55, emissive=0.0):
    path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    col = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(0.8, 0.8, 0.8, 1))
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", roughness)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    emi = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 350)
    emi.set_editor_property("parameter_name", "Emissive")
    emi.set_editor_property("default_value", emissive)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 350)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(emi, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


color_material("M_DSColor")
unreal.log("DS_MATERIALS_OK")
