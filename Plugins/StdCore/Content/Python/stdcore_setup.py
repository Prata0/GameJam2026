"""Creates or updates the Std Core plugin assets (audio, input, maps).

Idempotent: every asset is created when missing, compared with the specification below and saved only when
something differs. A second run reports 0 created and 0 updated. The Widget Blueprints are not handled here:
Python cannot edit a widget tree.

Run it from the editor (Output Log, Python mode):
    py "<project>/Plugins/StdCore/Content/Python/stdcore_setup.py"
or headless, with the editor closed:
    UnrealEditor-Cmd.exe <project>.uproject -run=pythonscript -script="<path>/stdcore_setup.py"

Optional arguments select stages: audio, input, maps (default: all of them).
Requires the Python Editor Script Plugin and Editor Scripting Utilities.
"""

import sys

import unreal

ROOT = "/StdCore"
AUDIO = ROOT + "/Audio"
INPUT = ROOT + "/Input"
MAPS = ROOT + "/Maps"

LOG_PREFIX = "[StdCore setup]"


class Report:
    def __init__(self):
        self.rows = []
        self.errors = []

    def add(self, asset_path, status, details=""):
        self.rows.append((asset_path, status, details))
        unreal.log("{} {}: {}{}".format(LOG_PREFIX, asset_path, status, " (" + details + ")" if details else ""))

    def error(self, asset_path, message):
        self.errors.append((asset_path, message))
        unreal.log_error("{} {}: {}".format(LOG_PREFIX, asset_path, message))

    def summary(self):
        counts = {status: sum(1 for row in self.rows if row[1] == status) for status in ("created", "updated", "unchanged")}
        unreal.log("{} Done: {} created, {} updated, {} unchanged, {} errors.".format(
            LOG_PREFIX, counts["created"], counts["updated"], counts["unchanged"], len(self.errors)))


REPORT = Report()


# ---------------------------------------------------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------------------------------------------------

def object_path(package_path):
    return package_path + "." + package_path.rsplit("/", 1)[1]


def load(package_path):
    if unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return unreal.EditorAssetLibrary.load_asset(package_path)
    return None


def load_or_create(package_path, asset_class, factory):
    """Returns (asset, created)."""
    asset = load(package_path)
    if asset:
        if not isinstance(asset, asset_class):
            raise RuntimeError("exists but is a {}, expected {}".format(asset.get_class().get_name(), asset_class.__name__))
        return asset, False
    folder, name = package_path.rsplit("/", 1)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_class, factory)
    if not asset:
        raise RuntimeError("could not be created")
    return asset, True


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("could not be saved")


def make_key(name):
    key = unreal.Key()
    key.set_editor_property("key_name", name)
    return key


def make_tag(name):
    tag = unreal.GameplayTag()
    if not tag.import_text('(TagName="{}")'.format(name)) or str(tag.get_editor_property("tag_name")) != name:
        raise RuntimeError("gameplay tag '{}' is not registered".format(name))
    return tag


def path_of(obj):
    return obj.get_path_name() if obj else None


def run_step(package_path, step):
    """Runs one ensure_* step: the step returns (asset, created, changed) and saves nothing itself."""
    try:
        asset, created, changed = step()
        if created or changed:
            save(asset)
        REPORT.add(package_path, "created" if created else "updated" if changed else "unchanged")
        return asset
    except Exception as exc:  # Reported per asset; the other assets still run.
        REPORT.error(package_path, str(exc))
        return None


def set_if_different(obj, prop, value):
    """Sets an editor property when it differs; returns True when it changed."""
    if obj.get_editor_property(prop) == value:
        return False
    obj.set_editor_property(prop, value)
    return True


# ---------------------------------------------------------------------------------------------------------------------
# Audio
# ---------------------------------------------------------------------------------------------------------------------

SOUND_CLASSES = [
    # name, is UI sound
    ("SC_Master", False),
    ("SC_Music", True),
    ("SC_SFX", False),
    ("SC_UI", True),
]
CHILD_CLASSES = ["SC_Music", "SC_SFX", "SC_UI"]


def ensure_sound_class(name, is_ui_sound):
    def step():
        sound_class, created = load_or_create(AUDIO + "/" + name, unreal.SoundClass, unreal.SoundClassFactory())
        properties = sound_class.get_editor_property("properties")
        changed = False
        if properties.get_editor_property("is_ui_sound") != is_ui_sound:
            properties.set_editor_property("is_ui_sound", is_ui_sound)
            sound_class.set_editor_property("properties", properties)
            changed = True
        return sound_class, created, changed
    return run_step(AUDIO + "/" + name, step)


def ensure_sound_class_hierarchy(classes):
    """SC_Master's Child Classes; the engine sets each child's Parent Class when the list changes."""
    master = classes.get("SC_Master")
    children = [classes.get(name) for name in CHILD_CLASSES]
    if not master or not all(children):
        REPORT.error(AUDIO + "/SC_Master", "hierarchy skipped: a Sound Class is missing")
        return

    current = [path_of(child) for child in master.get_editor_property("child_classes")]
    wanted = [path_of(child) for child in children]
    parents_ok = all(path_of(child.get_editor_property("parent_class")) == path_of(master) for child in children)
    if current == wanted and parents_ok and not master.get_editor_property("parent_class"):
        REPORT.add(AUDIO + "/SC_Master (hierarchy)", "unchanged")
        return

    try:
        if current != wanted:
            # USoundClass::PostEditChangeProperty parents only the first new child per change: add them one by one.
            master.set_editor_property("child_classes", [])
            for count in range(1, len(children) + 1):
                master.set_editor_property("child_classes", children[:count])
        for child in children:
            if path_of(child.get_editor_property("parent_class")) != path_of(master):
                raise RuntimeError("{} did not get SC_Master as parent".format(child.get_name()))
        for asset in [master] + children:
            save(asset)
        REPORT.add(AUDIO + "/SC_Master (hierarchy)", "updated")
    except Exception as exc:
        REPORT.error(AUDIO + "/SC_Master (hierarchy)", str(exc))


def ensure_sound_mix(classes):
    package_path = AUDIO + "/SCM_Volumes"

    def step():
        if not all(classes.get(name) for name, _ in SOUND_CLASSES):
            raise RuntimeError("a Sound Class is missing")
        mix, created = load_or_create(package_path, unreal.SoundMix, unreal.SoundMixFactory())

        wanted = [(path_of(classes[name]), name != "SC_Master") for name, _ in SOUND_CLASSES]
        current = [(path_of(adj.get_editor_property("sound_class_object")), adj.get_editor_property("apply_to_children"),
                    adj.get_editor_property("volume_adjuster"), adj.get_editor_property("pitch_adjuster"))
                   for adj in mix.get_editor_property("sound_class_effects")]
        changed = False
        if current != [(path, children, 1.0, 1.0) for path, children in wanted]:
            adjusters = []
            for name, _ in SOUND_CLASSES:
                adjuster = unreal.SoundClassAdjuster()
                adjuster.set_editor_property("sound_class_object", classes[name])
                adjuster.set_editor_property("volume_adjuster", 1.0)
                adjuster.set_editor_property("pitch_adjuster", 1.0)
                adjuster.set_editor_property("apply_to_children", name != "SC_Master")
                adjusters.append(adjuster)
            mix.set_editor_property("sound_class_effects", adjusters)
            changed = True

        changed |= set_if_different(mix, "fade_in_time", 0.0)
        changed |= set_if_different(mix, "duration", -1.0)
        changed |= set_if_different(mix, "fade_out_time", 0.0)
        return mix, created, changed
    run_step(package_path, step)


def setup_audio():
    classes = {}
    for name, is_ui_sound in SOUND_CLASSES:
        classes[name] = ensure_sound_class(name, is_ui_sound)
    ensure_sound_class_hierarchy(classes)
    ensure_sound_mix(classes)


# ---------------------------------------------------------------------------------------------------------------------
# Input
# ---------------------------------------------------------------------------------------------------------------------

V = unreal.InputActionValueType
INPUT_ACTIONS = [
    # name, value type, trigger when paused, player mappable settings (name, display name, category) or None
    ("IA_Move", V.AXIS2D, False, None),
    ("IA_Look", V.AXIS2D, False, None),
    ("IA_Jump", V.BOOLEAN, False, ("Jump", "Jump", "Actions")),
    ("IA_Interact", V.BOOLEAN, False, ("Interact", "Interact", "Actions")),
    ("IA_Pause", V.BOOLEAN, True, None),
]

B = unreal.PlayerMappableKeySettingBehaviors
INHERIT, OVERRIDE, IGNORE = B.INHERIT_SETTINGS_FROM_ACTION, B.OVERRIDE_SETTINGS, B.IGNORE_SETTINGS

# Modifier descriptions: ("swizzle",), ("negate",) all axes, ("negate_y",), ("deadzone",)
MAPPINGS = [
    # action, key, modifiers, setting behavior, player mappable settings (override only)
    ("IA_Move", "W", [("swizzle",)], OVERRIDE, ("MoveForward", "Move forward", "Movement")),
    ("IA_Move", "S", [("swizzle",), ("negate",)], OVERRIDE, ("MoveBackward", "Move backward", "Movement")),
    ("IA_Move", "A", [("negate",)], OVERRIDE, ("MoveLeft", "Move left", "Movement")),
    ("IA_Move", "D", [], OVERRIDE, ("MoveRight", "Move right", "Movement")),
    ("IA_Move", "Gamepad_Left2D", [("deadzone",)], IGNORE, None),
    ("IA_Look", "Mouse2D", [("negate_y",)], IGNORE, None),
    ("IA_Look", "Gamepad_Right2D", [("deadzone",), ("negate_y",)], IGNORE, None),
    ("IA_Jump", "SpaceBar", [], INHERIT, None),
    ("IA_Jump", "Gamepad_FaceButton_Bottom", [], IGNORE, None),
    ("IA_Interact", "E", [], INHERIT, None),
    ("IA_Interact", "Gamepad_FaceButton_Left", [], IGNORE, None),
    ("IA_Pause", "Escape", [], IGNORE, None),
    ("IA_Pause", "Gamepad_Special_Right", [], IGNORE, None),
]

INPUT_TAGS = [
    ("IA_Move", "InputTag.Move"),
    ("IA_Look", "InputTag.Look"),
    ("IA_Jump", "InputTag.Jump"),
    ("IA_Interact", "InputTag.Interact"),
    ("IA_Pause", "InputTag.Pause"),
]


def describe_key_settings(settings):
    if not settings:
        return None
    return (str(settings.get_editor_property("name")), str(settings.get_editor_property("display_name")),
            str(settings.get_editor_property("display_category")))


def make_key_settings(outer, spec):
    settings = unreal.new_object(unreal.PlayerMappableKeySettings, outer=outer)
    settings.set_editor_property("name", spec[0])
    settings.set_editor_property("display_name", unreal.Text(spec[1]))
    settings.set_editor_property("display_category", unreal.Text(spec[2]))
    return settings


def ensure_input_action(name, value_type, trigger_when_paused, key_settings):
    package_path = INPUT + "/" + name

    def step():
        action, created = load_or_create(package_path, unreal.InputAction, unreal.InputAction_Factory())
        changed = set_if_different(action, "value_type", value_type)
        changed |= set_if_different(action, "trigger_when_paused", trigger_when_paused)
        if describe_key_settings(action.get_editor_property("player_mappable_key_settings")) != key_settings:
            action.set_editor_property("player_mappable_key_settings",
                                       make_key_settings(action, key_settings) if key_settings else None)
            changed = True
        return action, created, changed
    return run_step(package_path, step)


def describe_modifier(modifier):
    if isinstance(modifier, unreal.InputModifierSwizzleAxis):
        return ("swizzle",) if modifier.get_editor_property("order") == unreal.InputAxisSwizzle.YXZ else ("swizzle?",)
    if isinstance(modifier, unreal.InputModifierNegate):
        axes = (modifier.get_editor_property("x"), modifier.get_editor_property("y"), modifier.get_editor_property("z"))
        return {(True, True, True): ("negate",), (False, True, False): ("negate_y",)}.get(axes, ("negate?",))
    if isinstance(modifier, unreal.InputModifierDeadZone):
        return ("deadzone",)
    return (modifier.get_class().get_name(),)


def make_modifier(outer, spec):
    kind = spec[0]
    if kind == "swizzle":
        modifier = unreal.new_object(unreal.InputModifierSwizzleAxis, outer=outer)
        modifier.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
    elif kind in ("negate", "negate_y"):
        modifier = unreal.new_object(unreal.InputModifierNegate, outer=outer)
        only_y = kind == "negate_y"
        modifier.set_editor_property("x", not only_y)
        modifier.set_editor_property("y", True)
        modifier.set_editor_property("z", not only_y)
    elif kind == "deadzone":
        modifier = unreal.new_object(unreal.InputModifierDeadZone, outer=outer)
    else:
        raise RuntimeError("unknown modifier " + kind)
    return modifier


def describe_mapping(mapping):
    behavior = mapping.get_editor_property("setting_behavior")
    return (path_of(mapping.get_editor_property("action")),
            str(mapping.get_editor_property("key").get_editor_property("key_name")),
            [describe_modifier(m) for m in mapping.get_editor_property("modifiers")],
            behavior,
            describe_key_settings(mapping.get_editor_property("player_mappable_key_settings")) if behavior == OVERRIDE else None)


def ensure_mapping_context(actions):
    package_path = INPUT + "/IMC_Default"

    def step():
        missing = [name for name in actions if not actions[name]]
        if missing:
            raise RuntimeError("missing input actions: " + ", ".join(missing))
        context, created = load_or_create(package_path, unreal.InputMappingContext, unreal.InputMappingContext_Factory())

        wanted = [(path_of(actions[a]), key, list(mods), behavior, settings) for a, key, mods, behavior, settings in MAPPINGS]
        data = context.get_editor_property("default_key_mappings")
        current = [describe_mapping(m) for m in data.get_editor_property("mappings")]
        if current == wanted:
            return context, created, False

        mappings = []
        for action_name, key, mods, behavior, settings in MAPPINGS:
            mapping = unreal.EnhancedActionKeyMapping()
            mapping.set_editor_property("action", actions[action_name])
            mapping.set_editor_property("key", make_key(key))
            mapping.set_editor_property("modifiers", [make_modifier(context, spec) for spec in mods])
            mapping.set_editor_property("setting_behavior", behavior)
            if settings:
                mapping.set_editor_property("player_mappable_key_settings", make_key_settings(context, settings))
            mappings.append(mapping)
        data.set_editor_property("mappings", mappings)
        context.set_editor_property("default_key_mappings", data)

        check = [describe_mapping(m) for m in context.get_editor_property("default_key_mappings").get_editor_property("mappings")]
        if check != wanted:
            raise RuntimeError("mappings differ after the update: {}".format(check))
        return context, created, True
    run_step(package_path, step)


def ensure_input_config(actions):
    package_path = INPUT + "/DA_InputConfig_Core"

    def step():
        missing = [name for name in actions if not actions[name]]
        if missing:
            raise RuntimeError("missing input actions: " + ", ".join(missing))
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.StdInputConfig)
        config, created = load_or_create(package_path, unreal.StdInputConfig, factory)

        wanted = [(path_of(actions[a]), tag) for a, tag in INPUT_TAGS]
        current = [(path_of(b.get_editor_property("input_action")), str(b.get_editor_property("input_tag").get_editor_property("tag_name")))
                   for b in config.get_editor_property("input_actions")]
        if current == wanted:
            return config, created, False

        bindings = []
        for action_name, tag in INPUT_TAGS:
            binding = unreal.StdInputActionBinding()
            binding.set_editor_property("input_action", actions[action_name])
            binding.set_editor_property("input_tag", make_tag(tag))
            bindings.append(binding)
        config.set_editor_property("input_actions", bindings)
        return config, created, True
    run_step(package_path, step)


def setup_input():
    actions = {}
    for name, value_type, trigger_when_paused, key_settings in INPUT_ACTIONS:
        actions[name] = ensure_input_action(name, value_type, trigger_when_paused, key_settings)
    ensure_mapping_context(actions)
    ensure_input_config(actions)


# ---------------------------------------------------------------------------------------------------------------------
# Maps
# ---------------------------------------------------------------------------------------------------------------------

MOVABLE = unreal.ComponentMobility.MOVABLE

# Actors are identified by label. Each entry: label, class, location, rotation (pitch, yaw, roll), scale,
# root component properties.
LIGHTING_ACTORS = [
    ("StdSun", unreal.DirectionalLight, (0, 0, 500), (-50, -30, 0), (1, 1, 1),
     {"mobility": MOVABLE, "atmosphere_sun_light": True}),
    ("StdSkyLight", unreal.SkyLight, (0, 0, 500), (0, 0, 0), (1, 1, 1),
     {"mobility": MOVABLE, "real_time_capture": True}),
    ("StdSkyAtmosphere", unreal.SkyAtmosphere, (0, 0, 0), (0, 0, 0), (1, 1, 1), {}),
]

GAMEPLAY_ACTORS = LIGHTING_ACTORS + [
    # A 100 cm cube scaled to 40 x 40 m, 10 cm thick, top face at z = 0.
    ("StdFloor", unreal.StaticMeshActor, (0, 0, -5), (0, 0, 0), (40, 40, 0.1),
     {"static_mesh": "/Engine/BasicShapes/Cube.Cube"}),
    ("StdPlayerStart", unreal.PlayerStart, (0, 0, 100), (0, 0, 0), (1, 1, 1), {}),
]

MAP_SPECS = [
    # name, GameMode Override, actors
    ("L_MainMenu", "StdMenuGameMode", LIGHTING_ACTORS),
    ("L_TestGameplay", "StdGameMode", GAMEPLAY_ACTORS),
]


def nearly_equal(a, b, tolerance=0.01):
    return all(abs(x - y) <= tolerance for x, y in zip(a, b))


def component_value(value):
    """Turns a spec value into the property value: asset paths are loaded."""
    if isinstance(value, str) and value.startswith("/"):
        asset = unreal.load_asset(value)
        if not asset:
            raise RuntimeError("missing engine asset " + value)
        return asset
    return value


def ensure_actor(actors_by_label, spec):
    """Spawns the actor when missing and aligns its transform and root component properties. Returns True on change."""
    label, actor_class, location, rotation, scale, properties = spec
    actor = actors_by_label.get(label)
    changed = False
    if actor and not isinstance(actor, actor_class):
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
        actor = None
    if not actor:
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            actor_class, unreal.Vector(*location), unreal.Rotator(roll=rotation[2], pitch=rotation[0], yaw=rotation[1]))
        if not actor:
            raise RuntimeError("could not spawn " + label)
        actor.set_actor_label(label)
        changed = True

    current_location = actor.get_actor_location()
    if not nearly_equal((current_location.x, current_location.y, current_location.z), location):
        actor.set_actor_location(unreal.Vector(*location), False, False)
        changed = True
    current_rotation = actor.get_actor_rotation()
    if not nearly_equal((current_rotation.pitch, current_rotation.yaw, current_rotation.roll), rotation):
        actor.set_actor_rotation(unreal.Rotator(roll=rotation[2], pitch=rotation[0], yaw=rotation[1]), False)
        changed = True
    current_scale = actor.get_actor_scale3d()
    if not nearly_equal((current_scale.x, current_scale.y, current_scale.z), scale):
        actor.set_actor_scale3d(unreal.Vector(*scale))
        changed = True

    component = actor.get_editor_property("root_component")
    if isinstance(actor, unreal.StaticMeshActor):
        component = actor.get_editor_property("static_mesh_component")
    for prop, value in properties.items():
        value = component_value(value)
        if prop == "mobility":
            if component.get_editor_property("mobility") != value:
                component.set_mobility(value)
                changed = True
        elif prop == "static_mesh":
            if path_of(component.get_editor_property("static_mesh")) != path_of(value):
                component.set_static_mesh(value)
                changed = True
        else:
            changed |= set_if_different(component, prop, value)
    return changed


def ensure_map(name, game_mode_name, actor_specs):
    package_path = MAPS + "/" + name
    try:
        level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        created = False
        if unreal.EditorAssetLibrary.does_asset_exist(package_path):
            if not level_editor.load_level(package_path):
                raise RuntimeError("could not be loaded")
        else:
            if not level_editor.new_level(package_path):
                raise RuntimeError("could not be created")
            created = True

        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if not world or path_of(world.get_outermost()) != package_path:
            raise RuntimeError("the editor world is {}, not this map".format(path_of(world)))

        changed = False
        world_settings = world.get_world_settings()
        game_mode = getattr(unreal, game_mode_name).static_class()
        if path_of(world_settings.get_editor_property("default_game_mode")) != path_of(game_mode):
            world_settings.set_editor_property("default_game_mode", game_mode)
            changed = True

        actors_by_label = {}
        for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
            actors_by_label.setdefault(actor.get_actor_label(), actor)
        for spec in actor_specs:
            changed |= ensure_actor(actors_by_label, spec)

        if created or changed:
            if not level_editor.save_current_level():
                raise RuntimeError("could not be saved")
        REPORT.add(package_path, "created" if created else "updated" if changed else "unchanged")
    except Exception as exc:
        REPORT.error(package_path, str(exc))


def setup_maps():
    for name, game_mode_name, actor_specs in MAP_SPECS:
        ensure_map(name, game_mode_name, actor_specs)


# ---------------------------------------------------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------------------------------------------------

STAGES = {
    "audio": setup_audio,
    "input": setup_input,
    "maps": setup_maps,
}


def main(args):
    selected = [arg.lower() for arg in args if not arg.startswith("-")] or list(STAGES)
    unknown = [stage for stage in selected if stage not in STAGES]
    if unknown:
        unreal.log_error("{} Unknown stage(s): {}. Valid: {}.".format(LOG_PREFIX, ", ".join(unknown), ", ".join(STAGES)))
        return
    # A commandlet starts before the asset registry has scanned the plugin content: existing assets would look missing.
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT], True)
    for stage in STAGES:
        if stage in selected:
            unreal.log("{} Stage: {}".format(LOG_PREFIX, stage))
            STAGES[stage]()
    REPORT.summary()


if __name__ == "__main__":
    main(sys.argv[1:])
