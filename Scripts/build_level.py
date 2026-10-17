"""
Construye el Nivel 1 de Estacion Nueve dentro del editor de Unreal.

- Reutiliza el blockout T3D del diseno (16 zonas) escalado a 1:1.
- Crea los materiales del blockout y los materiales propios del prototipo.
- Coloca los actores de juego: parcelas, valvulas, porton, mercader, refineria,
  generador, cama, checkpoints, recogibles, Brisa y los dinosaurios.
- Excava la caverna del tutorial como sala aparte y conecta el pozo.

Ejecutar:
  UnrealEditor-Cmd.exe EstacionNueve.uproject -run=pythonscript
      -script="<ruta>/build_level.py" -unattended -nosplash -nop4 -nullrhi
"""

import os
import re
import time
import traceback

import unreal

PROJECT_DIR = unreal.Paths.project_dir()
T3D_PATH = os.path.join(PROJECT_DIR, "Scripts", "Blockout_Nivel1_Unreal.t3d")
LEVEL_PATH = "/Game/Maps/L_Nivel1"
BLOCKOUT_SCALE = 2.0 / 3.0   # el T3D se genero a 1.5x; 1:1 respeta las metricas del GDD

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(message):
    unreal.log("[EN-BUILD] " + str(message))


# --------------------------------------------------------------------------- anclas (espejo de ENAnchors.h)
OX, OY = 4.0, -80.0


def W(east, north, z=0.0):
    return unreal.Vector((north - OY) * 100.0, (east - OX) * 100.0, z)


PLOTS = [(-58, 14), (-38, 14), (-58, -12), (-38, -12)]
CAVE_X, CAVE_Y, CAVE_Z = 41200.0, 0.0, -2200.0


# --------------------------------------------------------------------------- materiales
def make_material(name, folder, color, emissive=0.0, roughness=0.8):
    path = "{}/{}".format(folder, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)

    unreal.EditorAssetLibrary.make_directory(folder)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        log("No se pudo crear el material " + path)
        return None

    base = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -420, 0)
    base.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -420, 220)
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if emissive > 0.0:
        glow = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant3Vector, -420, 420)
        glow.set_editor_property("constant", unreal.LinearColor(
            color[0] * emissive, color[1] * emissive, color[2] * emissive, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat


def build_materials():
    mats = {}
    proto = "/Game/LevelPrototyping/Materials"
    mats["MI_PrototypeGrid_Gray"] = make_material("MI_PrototypeGrid_Gray", proto, (0.42, 0.43, 0.45))
    mats["MI_PrototypeGrid_Gray_02"] = make_material("MI_PrototypeGrid_Gray_02", proto, (0.30, 0.30, 0.32))
    mats["MI_PrototypeGrid_TopDark"] = make_material("MI_PrototypeGrid_TopDark", proto, (0.07, 0.07, 0.08))
    mats["MI_DefaultColorway"] = make_material("MI_DefaultColorway", proto, (0.85, 0.45, 0.12))
    mats["MI_PrototypeGrid_Gray_Round"] = make_material("MI_PrototypeGrid_Gray_Round", proto, (0.36, 0.36, 0.38))
    mats["MI_GlowNT"] = make_material("MI_GlowNT", proto, (1.0, 0.55, 0.12), emissive=6.0)

    own = "/Game/Materials"
    mats["M_Floor"] = make_material("M_Floor", own, (0.24, 0.22, 0.19))
    mats["M_Crop"] = make_material("M_Crop", own, (0.22, 0.58, 0.16))
    mats["M_CropRipe"] = make_material("M_CropRipe", own, (0.80, 0.72, 0.22))
    mats["M_Water"] = make_material("M_Water", own, (0.15, 0.42, 0.65))
    mats["M_Valve"] = make_material("M_Valve", own, (0.65, 0.18, 0.12))
    mats["M_Gate"] = make_material("M_Gate", own, (0.30, 0.18, 0.09))
    mats["M_Merchant"] = make_material("M_Merchant", own, (0.58, 0.28, 0.48))
    mats["M_Vein"] = make_material("M_Vein", own, (1.0, 0.62, 0.10), emissive=1.4)
    mats["M_VeinIron"] = make_material("M_VeinIron", own, (0.36, 0.36, 0.40), roughness=0.4)
    mats["M_VeinCoal"] = make_material("M_VeinCoal", own, (0.07, 0.07, 0.07))
    mats["M_Dino"] = make_material("M_Dino", own, (0.36, 0.42, 0.24))
    mats["M_Raptor"] = make_material("M_Raptor", own, (0.28, 0.22, 0.15))
    mats["M_Carnotaurus"] = make_material("M_Carnotaurus", own, (0.36, 0.10, 0.08))
    mats["M_EyeGlow"] = make_material("M_EyeGlow", own, (1.0, 0.10, 0.04), emissive=12.0)
    mats["M_Mount"] = make_material("M_Mount", own, (0.45, 0.55, 0.28))
    mats["M_FlareGlow"] = make_material("M_FlareGlow", own, (1.0, 0.35, 0.05), emissive=15.0)
    mats["M_CaveRock"] = make_material("M_CaveRock", own, (0.17, 0.155, 0.14))
    mats["M_Pickup"] = make_material("M_Pickup", own, (0.90, 0.75, 0.18), emissive=0.7)
    mats["M_Generator"] = make_material("M_Generator", own, (0.30, 0.30, 0.32), roughness=0.45)
    mats["M_Bed"] = make_material("M_Bed", own, (0.42, 0.28, 0.18))
    return mats


# --------------------------------------------------------------------------- utilidades de actores
def spawn(actor_class, location, yaw=0.0, label=None, folder=None):
    rotation = unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0)
    actor = EAS.spawn_actor_from_class(actor_class, location, rotation)
    if actor is None:
        log("No se pudo crear " + str(actor_class) + " en " + str(location))
        return None
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def box(location, scale, material, label, folder):
    actor = spawn(unreal.StaticMeshActor, location, 0.0, label, folder)
    if actor is None:
        return None
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
    if material:
        comp.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
    return actor


SKIP_EXACT = {"Boca_Mina", "Valvula_P1", "Generador", "Pozo", "Tienda_Mercader", "Mostrador"}


def spawn_blockout(materials):
    if not os.path.exists(T3D_PATH):
        log("Falta el T3D: " + T3D_PATH)
        return 0

    text = open(T3D_PATH, "r", encoding="utf-8").read()
    blocks = re.findall(r"Begin Actor Class=/Script/Engine\.StaticMeshActor.*?End Actor", text, re.S)
    log("Actores en el T3D: {}".format(len(blocks)))

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    cone = unreal.load_asset("/Engine/BasicShapes/Cone.Cone")

    created = 0
    for block in blocks:
        label_match = re.search(r'ActorLabel="([^"]*)"', block)
        label = label_match.group(1) if label_match else "Actor"
        if label in SKIP_EXACT or label.startswith("BLOQUEO_"):
            continue

        loc = re.search(r"RelativeLocation=\(X=([-\d.]+),Y=([-\d.]+),Z=([-\d.]+)\)", block)
        rot = re.search(r"RelativeRotation=\(Pitch=([-\d.]+),Yaw=([-\d.]+),Roll=([-\d.]+)\)", block)
        scl = re.search(r"RelativeScale3D=\(X=([-\d.]+),Y=([-\d.]+),Z=([-\d.]+)\)", block)
        mesh = re.search(r'StaticMesh="/Script/Engine\.StaticMesh\'([^\']+)\'', block)
        mat = re.search(r'OverrideMaterials\(0\)="/Script/Engine\.MaterialInstanceConstant\'([^\']+)\'', block)
        folder_match = re.search(r'FolderPath="([^"]*)"', block)
        if not (loc and rot and scl and mesh):
            continue

        location = unreal.Vector(float(loc.group(1)) * BLOCKOUT_SCALE,
                                 float(loc.group(2)) * BLOCKOUT_SCALE,
                                 float(loc.group(3)) * BLOCKOUT_SCALE)
        rotation = unreal.Rotator(pitch=float(rot.group(1)), yaw=float(rot.group(2)), roll=float(rot.group(3)))
        scale = unreal.Vector(float(scl.group(1)) * BLOCKOUT_SCALE,
                              float(scl.group(2)) * BLOCKOUT_SCALE,
                              float(scl.group(3)) * BLOCKOUT_SCALE)

        mesh_path = mesh.group(1)
        loaded = None
        if mesh_path.endswith("Cube.Cube"):
            loaded = cube
        elif mesh_path.endswith("Cylinder.Cylinder"):
            loaded = cyl
        elif mesh_path.endswith("Cone.Cone"):
            loaded = cone
        if loaded is None:
            loaded = unreal.load_asset(mesh_path)
        if loaded is None:
            continue

        actor = EAS.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
        if actor is None:
            continue
        comp = actor.static_mesh_component
        comp.set_static_mesh(loaded)

        if mat:
            package = mat.group(1).split(".")[0]
            material = unreal.EditorAssetLibrary.load_asset(package)
            if material:
                comp.set_material(0, material)
        if label == "Suelo":
            floor_material = unreal.EditorAssetLibrary.load_asset("/Game/Materials/M_Floor")
            if floor_material:
                comp.set_material(0, floor_material)

        actor.set_actor_scale3d(scale)
        actor.set_actor_label(label)
        if folder_match:
            actor.set_folder_path("Blockout_Nivel1/" + folder_match.group(1))
        created += 1

    log("Actores del blockout creados: {}".format(created))
    return created


# --------------------------------------------------------------------------- luces y ambiente
def build_lighting():
    sun = spawn(unreal.DirectionalLight, unreal.Vector(7500.0, -400.0, 2500.0), 35.0, "Sol", "Ambiente")
    if sun:
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_intensity(7.0)
        sun.set_actor_rotation(unreal.Rotator(-42.0, 35.0, 0.0), False)

    sky = spawn(unreal.SkyLight, unreal.Vector(7500.0, -400.0, 1400.0), 0.0, "LuzCielo", "Ambiente")
    if sky:
        comp = sky.get_component_by_class(unreal.SkyLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        try:
            comp.set_editor_property("bRealTimeCapture", True)
        except Exception:
            pass
        try:
            comp.set_editor_property("real_time_capture", True)
        except Exception:
            pass
        comp.set_intensity(1.0)

    atmosphere = spawn(unreal.SkyAtmosphere, unreal.Vector(7500.0, -400.0, 0.0), 0.0, "Atmosfera", "Ambiente")
    if atmosphere:
        comp = atmosphere.get_component_by_class(unreal.SkyAtmosphereComponent)
        if comp:
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)

    fog = spawn(unreal.ExponentialHeightFog, unreal.Vector(7500.0, -400.0, 200.0), 0.0, "Niebla", "Ambiente")
    if fog:
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        comp.set_editor_property("FogDensity", 0.008)
        comp.set_editor_property("FogHeightFalloff", 0.18)
        try:
            comp.set_editor_property("bEnableVolumetricFog", True)
        except Exception:
            pass

    ppv = spawn(unreal.PostProcessVolume, unreal.Vector(7500.0, -400.0, 800.0), 0.0, "PostProceso", "Ambiente")
    if ppv:
        try:
            ppv.set_editor_property("bUnbound", True)
        except Exception:
            try:
                ppv.set_editor_property("unbound", True)
            except Exception:
                pass

    # Luces de trabajo dentro de la estacion.
    for name, location, intensity in [
        ("LuzEstablo", W(26, -32, 500), 700.0),
        ("LuzTaller", W(50, 2, 500), 700.0),
        ("LuzMaque", W(0, 42, 400), 600.0),
        ("LuzMercader", W(20, -64, 400), 500.0),
        ("LuzCasa", W(-50, 42, 450), 600.0),
    ]:
        light = spawn(unreal.PointLight, location, 0.0, name, "Ambiente/Luces")
        if light:
            comp = light.get_component_by_class(unreal.PointLightComponent)
            comp.set_intensity(intensity)
            comp.set_editor_property("AttenuationRadius", 1200.0)
            comp.set_light_color(unreal.LinearColor(1.0, 0.72, 0.42, 1.0))
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)


# --------------------------------------------------------------------------- actores de juego
def en_class(name):
    return unreal.load_class(None, "/Script/EstacionNueve." + name)


def spawn_en(name, location, yaw=0.0, label=None, folder="Gameplay", props=None):
    actor = spawn(en_class(name), location, yaw, label, folder)
    if actor is None:
        return None
    for key, value in (props or {}).items():
        try:
            actor.set_editor_property(key, value)
        except Exception as exc:
            log("Propiedad {} fallo en {}: {}".format(key, name, exc))
    return actor


def build_gameplay(materials):
    # Parcelas del huerto.
    for i, (east, north) in enumerate(PLOTS):
        plot = spawn_en("ENFarmPlot", W(east, north, 0.0), 0.0, "Parcela_{}".format(i + 1), "Gameplay/Huerto")
        if plot:
            plot.set_editor_property("PlotIndex", i)

    # Agua, valvula y porton.
    spawn_en("ENWaterTower", W(-48, 1, 80), 0.0, "Torre_Agua", "Gameplay/Huerto")
    spawn_en("ENValve", W(12.5, -51.5, 60), 0.0, "Valvula_P1", "Gameplay/Porton")
    gate = spawn_en("ENGate", W(4, -50, 250), 0.0, "Porton_P1", "Gameplay/Porton")

    # Mercader, refineria, generador y cama.
    spawn_en("ENMerchant", W(20, -70, 110), 0.0, "Barnaby_Quill", "Gameplay/Mercader")
    spawn_en("ENRefinery", W(50, -8, 120), 0.0, "Refineria", "Gameplay/Taller")
    spawn_en("ENGenerator", W(20, 52, 175), 0.0, "Generador", "Gameplay/Noche")
    spawn_en("ENSleepSpot", W(-50, 34, 30), 0.0, "Cama_Casa_Mando", "Gameplay/CasaMando")

    # Viaje superficie <-> caverna.
    cave_landing = unreal.Vector(40200.0, 0.0, -2100.0)
    spawn_en("ENTravelPoint", W(62, 42, 60), 0.0, "Boca_Mina", "Gameplay/Mina",
             {"TargetLocation": cave_landing, "TargetYaw": 0.0,
              "Prompt": "Bajar a la caverna por el pozo"})
    spawn_en("ENTravelPoint", unreal.Vector(42400.0, 500.0, -2140.0), 0.0, "Cuerda_Salida", "Gameplay/Caverna",
             {"TargetLocation": W(62, 46, 120), "TargetYaw": 180.0,
              "Prompt": "Subir por la cuerda a la superficie"})

    # Checkpoints de la ruta critica.
    for i, (east, north) in enumerate([(14, -32), (58, 50), (64, 14), (12, 6), (8, 44)]):
        spawn_en("ENCheckpoint", W(east, north, 170), 0.0, "Checkpoint_{}".format(i + 1),
                 "Gameplay/Checkpoints")

    # Recogibles.
    pickups = [
        ("Ammo", 6, (18, -26, 20)),
        ("Bandage", 1, (-48, -40, 20)),
        ("Oil", 50, (0, 36, 20)),
        ("Flare", 1, (64, 14, 20)),
        ("Ammo", 9, (40, 0, 20)),
        ("Forage", 2, (-30, 16, 20)),
        ("Ammo", 6, (70, 45, 20)),
    ]
    for i, (loot, amount, (east, north, z)) in enumerate(pickups):
        value = getattr(unreal.ENLootType, loot.upper())
        actor = spawn_en("ENPickup", W(east, north, z), 0.0, "Pickup_{}_{}".format(loot, i + 1),
                         "Gameplay/Recogibles", {"Loot": value, "Amount": amount})
        if actor:
            actor.set_material_path("/Game/Materials/M_Pickup")

    # Brisa.
    brisa = spawn_en("ENMount", W(25, -27, 0), 90.0, "Brisa", "Gameplay/Establo")

    # Dinosaurios: establo (3 Chillones) y refineria (2 Rastreadores + 1 Chillon).
    stable_group = [(32, -30), (30, -24), (36, -26)]
    refinery_group = [(48, -4), (46, -12), (52, -8)]
    for i, (east, north) in enumerate(stable_group):
        spawn_en("ENDinosaur", W(east, north, 90), 0.0, "Chillon_Establo_{}".format(i + 1),
                 "Gameplay/Enemigos")
    for i, (east, north) in enumerate(refinery_group):
        dino = spawn_en("ENDinosaur", W(east, north, 100), 0.0, "Raptor_Refineria_{}".format(i + 1),
                        "Gameplay/Enemigos", {"bIsRaptor": True})
    return gate


def build_cave(materials):
    rock = materials.get("M_CaveRock")
    floor_z = CAVE_Z

    # Suelo, techo y muros de la caverna.
    box(unreal.Vector(CAVE_X, CAVE_Y, floor_z - 20.0), (24.0, 16.0, 0.4), rock, "Cueva_Suelo", "Caverna")
    box(unreal.Vector(CAVE_X, CAVE_Y, floor_z + 520.0), (24.0, 16.0, 0.4), rock, "Cueva_Techo", "Caverna")
    box(unreal.Vector(CAVE_X + 1200.0, CAVE_Y, floor_z + 250.0), (0.4, 16.0, 5.0), rock, "Cueva_Muro_N", "Caverna")
    box(unreal.Vector(CAVE_X - 1200.0, CAVE_Y, floor_z + 250.0), (0.4, 16.0, 5.0), rock, "Cueva_Muro_S", "Caverna")
    box(unreal.Vector(CAVE_X, CAVE_Y + 800.0, floor_z + 250.0), (24.0, 0.4, 5.0), rock, "Cueva_Muro_E", "Caverna")
    box(unreal.Vector(CAVE_X, CAVE_Y - 800.0, floor_z + 250.0), (24.0, 0.4, 5.0), rock, "Cueva_Muro_O", "Caverna")

    # Estalagmitas como cobertura.
    for i, (dx, dy, size) in enumerate([(-700, -450, 1.6), (-300, 520, 2.2), (400, -520, 1.9), (800, 430, 1.5)]):
        box(unreal.Vector(CAVE_X + dx, CAVE_Y + dy, floor_z + size * 50.0), (size, size, size * 2.0), rock,
            "Estalagmita_{}".format(i + 1), "Caverna")

    # Luces de la caverna.
    for i, (dx, dy) in enumerate([(-900, 0), (0, 0), (900, 0)]):
        light = spawn(unreal.PointLight, unreal.Vector(CAVE_X + dx, CAVE_Y + dy, floor_z + 420.0),
                      0.0, "Luz_Cueva_{}".format(i + 1), "Caverna/Luces")
        if light:
            comp = light.get_component_by_class(unreal.PointLightComponent)
            comp.set_intensity(1600.0)
            comp.set_editor_property("AttenuationRadius", 2200.0)
            comp.set_light_color(unreal.LinearColor(1.0, 0.70, 0.38, 1.0))
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)

    # Vetas: 6 de resina, 2 de hierro y 2 de carbon.
    veins = ([(unreal.ENLootType.RESIN, "/Game/Materials/M_Vein")] * 6 +
             [(unreal.ENLootType.IRON, "/Game/Materials/M_VeinIron")] * 2 +
             [(unreal.ENLootType.COAL, "/Game/Materials/M_VeinCoal")] * 2)
    vein_positions = [(-950, -350), (-950, 350), (-400, -600), (100, 600), (600, -400), (950, 200),
                      (350, 300), (-600, 100), (700, 500), (-150, -500)]
    for i, ((loot, mat_path), (dx, dy)) in enumerate(zip(veins, vein_positions)):
        vein = spawn_en("ENAmberVein", unreal.Vector(CAVE_X + dx, CAVE_Y + dy, floor_z + 110.0), 0.0,
                        "Veta_{}".format(i + 1), "Caverna/Vetas",
                        {"Loot": loot, "HitsLeft": 2, "AmountPerHit": 1})
        if vein:
            vein.set_material_path(mat_path)

    # Valvula de vapor de la camara del nido.
    valve = spawn_en("ENValve", unreal.Vector(CAVE_X + 500.0, CAVE_Y - 600.0, floor_z + 60.0), 0.0,
                     "Valvula_Cueva", "Caverna", {"ValveId": "P3", "bSteamTrap": True})
    if valve:
        valve.set_material_path("/Game/Materials/M_Valve")

    # Checkpoint y recogibles de la caverna.
    spawn_en("ENCheckpoint", unreal.Vector(CAVE_X - 1000.0, CAVE_Y, floor_z + 170.0), 0.0,
             "Checkpoint_Caverna", "Caverna")
    spawn_en("ENPickup", unreal.Vector(CAVE_X - 700.0, CAVE_Y + 250.0, floor_z + 20.0), 0.0,
             "Pickup_Aceite_Cueva", "Caverna/Recogibles",
             {"Loot": unreal.ENLootType.OIL, "Amount": 50})
    spawn_en("ENPickup", unreal.Vector(CAVE_X + 800.0, CAVE_Y - 300.0, floor_z + 20.0), 0.0,
             "Pickup_Bengala_Cueva", "Caverna/Recogibles",
             {"Loot": unreal.ENLootType.FLARE, "Amount": 1})


# --------------------------------------------------------------------------- rotulos
def build_labels():
    labels = [
        ("MERCADER", 20, -66, 3.0),
        ("PORTON P1", 4, -52, 4.0),
        ("ESTABLO Y CORRAL", 26, -34, 6.0),
        ("HUERTO", -48, 6, 3.5),
        ("TORRE DE AGUA", 18, 14, 15.0),
        ("CAMPAMENTO DE CAZADORES", 35, 24, 4.0),
        ("TALLER Y REFINERIA", 50, -6, 6.5),
        ("BOCA DE LA MINA", 62, 40, 6.0),
        ("SALA DE MAQUINAS", 0, 40, 11.0),
        ("GENERADOR Y BARRANCO", 22, 50, 4.0),
        ("CASA DE MANDO", -50, 38, 5.0),
    ]
    for text, east, north, z in labels:
        try:
            actor = spawn(unreal.TextRenderActor, W(east, north, z * 100.0), 180.0,
                          "Rotulo_" + text.replace(" ", "_"), "Rotulos")
            if actor is None:
                continue
            comp = actor.get_component_by_class(unreal.TextRenderComponent)
            try:
                comp.set_text(text)
            except Exception:
                comp.set_editor_property("Text", text)
            comp.set_editor_property("WorldSize", 140.0)
            comp.set_editor_property("TextRenderColor", unreal.Color(240, 190, 110, 255))
        except Exception as exc:
            log("Rotulo {} fallo: {}".format(text, exc))


# --------------------------------------------------------------------------- main
def main():
    log("=== Construyendo Nivel 1 ===")

    materials = build_materials()

    # Un mapa de trabajo nuevo en cada pasada: new_level no sobrescribe.
    try:
        for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/Maps", recursive=False):
            short = asset_path.split("/")[-1].split(".")[0]
            if short.startswith("L_Work_"):
                unreal.EditorAssetLibrary.delete_asset(asset_path)
    except Exception as exc:
        log("Limpieza de mapas de trabajo fallo: {}".format(exc))

    work_path = "/Game/Maps/L_Work_{}".format(int(time.time()))
    try:
        created = LES.new_level(work_path)
        log("new_level {} -> {}".format(work_path, created))
    except Exception as exc:
        log("new_level fallo: {}".format(exc))

    world = unreal.EditorLevelLibrary.get_editor_world()
    try:
        settings = world.get_world_settings()
        settings.set_editor_property("bForceNoPrecomputedLighting", True)
    except Exception as exc:
        log("WorldSettings: {}".format(exc))

    stages = [
        ("blockout", lambda: spawn_blockout(materials)),
        ("luces", build_lighting),
        ("gameplay", lambda: build_gameplay(materials)),
        ("caverna", lambda: build_cave(materials)),
        ("rotulos", build_labels),
    ]
    for name, stage in stages:
        try:
            stage()
            log("Etapa OK: " + name)
        except Exception:
            log("ETAPA FALLIDA {}:\n{}".format(name, traceback.format_exc()))

    start = spawn(unreal.PlayerStart, W(4, -80, 120.0), 0.0, "PlayerStart", "Gameplay")
    if start:
        start.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)

    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
        saved = unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL_PATH)
        log("save_map -> {} en {}".format(saved, LEVEL_PATH))
    except Exception as exc:
        log("save_map fallo: {}".format(exc))
        try:
            LES.save_current_level()
            unreal.EditorAssetLibrary.rename_asset(work_path, LEVEL_PATH)
            log("Nivel renombrado a " + LEVEL_PATH)
        except Exception as exc2:
            log("Guardado alternativo fallo: {}".format(exc2))

    log("=== Nivel 1 construido ===")


main()
