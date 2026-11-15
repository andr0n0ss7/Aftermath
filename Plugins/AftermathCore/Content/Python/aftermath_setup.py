"""
Aftermath setup — builds the Kitchen 02:14 greybox inside the Unreal Editor (UE 5.6).

Run it from the editor: Tools > Execute Python Script... and pick this file
(or type in the Output Log, Python mode:  import aftermath_setup; aftermath_setup.run()).

Safe to run again: assets are updated in place and only actors in the "Aftermath/" outliner folders are rebuilt.
Reads <Project>/AftermathData/ (case data, surface library, textures), which is part of the Git repository.
"""
import json
import os
import unreal

ROOT = "/Game/Aftermath"
MAP = ROOT + "/Maps/L_Kitchen0214"
DATA_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AftermathData")

tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(msg):
    unreal.log("[Aftermath] " + msg)


def vec(d):
    return unreal.Vector(d["X"], d["Y"], d["Z"])


def get_or_create(name, path, cls, factory):
    full = path + "/" + name
    if eal.does_asset_exist(full):
        return eal.load_asset(full)
    return tools.create_asset(name, path, cls, factory)


# ---------------------------------------------------------------- textures
def import_textures():
    src = os.path.join(DATA_DIR, "Textures")
    tasks = []
    for f in sorted(os.listdir(src)):
        if not f.lower().endswith(".png"):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(src, f))
        t.set_editor_property("destination_path", ROOT + "/Textures")
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    log("Imported %d textures" % len(tasks))


def tex(name):
    return eal.load_asset(ROOT + "/Textures/" + name)


# ---------------------------------------------------------------- materials
def scalar_param(mat, name, value, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def build_surface_material():
    """M_AM_Surface: flat colour + roughness + metallic. Placeholder until Fab materials arrive in Phase 3."""
    m = get_or_create("M_AM_Surface", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    c = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -400, -100)
    c.set_editor_property("parameter_name", "BaseColor")
    c.set_editor_property("default_value", unreal.LinearColor(0.7, 0.7, 0.7, 1))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar_param(m, "Roughness", 0.8, -400, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar_param(m, "Metallic", 0.0, -400, 200), "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(m)
    return m


def build_decal_material():
    """M_AM_Decal: deferred decal; colour and opacity from a defect texture."""
    m = get_or_create("M_AM_Decal", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mel.delete_all_material_expressions(m)
    t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
    t.set_editor_property("parameter_name", "Defect")
    t.set_editor_property("texture", tex("T_Defect_DrywallEntry"))
    mel.connect_material_property(t, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(t, "A", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(scalar_param(m, "Roughness", 0.85, -500, 250), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    return m


def build_glass_material():
    """M_AM_Glass: translucent pane with the crack pattern. UV flip/swap switches line the texture up with the holes."""
    m = get_or_create("M_AM_Glass", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    mel.delete_all_material_expressions(m)

    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    flip_u = scalar_param(m, "FlipU", 0.0, -900, 120)
    flip_v = scalar_param(m, "FlipV", 0.0, -900, 220)
    swap = scalar_param(m, "SwapUV", 0.0, -900, 320)
    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -650, 100)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    custom.set_editor_property("code",
        "float2 t = (Swap > 0.5) ? UV.yx : UV;\n"
        "if (FlipU > 0.5) t.x = 1 - t.x;\n"
        "if (FlipV > 0.5) t.y = 1 - t.y;\n"
        "return t;")
    inputs = []
    for n in ["UV", "FlipU", "FlipV", "Swap"]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(uv, "", custom, "UV")
    mel.connect_material_expressions(flip_u, "", custom, "FlipU")
    mel.connect_material_expressions(flip_v, "", custom, "FlipV")
    mel.connect_material_expressions(swap, "", custom, "Swap")

    t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 100)
    t.set_editor_property("parameter_name", "Cracks")
    t.set_editor_property("texture", tex("T_Glass_KitchenWindow_Cracks"))
    mel.connect_material_expressions(custom, "", t, "UVs")

    floor_ = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 350)
    floor_.set_editor_property("r", 0.06)
    mx = mel.create_material_expression(m, unreal.MaterialExpressionMax, -150, 250)
    mel.connect_material_expressions(t, "A", mx, "A")
    mel.connect_material_expressions(floor_, "", mx, "B")

    mel.connect_material_property(t, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(mx, "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(scalar_param(m, "Roughness", 0.05, -150, 450), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    return m


def instance(name, parent, vectors=None, scalars=None, textures=None):
    mi = get_or_create(name, ROOT + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1))
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    mel.update_material_instance(mi)
    return mi


def build_materials():
    surface = build_surface_material()
    decal = build_decal_material()
    glass = build_glass_material()
    mats = {
        "Wall_Kitchen": instance("MI_Wall_Kitchen", surface, {"BaseColor": (0.72, 0.70, 0.64)}, {"Roughness": 0.9}),
        "Wall_Hall": instance("MI_Wall_Hall", surface, {"BaseColor": (0.38, 0.43, 0.37)}, {"Roughness": 0.9}),
        "Floor_Tile": instance("MI_Floor_Tile", surface, {"BaseColor": (0.17, 0.16, 0.15)}, {"Roughness": 0.5}),
        "Floor_Oak": instance("MI_Floor_Oak", surface, {"BaseColor": (0.30, 0.17, 0.08)}, {"Roughness": 0.55}),
        "Ceiling": instance("MI_Ceiling", surface, {"BaseColor": (0.80, 0.80, 0.78)}, {"Roughness": 0.95}),
        "Furniture": instance("MI_Furniture", surface, {"BaseColor": (0.09, 0.12, 0.14)}, {"Roughness": 0.55}),
    }
    instance("MI_Brass", surface, {"BaseColor": (0.85, 0.62, 0.25)}, {"Roughness": 0.28, "Metallic": 1.0})
    instance("MI_Brass_Tarnished", surface, {"BaseColor": (0.33, 0.27, 0.15)}, {"Roughness": 0.7, "Metallic": 1.0})
    instance("MI_String_Red", surface, {"BaseColor": (0.75, 0.07, 0.04)}, {"Roughness": 0.6})
    instance("MI_Gunmetal", surface, {"BaseColor": (0.04, 0.04, 0.045)}, {"Roughness": 0.35, "Metallic": 0.9})
    for mi, t in [("MI_Defect_DrywallEntry", "T_Defect_DrywallEntry"), ("MI_Defect_DrywallExit", "T_Defect_DrywallExit"),
                  ("MI_Defect_PlasterCrater", "T_Defect_PlasterCrater"), ("MI_Defect_SteelHole", "T_Defect_SteelHole"),
                  ("MI_Decoy_PaintedHole", "T_Decoy_PaintedHole"), ("MI_Decoy_WallPlugs", "T_Decoy_WallPlugs"),
                  ("MI_Defect_Ricochet", "T_Defect_Ricochet")]:
        instance(mi, decal, textures={"Defect": tex(t)})
    mats["Glass"] = instance("MI_Glass_KitchenWindow", glass, textures={"Cracks": tex("T_Glass_KitchenWindow_Cracks")})
    log("Materials built")
    return mats


# ---------------------------------------------------------------- level
def open_or_create_map():
    if eal.does_asset_exist(MAP):
        levels.load_level(MAP)
        for a in actors.get_all_level_actors():
            if str(a.get_folder_path()).startswith("Aftermath"):
                actors.destroy_actor(a)
        log("Reusing " + MAP)
    else:
        levels.new_level(MAP)
        log("Created " + MAP)


def spawn(cls, loc, rot=None, label=None, folder="Aftermath/Lighting"):
    a = actors.spawn_actor_from_class(cls, loc, rot or unreal.Rotator(0, 0, 0))
    if label:
        a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def build_blockout(level, mats):
    cube = eal.load_asset("/Engine/BasicShapes/Cube")
    plane = eal.load_asset("/Engine/BasicShapes/Plane")
    for b in level["Boxes"]:
        a = spawn(unreal.StaticMeshActor, vec(b["Center"]), label=b["Name"], folder="Aftermath/Blockout")
        smc = a.static_mesh_component
        smc.set_static_mesh(cube)
        smc.set_material(0, mats[b["Material"]])
        s = b["Size"]
        a.set_actor_scale3d(unreal.Vector(s["X"] / 100.0, s["Y"] / 100.0, s["Z"] / 100.0))

    w = level["WindowPane"]
    pane = spawn(unreal.StaticMeshActor, vec(w["Center"]), unreal.Rotator(0, 90, 0), "WindowPane", "Aftermath/Blockout")
    pane.static_mesh_component.set_static_mesh(plane)
    pane.static_mesh_component.set_material(0, mats["Glass"])
    pane.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    pane.set_actor_scale3d(unreal.Vector(w["HeightCm"] / 100.0, w["WidthCm"] / 100.0, 1.0))
    log("Blockout: %d boxes" % len(level["Boxes"]))


def light_component(actor, cls):
    return actor.get_component_by_class(cls)


def build_lighting(level):
    for L in level["Lights"]:
        loc = vec(L["Location"])
        rot = unreal.MathLibrary.find_look_at_rotation(loc, vec(L["Target"])) if "Target" in L else unreal.Rotator(0, 0, 0)
        cls, ccls = {"Rect": (unreal.RectLight, unreal.RectLightComponent),
                     "Point": (unreal.PointLight, unreal.PointLightComponent),
                     "Spot": (unreal.SpotLight, unreal.SpotLightComponent)}[L["Type"]]
        a = spawn(cls, loc, rot, L["Name"])
        c = light_component(a, ccls)
        # Stationary: baked bounce light on DirectX 11, live under Lumen on DirectX 12.
        c.set_mobility(unreal.ComponentMobility.STATIONARY)
        c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        c.set_editor_property("intensity", float(L["Intensity"]))
        col = L["Color"]
        c.set_light_color(unreal.LinearColor(col[0] / 255.0, col[1] / 255.0, col[2] / 255.0, 1))
        c.set_editor_property("attenuation_radius", 900.0)
        if L["Type"] == "Rect":
            c.set_editor_property("source_width", float(L["Width"]))
            c.set_editor_property("source_height", float(L["Height"]))
        if L["Type"] == "Spot":
            c.set_editor_property("inner_cone_angle", 10.0)
            c.set_editor_property("outer_cone_angle", 24.0)

    # Reflection captures: DirectX 11 has no Lumen reflections, so each room needs one.
    for name, loc, radius in [("Reflections_Kitchen", unreal.Vector(250, 200, 140), 420.0),
                              ("Reflections_Hall", unreal.Vector(-100, 200, 140), 300.0)]:
        rc = spawn(unreal.SphereReflectionCapture, loc, label=name)
        light_component(rc, unreal.SphereReflectionCaptureComponent).set_editor_property("influence_radius", radius)

    ppv = spawn(unreal.PostProcessVolume, unreal.Vector(150, 200, 130), label="PPV_Kitchen")
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", 1.0)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", 5.0)
    s.set_editor_property("override_vignette_intensity", True)
    s.set_editor_property("vignette_intensity", 0.35)
    ppv.set_editor_property("settings", s)

    ps = level["PlayerStart"]
    spawn(unreal.PlayerStart, vec(ps["Location"]), unreal.Rotator(0, 0, ps["YawDeg"]), "PlayerStart", "Aftermath/Gameplay")
    log("Lighting and player start placed")


# ---------------------------------------------------------------- case
def build_libraries():
    """Surface library (how each material takes a bullet) and one firearm asset per gun in the case."""
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", unreal.AMSurfaceLibrary)
    lib = get_or_create("DA_SurfaceLibrary", ROOT + "/Data", unreal.AMSurfaceLibrary, f)
    if not lib.import_from_json_file(os.path.join(DATA_DIR, "Data", "SurfaceProfiles.json")):
        raise RuntimeError("Surface library import failed - see the Output Log (LogAMSurfaces).")
    eal.save_loaded_asset(lib)

    guns = [
        ("DA_Firearm_Pistol9mm", "9 mm striker-fired pistol", "9mm", "9mm Luger", unreal.AMFirearmAction.SEMI_AUTO_PISTOL, 15, (19.0, 13.0, 3.0)),
        ("DA_Firearm_Revolver38", ".38 Special six-shot revolver", "38", ".38 Special", unreal.AMFirearmAction.REVOLVER, 6, (24.0, 14.0, 4.0)),
    ]
    for name, display, key, cal, action, cap, size in guns:
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", unreal.AMFirearmData)
        a = get_or_create(name, ROOT + "/Firearms", unreal.AMFirearmData, f)
        a.set_editor_property("display_name", unreal.Text(display))
        a.set_editor_property("weapon_key", key)
        a.set_editor_property("caliber", cal)
        a.set_editor_property("action", action)
        a.set_editor_property("capacity", cap)
        a.set_editor_property("placeholder_size_cm", unreal.Vector(*size))
        eal.save_loaded_asset(a)
    log("Surface library and firearm assets ready")


def build_case():
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.AMCaseData)
    case = get_or_create("DA_Case_Kitchen0214", ROOT + "/Cases", unreal.AMCaseData, factory)
    ok = case.import_from_json_file(os.path.join(DATA_DIR, "Data", "Case_Kitchen0214.json"))
    if not ok:
        raise RuntimeError("Case JSON import failed - see the Output Log (LogAMCase).")
    eal.save_loaded_asset(case)
    director = spawn(unreal.AMCaseDirector, unreal.Vector(150, 200, 240), label="CaseDirector", folder="Aftermath/Gameplay")
    director.set_editor_property("case", case)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.AMGameMode.static_class())
    log("Case asset imported and director placed")


def run():
    if not os.path.isdir(DATA_DIR):
        raise RuntimeError("Copy the kit's Data and Textures folders to " + DATA_DIR)
    with open(os.path.join(DATA_DIR, "Data", "Level_Kitchen0214_Blockout.json")) as f:
        level = json.load(f)
    import_textures()
    mats = build_materials()
    open_or_create_map()
    build_blockout(level, mats)
    build_lighting(level)
    build_libraries()
    build_case()
    levels.save_current_level()
    eal.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
    log("Done. Press Play.")


if __name__ == "__main__":
    run()
