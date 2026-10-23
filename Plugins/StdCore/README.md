# Std Core

Genre-neutral infrastructure for a small Unreal Engine 5.8 game: main menu, pause menu, settings (volumes, window mode, quality, key remapping), map transitions with a fade, persistent music and tag-based input. No gameplay logic.

Everything lives in this folder: C++ in `Source/StdCore`, assets in `Content` (mount point `/StdCore/`). The only engine plugin it depends on is Enhanced Input.

## Contents

- [Install in a new project](#install-in-a-new-project)
- [Assets to create](#assets-to-create) (specification and setup script)
- [Restyle the menus](#restyle-the-menus) (UI theme)
- [Daily use](#daily-use)
- [How it works](#how-it-works)
- [Console commands](#console-commands)
- [Tests](#tests)
- [Package the game](#package-the-game)
- [Checklist for a new project](CHECKLIST.md)
- [Usage and extension guide](GUIDE.md) (recipes for using, extending and modifying the plugin)
- [Known limitations and possible improvements](LIMITATIONS.md)

---

## Install in a new project

1. **The project must be a C++ project.** The plugin has its own C++ module and the host project must be able to compile it. In a Blueprint-only project, add any C++ class from the editor (Tools > New C++ Class) first.
2. **Copy the folder** `Plugins/StdCore` into the new project's `Plugins` folder (create it if needed). Do not copy `Binaries` or `Intermediate`.
3. **Enable the plugin** in the `.uproject` (the editor does it for you if you enable "Std Core" in Edit > Plugins):
   ```json
   { "Name": "StdCore", "Enabled": true }
   ```
4. **Regenerate project files and compile** (or open the `.uproject` and accept the rebuild). The plugin has two modules: `StdCore` (runtime) and `StdCoreTests` (editor only, never packaged). The first start of a new project compiles shaders and can take a few minutes.
5. **Project Settings**, the only steps the plugin cannot do by itself:
   - *Maps & Modes > Game Default Map* = `/StdCore/Maps/L_MainMenu`. This writes `GameDefaultMap` in `DefaultEngine.ini`.
   - *Maps & Modes > Default GameMode*: optional. The plugin maps set their own Game Mode in their World Settings. For the game's own maps either set `StdGameMode` (or a subclass) here or in each map's World Settings.
   - *Engine > Input > Default Input Component Class* must be `EnhancedInputComponent` and *Default Player Input Class* `EnhancedPlayerInput`. This is the default for projects created with UE5 templates; check it in old projects.
6. **Nothing else is needed in the `.ini` files:**
   - Volumes are saved by the plugin's own `StdUserSettings` object in `GameUserSettings.ini`; `GameUserSettingsClassName` is **not** changed.
   - Key remapping uses the Enhanced Input user settings, enabled by default in 5.8 (*Engine > Enhanced Input > Enable User Settings*). If a project turns them off, remapping stops and the log says so.
   - **Packaging.** No *Directories to Always Cook* entry is needed. During a cook, the plugin adds every package under `/StdCore` itself, plus every asset named in *Project Settings > Std Core* (for example the game's own gameplay map). It uses the engine's `ModifyCook` hook, in the editor only.
7. **Check it works.**
   - Run the [tests](#tests). `StdCore.Assets.SettingsDefaults` fails on any missing or misplaced asset.
   - Play (Standalone or PIE) from `L_MainMenu`: the menu fades in, Play goes to `L_TestGameplay` with a fade, and Escape pauses.
   - In PIE, Escape is the editor's *Stop* shortcut: test the pause in Standalone, or rebind *Stop* (see [LIMITATIONS.md](LIMITATIONS.md)).
   - Filter the Output Log on `LogStdCore`: every missing or misconfigured asset is reported there once, with the setting name and the expected path.

   This procedure was checked on a fresh *Blank* C++ project of UE 5.8, with no other change. The manual checks the tests do not cover are in [CHECKLIST.md](CHECKLIST.md).

All plugin settings are in *Project Settings > Game > Std Core* (`UStdProjectSettings`, saved in `DefaultGame.ini`). Defaults point to the assets below; change them to use the game's own assets without touching the plugin.

---

## Assets to create

The C++ code expects these assets. They already ship in `Plugins/StdCore/Content`. This table is their specification, and the reference for rebuilding them. Until an asset exists the plugin compiles, starts and degrades: missing assets are logged once and skipped. Create them **in this order**, so each one can reference the previous ones. All paths are inside the plugin content.

Audio, input, maps and the UI theme (#1–#12, #18–#20) are built by a script, see [Rebuild the assets with the script](#rebuild-the-assets-with-the-script). Python cannot edit a widget tree, so the Widget Blueprints (#13–#17) are made in the editor, by hand or through the editor's MCP UMG toolset. The Widget Blueprints hold layout and texts only: colors, fonts and shapes come from the theme (see [Restyle the menus](#restyle-the-menus)).

### 1. Audio (`/StdCore/Audio/`)

| # | Asset | Type | Settings |
|---|---|---|---|
| 1 | `SC_Master` | Sound Class | No parent. Root of the hierarchy. **Never assign sounds directly to it.** |
| 2 | `SC_Music` | Sound Class | Parent `SC_Master`. **Is UI Sound: on** (keeps playing while paused). |
| 3 | `SC_SFX` | Sound Class | Parent `SC_Master`. Is UI Sound: off (game effects stop while paused). |
| 4 | `SC_UI` | Sound Class | Parent `SC_Master`. **Is UI Sound: on** (menu sounds play while paused). |
| 5 | `SCM_Volumes` | Sound Mix | Four Sound Class Effects (adjusters), volume 1, pitch 1: `SC_Master` (Apply to Children off), `SC_Music`, `SC_SFX`, `SC_UI` (Apply to Children on). Fade In Time 0, Duration -1, Fade Out Time 0. |

Setting the parent on each child also fills `SC_Master`'s Child Classes list.

### 2. Input (`/StdCore/Input/`)

Input Actions:

| # | Asset | Value Type | Trigger when Paused | Player Mappable Key Settings (on the action) |
|---|---|---|---|---|
| 6 | `IA_Move` | Axis2D (Vector2D) | off | none: set per mapping in the IMC (4 directions) |
| 7 | `IA_Look` | Axis2D (Vector2D) | off | none (mouse look is not remappable) |
| 8 | `IA_Jump` | Digital (bool) | off | Name `Jump`, Display Name "Jump", Display Category "Actions" |
| 9 | `IA_Interact` | Digital (bool) | off | Name `Interact`, Display Name "Interact", Display Category "Actions" |
| 10 | `IA_Pause` | Digital (bool) | **on** | none (Escape and Start are fixed) |

`IMC_Default` (Input Mapping Context, #11). "Setting Behavior" is the per-mapping *Player Mappable Key Settings* behavior; *Ignore Settings* keeps a mapping out of the remapping list.

| Action | Key | Modifiers | Setting Behavior | Mappable name / display name / category |
|---|---|---|---|---|
| `IA_Move` | W | Swizzle Input Axis Values (YXZ) | Override Settings | `MoveForward` / "Move forward" / "Movement" |
| `IA_Move` | S | Swizzle (YXZ), Negate | Override Settings | `MoveBackward` / "Move backward" / "Movement" |
| `IA_Move` | A | Negate | Override Settings | `MoveLeft` / "Move left" / "Movement" |
| `IA_Move` | D | none | Override Settings | `MoveRight` / "Move right" / "Movement" |
| `IA_Move` | Gamepad Left Thumbstick 2D-Axis | Dead Zone | Ignore Settings | |
| `IA_Look` | Mouse XY 2D-Axis | Negate (Y only) | Ignore Settings | |
| `IA_Look` | Gamepad Right Thumbstick 2D-Axis | Dead Zone, Negate (Y only) | Ignore Settings | |
| `IA_Jump` | Space Bar | none | Inherit Settings From Action | (`Jump`) |
| `IA_Jump` | Gamepad Face Button Bottom | none | Ignore Settings | |
| `IA_Interact` | E | none | Inherit Settings From Action | (`Interact`) |
| `IA_Interact` | Gamepad Face Button Left | none | Ignore Settings | |
| `IA_Pause` | Escape | none | Ignore Settings | |
| `IA_Pause` | Gamepad Special Right (Start) | none | Ignore Settings | |

The Y negation on look follows the UE5 Third Person template; flip it if the camera feels inverted.

`DA_InputConfig_Core` (#12), Data Asset of class `StdInputConfig`, *Input Actions*:

| Input Action | Input Tag |
|---|---|
| `IA_Move` | `InputTag.Move` |
| `IA_Look` | `InputTag.Look` |
| `IA_Jump` | `InputTag.Jump` |
| `IA_Interact` | `InputTag.Interact` |
| `IA_Pause` | `InputTag.Pause` |

The tags are native (defined in C++), they appear in the tag picker without any `.ini` entry.

### 3. Widgets (`/StdCore/UI/`)

Each Widget Blueprint must have the C++ class as parent and contain the widgets below **with exactly these names** (`BindWidget`: the Blueprint does not compile without them). Every button must stay focusable (the default) for keyboard and gamepad navigation.

| # | Asset | Parent class | Widgets (name: type, purpose) |
|---|---|---|---|
| 13 | `WBP_KeyBindingRow` | `StdKeyBindingRowWidget` | `Text_ActionName`: Text Block, action name. `KeySelector`: Input Key Selector, shows and captures the key. |
| 14 | `WBP_Settings` | `StdSettingsWidget` | `Button_TabAudioVideo`, `Button_TabControls`: Button, tab buttons. `Switcher_Tabs`: Widget Switcher, child 0 = audio/video panel, child 1 = controls panel. `Slider_MasterVolume`, `Slider_MusicVolume`, `Slider_SfxVolume`: Slider, volumes 0..1. `Button_WindowMode`: Button, cycles window mode. `Text_WindowMode`: Text Block, current window mode. `Button_Quality`: Button, cycles quality. `Text_Quality`: Text Block, current quality. `Box_KeyBindings`: Vertical Box (or Scroll Box), left empty, filled at runtime. `Button_ResetDefaults`: Button, restores default keys. `Text_KeyBindingMessage`: Text Block, swap/refusal messages. `Button_Back`: Button, saves and closes. |
| 15 | `WBP_Credits` | `StdCreditsWidget` | `ScrollBox_Credits`: Scroll Box, scrolls automatically. `Text_Credits`: Text Block inside the scroll box, the credits (or fill *Credits Text* in the Class Defaults). `Button_Back`: Button, closes. |
| 16 | `WBP_PauseMenu` | `StdPauseMenuWidget` | `Button_Resume`, `Button_Settings`, `Button_MainMenu`, `Button_Quit`: Button. |
| 17 | `WBP_MainMenu` | `StdMainMenuWidget` | `Button_Play`, `Button_Settings`, `Button_Credits`, `Button_Quit`: Button. |

Optional widgets (`BindWidgetOptional`), used when present:

| Asset | Widget | Purpose |
|---|---|---|
| `WBP_Settings` | `Text_MasterVolumeValue`, `Text_MusicVolumeValue`, `Text_SfxVolumeValue`: Text Block | The volume as a percentage, next to its slider. |
| `WBP_MainMenu` | `Text_Version`: Text Block | Filled with *Project Settings > Description > Project Version*. |

Names the theme recognizes (see [Restyle the menus](#restyle-the-menus)): `Border_Background` (full-screen dim layer), `Border_Panel` (rounded panel), `Text_Title`, `Text_Heading*`, `Text_Version`, and `Button_Tab*` for tab buttons.

The current layouts, at a reference resolution of 1920×1080:
- **`WBP_MainMenu`**: a column on the left with the title and four buttons, and the version at the bottom right.
- **`WBP_PauseMenu`**, **`WBP_Credits`**: a centered panel.
- **`WBP_Settings`**: a centered panel with the title and tabs on top. *Audio / Video* has rows of label, control and value. *Controls* has a scrolling list of key rows, then the message and *Reset to defaults*. *Back* is at the bottom right.

Texts (button labels, window mode and quality names) are ordinary properties of the Blueprints; the C++ defaults are English. The volume sliders have *Requires Controller Lock* off: Left and Right change the value as soon as the slider has the focus. The key list's Scroll Box has *Scroll When Focus Changes* on, so it follows keyboard and gamepad navigation.

In the testbed project, `Scripts/stdcore_build_widgets.py` rebuilds these five layouts through the editor's MCP server.

### UI theme (`/StdCore/UI/`)

| # | Asset | Type | Settings |
|---|---|---|---|
| 20 | `DA_UITheme` | Data Asset of class `StdUITheme` | None needed: the C++ defaults are the look (dark panels, light text, blue accent). Edit it to restyle the menus. |

### 4. Maps (`/StdCore/Maps/`)

| # | Asset | World Settings > GameMode Override | Content |
|---|---|---|---|
| 18 | `L_MainMenu` | `StdMenuGameMode` | Empty is fine (the menu is UI only). Optionally a camera, a light and a backdrop. |
| 19 | `L_TestGameplay` | `StdGameMode` | A floor (a scaled cube or plane with collision), a Directional Light (plus Sky Light if desired) and a Player Start. |

### 5. Project settings references

The defaults of *Project Settings > Game > Std Core* already point to all of the above. Nothing to set unless an asset gets a different name or path. In the testbed project, *Game Default Map* = `L_MainMenu`.

### Rebuild the assets with the script

`Content/Python/stdcore_setup.py` creates or updates assets #1–#12 and #18–#20 from the tables above. It is only needed to rebuild them or to check them: a project that copies the plugin folder already has the assets.

- **Requires** the *Python Editor Script Plugin* and *Editor Scripting Utilities* to be enabled in the project. The Unreal MCP plugin enables both.
- **From the editor**: switch the Output Log to *Python* and run `py "<project>/Plugins/StdCore/Content/Python/stdcore_setup.py"`.
- **Headless**, with every editor of the project closed:
  ```
  UnrealEditor-Cmd.exe <project>.uproject -run=pythonscript -script="<project>/Plugins/StdCore/Content/Python/stdcore_setup.py"
  ```
- **Stages**: add `audio`, `input`, `maps` or `ui` after the script path (inside the quotes) to run only those. The `ui` stage only creates `DA_UITheme` when it is missing and never resets its values.
- **Idempotent**: each asset is created when missing, compared with the specification and saved only when something differs. The log ends with `[StdCore setup] Done: N created, N updated, N unchanged, N errors`; a second run reports 0 created and 0 updated.
- **It restores the specification.** Hand edits to these plugin assets are reverted on the next run (for example an extra `IMC_Default` row or a moved light). Put game-specific changes in the game's own assets instead (see *Daily use*).
- **Map actors** are matched by label (`StdSun`, `StdSkyLight`, `StdSkyAtmosphere`, `StdFloor`, `StdPlayerStart`). Other actors in the plugin maps are left alone.

---

## Daily use

### Go to a map with the transition

C++:
```cpp
#include "Flow/StdFlowSubsystem.h"

if (UStdFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UStdFlowSubsystem>())
{
    Flow->TravelToLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/L_Level2.L_Level2"))));
}
```
Blueprint: *Get Game Instance Subsystem (Std Flow Subsystem)* > **Travel To Level** (pick the map in the soft reference pin). Also **Return To Main Menu**, **Start Gameplay** (map from the settings) and **Quit Game**. A request during a transition is ignored and returns false; a map that does not exist logs an error and nothing happens. Do not call `Open Level` directly: it skips the fade and the input block.

### Start a music track

*Get Game Instance Subsystem (Std Music Subsystem)* > **Play Music** (sound, fade in seconds). Playing another track crossfades; asking for the track already playing does nothing; **Stop Music** fades out. Music continues through map changes and while paused. Typical place: the BeginPlay of the level or of the game mode.

### Assign a sound to the right Sound Class

Open the Sound Wave / Sound Cue / MetaSound and set *Sound Class*:
- gameplay effects: `SC_SFX` (stop while paused);
- menu and HUD sounds: `SC_UI` (play while paused);
- music: `SC_Music` (the music subsystem forces it anyway).

Never use `SC_Master` directly. To add a finer category (for example `SC_Footsteps`), create a Sound Class with parent `SC_SFX` (or `SC_Music`/`SC_UI`): it follows that branch's volume slider automatically, no new adjuster needed. Its own Volume property can still balance it within the branch.

### Add a gameplay action with its tag, remappable, without touching the plugin

1. **Tag**: Project Settings > GameplayTags > add `InputTag.Dash` (saved in the game's `DefaultGameplayTags.ini`), or declare it in the game's C++ with `UE_DEFINE_GAMEPLAY_TAG`.
2. **Input Action** in the game's content, e.g. `/Game/Input/IA_Dash` (Digital). To make it remappable set *Player Mappable Key Settings*: Name `Dash`, Display Name "Dash", Display Category "Actions" (or any new category: the Controls tab groups by category).
3. **Input Config**: create `/Game/Input/DA_InputConfig_Game` (class `StdInputConfig`) with `IA_Dash` / `InputTag.Dash`, and add it to *Std Core > Input Configs* next to the plugin's.
4. **Mapping Context**: create `/Game/Input/IMC_Game` with Left Shift > `IA_Dash` (Inherit Settings From Action) and a gamepad button (Ignore Settings), then add it to *Std Core > Mapping Contexts* (priority above 0 to win conflicts with `IMC_Default`).

The new entry appears in the Controls tab by itself.

### Bind to a tag

C++ (works on any `UEnhancedInputComponent`, no custom input component class):
```cpp
#include "Input/StdInputBinding.h"
#include "StdGameplayTags.h"

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

    StdInput::BindActionByTag(Input, StdGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
    StdInput::BindActionByTag(Input, StdGameplayTags::InputTag_Jump, ETriggerEvent::Started, this, &AMyCharacter::Jump);
    // A game tag: FGameplayTag::RequestGameplayTag(TEXT("InputTag.Dash"))
}

void AMyCharacter::Move(const FInputActionValue& Value) { const FVector2D Axis = Value.Get<FVector2D>(); /* ... */ }
```
An overload takes a specific `UStdInputConfig*` as second argument.

Blueprint: with `AStdPlayerController` (or a subclass) as the player controller, bind to its **On Input Tag** event, for example in the pawn's *Event Possessed*: *Cast To StdPlayerController* > *Bind Event to On Input Tag*, then *Switch on Gameplay Tag*. The event gives the tag, the value (*Break Input Action Value* / *To Vector2D*) and the phase (Started, Triggered, Completed, Canceled): use Started for presses, Triggered for continuous input like movement. Blueprint subclasses of the controller can also override the **Receive Input Tag** event. The pure node **Get Input Action For Tag** returns the asset when a Blueprint really needs it.

### Show the current key of an action

**Get Key Text For Input Tag** (player controller, tag) returns "E", "Space Bar"... following the remapping; **Get Key For Input Tag** returns the `FKey` (for an icon lookup). For a hint that updates when the player remaps, refresh it on *Std Input Subsystem > On Key Mappings Changed*. C++: `UStdInputSubsystem::GetKeyDisplayTextForTag(Tag)`.

```
Format Text: "Press {Key} to interact"   Key = Get Key Text For Input Tag(Get Owning Player, InputTag.Interact)
```

### Restyle the menus

All menus take their look from one asset, `DA_UITheme` (*Project Settings > Std Core > Theme*). Edit it and every menu changes, in the game and in the Widget Blueprint designer.

| Group | Fields |
|---|---|
| Colors | *Accent Color* (focus, hover, pressed, category headers), *Panel Color*, *Backdrop Color*, *Button Color*, *Text Color*, *Muted Text Color* (version, unselected tabs, slider bars), *Text On Accent Color* |
| Shape | *Corner Radius*, *Button Padding* |
| Fonts | *Title Font*, *Heading Font*, *Body Font*, *Button Font* (the engine's Roboto by default; any imported font works) |
| Sounds | *Hover Sound*, *Press Sound* (optional; give them the `SC_UI` Sound Class so they play while paused) |

**How widgets pick their style.** The theme styles widgets by type and name:

| Widget | Style |
|---|---|
| Buttons | Button style. |
| `Button_Tab*` | Tab style; the selected tab is filled. |
| Labels inside buttons | Button font; the color follows the button's state. |
| Sliders, key selectors | Their own style. |
| `Text_Title`, `Text_Heading*` | Title font, heading font. |
| Other texts | Body font (`Text_Version` in the muted color). |
| `Border_Background`, `Border_Panel` | Backdrop color, panel color. |

**Where it applies.**
- It runs in *Pre Construct*, before the Blueprint's own *Pre Construct*, so a Blueprint can still override single widgets there.
- To keep one screen's designer style, untick **Apply Theme** in its Class Defaults.
- To use the theme in the game's own widgets, call **Apply Std UI Theme** in their *Pre Construct*.
- With the *Theme* setting empty, the Widget Blueprints use their designer values. The plugin's ones fall back to dark layers and dark button labels.

**Focus is always visible.** The focused button, slider or key selector shows its hovered look, and hovering with the mouse moves the focus. This works with or without a theme; untick **Highlight Focus** on a screen to turn it off.

**Bigger changes.** For layout changes or a completely different look, create the game's own Widget Blueprints, for example by duplicating `WBP_MainMenu` into `/Game/UI/`.
- Keep the parent class and the widget names.
- Point *Std Core > UI* to the new classes; the plugin's Blueprints stay untouched.
- To restyle the Controls tab's category header, override **Create Category Header** in the settings Blueprint.

---

## How it works

- **Settings**: `UStdSettingsSubsystem` (game instance). Volumes are stored in `UStdUserSettings` (`[/Script/StdCore.StdUserSettings]` in `Saved/Config/<Platform>/GameUserSettings.ini`), applied with `SetSoundMixClassOverride` + `PushSoundMixModifier` on `SCM_Volumes`: `SC_Music` = Music x Master, `SC_SFX` = SFX x Master, `SC_UI` = Master, `SC_Master` = 1. The mix is pushed again on every new world. Sliders apply immediately; Back (or leaving the pause menu) saves. Window mode and quality use `UGameUserSettings`; at startup the saved scalability is loaded before the settings are read, and a mixed ("Custom") level is reset to Medium.
- **Transitions**: `UStdFlowSubsystem` draws a Slate overlay on the game viewport client, which survives map loads. Fade out > `OpenLevel` > new world > short delay > fade in, all on real time. Input is blocked by a Slate input pre-processor during the whole transition; travel and pause requests are refused. A map that does not arrive within *Travel Timeout Seconds* (or a travel failure) removes the fade, unblocks input and logs an error.
- **Input**: `UStdInputSubsystem` (local player) adds the configured mapping contexts and registers them with the Enhanced Input user settings when the player controller changes, so saved keys apply before the first input. Remapping covers keyboard and mouse entries only; gamepad keys, axes and *Reserved Keys* (Escape) are refused; a key already used by another entry swaps the two. Remapped keys are saved by Enhanced Input in `Saved/SaveGames/EnhancedInputUserSettings.sav`.
- **Pause**: `AStdPlayerController` pauses on `InputTag.Pause`. While the pause menu is open the input mode is UI only, so an input pre-processor handles the pause keys: Escape and gamepad B go back one screen (Settings saves) or resume from the pause menu itself; Start resumes from anywhere. Without a pause menu widget the game still pauses and Escape resumes.
- **Menus**: `UStdScreenWidget` handles:
  - **focus**: the first button on open, taken back if it falls on the viewport;
  - **screen stacking**: Settings and Credits open over their menu and return to it;
  - **Back**;
  - **the theme**, applied in `NativePreConstruct`;
  - **the focus highlight**: each tick, the focused button, slider or key selector gets its hovered style and gets its own style back when the focus leaves.
- **Theme**: `UStdUITheme` builds button, tab, slider and text styles from its colors with rounded-box brushes (no textures). `StdUITheme::ApplyToWidget` assigns them by widget type and name.

---

## Console commands

Development builds only:

| Command | Effect |
|---|---|
| `Std.Travel /Path/To/Map` | Travel with the fade, e.g. `Std.Travel /Engine/Maps/Entry`. |
| `Std.MainMenu` | Travel to the main menu map. |
| `Std.TogglePause` | Pause or resume through `AStdPlayerController`. |
| `Std.Volume master\|music\|sfx 0..1` | Set and save a volume. |
| `Std.Music /Path/To/Sound` / `Std.Music stop` | Play a track through the music subsystem, or stop it. |
| `Std.PlaySound /Path/To/Sound [ui]` | Play a 2D sound with `SC_SFX` (stops while paused) or, with `ui`, `SC_UI`. |
| `Std.DumpInput` | Log the remappable keys and the key of each plugin tag. |

---

## Tests

Automation tests live in the editor-only module `StdCoreTests`, under the `StdCore.` prefix:

| Test | Checks |
|---|---|
| `StdCore.Assets.SettingsDefaults` | Every asset and widget class named in *Project Settings > Std Core* loads, with the right class. |
| `StdCore.Assets.Audio` | Sound Class hierarchy, *Is UI Sound* flags, the four `SCM_Volumes` adjusters and their *Apply to Children*. |
| `StdCore.Assets.Input` | Value types, *Trigger when Paused* only on `IA_Pause`, the five tags of `DA_InputConfig_Core`, the six remappable entries of `IMC_Default` in order. |
| `StdCore.Settings.Volumes` | The volume formula (`StdAudio::ComputeClassVolumes`) and volume clamping. |
| `StdCore.Theme.Styles` | The styles built by `UStdUITheme`. |
| `StdCore.Flow.Play` | In PIE: main menu shown, a missing map refused, Play travels to the gameplay map, pause and resume, back to the main menu. |
| `StdCore.Input.Remap` | In PIE: reserved, gamepad and axis keys refused, a swap, an unchanged and a rejected remap, the keys after a reset. It leaves the default keys saved. |

**Run them:**
- **In the editor:** *Tools > Session Frontend > Automation*, filter `StdCore`, tick and *Start Tests*.
- **From the command line**, with the project's editor closed (about 15 seconds after the editor has started):
  ```
  UnrealEditor-Cmd.exe <project>.uproject -ExecCmds="Automation RunTests StdCore;Quit" -unattended -nullrhi -nosplash -log
  ```
  Each test logs `Test Completed. Result={Success}` (or `{Fail}` with the reason) under `LogAutomationController`.

---

## Package the game

*Platforms > Windows > Package Project*, or from the command line:
```
RunUAT.bat BuildCookRun -project=<project>.uproject -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory=<output folder> -unattended
```
- The result is in `<output folder>/Windows`: run `<Project>.exe`.
- **What gets cooked.** The plugin adds its own assets and the assets named in its settings to the cook; the cook log says `Std Core: N packages added to the cook.`
- **Checking the package.** `UnrealPak.exe <output>/Windows/<Project>/Content/Paks/<Project>-Windows.utoc -List` lists the packaged files: every `Plugins/StdCore/Content/...` asset, both maps included, must be there.
- **A project named `Test`.** Unreal also has a build configuration called *Test*, so the game target cannot be given by name: build it with `Build.bat -TargetType=Game Win64 Shipping -Project=<path>`, then run `BuildCookRun` without `-build`. Other project names work with the single command above.
- **Development builds** keep the log (`<output>/Windows/<Project>/Saved/Logs`), the console and the `Std.*` commands.
- **Shipping builds** have neither log nor console.
- A packaged game saves its settings and keys in `%LOCALAPPDATA%\<Project>\Saved`.
