# Std Core: usage and extension guide

How to build a game on top of Std Core, extend it from the game's own content, and change the plugin itself.

| Document | Use it for |
|---|---|
| [README.md](README.md) | The reference: installation, asset specification, settings, tests, packaging. |
| [CHECKLIST.md](CHECKLIST.md) | The day the plugin is copied into a project. |
| [LIMITATIONS.md](LIMITATIONS.md) | What is missing or known to behave oddly. |
| This guide | Recipes: using the plugin, extending it, modifying it. |

Rule of thumb: **extend from the game first, modify the plugin last.** Almost everything below is done with the game's own assets and classes, so the plugin folder stays identical to the template and can be updated or copied again.

---

## Contents

1. [How the plugin is organized](#1-how-the-plugin-is-organized)
2. [Using it in a game](#2-using-it-in-a-game)
3. [Extending it from the game](#3-extending-it-from-the-game)
4. [Modifying the plugin](#4-modifying-the-plugin)
5. [Troubleshooting](#5-troubleshooting)

---

## 1. How the plugin is organized

| Piece | Class | Lifetime | What it owns |
|---|---|---|---|
| Settings | `UStdProjectSettings` | Project config | Every asset and class the plugin uses (*Project Settings > Game > Std Core*). |
| Map flow | `UStdFlowSubsystem` | Game instance | Travel with fade, input blocking, quit. |
| Player settings | `UStdSettingsSubsystem` + `UStdUserSettings` | Game instance | Volumes, window mode, quality; saved in `GameUserSettings.ini`. |
| Music | `UStdMusicSubsystem` | Game instance | One persistent track with crossfades. |
| Input | `UStdInputSubsystem` | Local player | Mapping contexts, key remapping, key lookup by tag. |
| Gameplay controller | `AStdPlayerController` | Map | Pause, pause menu, input tags as events. |
| Menu controller | `AStdMenuPlayerController` | Main menu map | Shows the main menu. |
| Game modes | `AStdGameMode`, `AStdMenuGameMode` | Map | Pick the controllers. |
| Screens | `UStdScreenWidget` and subclasses | Widget | Focus, Back, screen stack, theme, focus highlight. |
| Look | `UStdUITheme` (`DA_UITheme`) | Asset | Colors, fonts, shapes of every menu. |

A few principles explain most of the code:

- **Nothing is hard-wired.** Every asset is a soft reference in `UStdProjectSettings`, with a default inside the plugin. A missing asset is logged once (`LogStdCore`) and skipped; the plugin never asserts on content.
- **Gameplay talks in tags, not in Input Actions.** `InputTag.Jump` stays the same when the action, the key or the mapping context changes.
- **Survive map loads.** The fade overlay, the music and the volume mix live in game-instance objects and survive `OpenLevel`.
- **The plugin cooks itself.** At cook time it adds its own content and every asset named in its settings (`StdCoreModule.cpp`).

---

## 2. Using it in a game

### 2.1 Point the flow at the game's maps

1. *Project Settings > Std Core > Gameplay Map* = the first map of the game. The main menu's Play opens it.
2. In each gameplay map, *World Settings > GameMode Override* = `StdGameMode`, or the game's subclass of it (see 2.2). This gives pause, the pause menu and input tags.
3. Keep `L_MainMenu` as *Game Default Map*. To use a custom main menu map, set its GameMode Override to `StdMenuGameMode` and point *Main Menu Map* to it.

### 2.2 Use the game's own pawn and controller

Create Blueprint (or C++) subclasses rather than replacing the plugin classes:

- `BP_GameMode`, parent `StdGameMode`: set *Default Pawn Class* to the game's character, and *Player Controller Class* to `BP_PlayerController` if you need one.
- `BP_PlayerController`, parent `StdPlayerController`: put controller-level logic here (HUD creation, game-over handling).

In C++, call `Super::BeginPlay()` and `Super::SetupInputComponent()` in overrides: the plugin binds the tagged actions and the pause key there.

### 2.3 React to input

**Blueprint (pawn or controller).** Bind to the controller's **On Input Tag** event and switch on the tag:

```
Event Possessed → Cast To StdPlayerController → Bind Event to On Input Tag (OnTag)
OnTag(InputTag, Value, TriggerEvent) → Switch on Gameplay Tag
    InputTag.Move     (Triggered) → Add Movement Input with Value.Get Vector2D
    InputTag.Jump     (Started)   → Jump
    InputTag.Interact (Started)   → your interaction
```

In a Blueprint subclass of `StdPlayerController`, override **Receive Input Tag** instead.

**C++ (pawn).**

```cpp
#include "Input/StdInputBinding.h"
#include "StdGameplayTags.h"

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    StdInput::BindActionByTag(Input, StdGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
    StdInput::BindActionByTag(Input, StdGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
    StdInput::BindActionByTag(Input, StdGameplayTags::InputTag_Interact, ETriggerEvent::Started, this, &AMyCharacter::Interact);
}

void AMyCharacter::Move(const FInputActionValue& Value) { const FVector2D Axis = Value.Get<FVector2D>(); /* ... */ }
```

**Key prompts.** **Get Key Text For Input Tag** (player controller, tag) returns the current key ("E"), following remapping. Refresh prompts on *Std Input Subsystem > On Key Mappings Changed*.

### 2.4 Change maps

Always go through the flow subsystem. It fades, blocks input during the load and refuses a second request.

| Need | Call |
|---|---|
| Next level | *Get Game Instance Subsystem (Std Flow Subsystem)* > **Travel To Level** (soft reference to the map). |
| Restart the level | **Travel To Level** with a soft reference to the current map (keep one in a variable of the game mode or level). |
| Back to the menu (game over, victory) | **Return To Main Menu**. |
| Start from the configured gameplay map | **Start Gameplay**. |
| Quit with a fade | **Quit Game**. |

`Is Transitioning` tells whether a fade is running. Avoid *Open Level*: it skips the fade and the input block.

### 2.5 Control the pause

| Need | How |
|---|---|
| No pause during a cutscene or a game-over screen | Set **Can Pause** (`bCanPause`) to false on the controller, then back to true afterwards. |
| Pause from code (a "Pause" HUD button) | **Pause Game** / **Resume Game** / **Toggle Pause** on the controller. |
| Stop the game's own timers, music ducking, HUD | Bind to **On Pause Changed** (`bPaused`). |
| Something must keep running while paused | Set *Tick Even When Paused* on that actor; for sounds, use the `SC_UI` or `SC_Music` Sound Class. |

Gameplay effects should use `SC_SFX`: the engine pauses them with the game.

### 2.6 Music and sounds

- **Music:** *Get Game Instance Subsystem (Std Music Subsystem)* > **Play Music** (sound, fade). Call it in each level's BeginPlay; asking for the track already playing does nothing, so menu → level → menu keeps a single track going. **Stop Music** fades out.
- **Sound Classes:** game effects `SC_SFX`, menu and HUD sounds `SC_UI`, music `SC_Music`. Never `SC_Master`.

### 2.7 Read or change player settings from code

*Get Game Instance Subsystem (Std Settings Subsystem)*:
- **Get Volume** / **Set Volume** (Master, Music, Sfx; 0..1), applied at once;
- **Get / Set Window Mode**;
- **Get / Set Quality Level** (0 Low … 3 Epic);
- **Save Settings** writes to disk.

The Settings screen already does all of this; call these only for custom UI (for example a first-run volume prompt).

### 2.8 Make the menus the game's own

| Change | Where |
|---|---|
| Colors, fonts, corners, button sounds | `DA_UITheme`, or a copy in `/Game` set in *Std Core > Theme*. |
| Game title | `Text_Title` in `WBP_MainMenu`. |
| Credits | `Text_Credits` in `WBP_Credits`, or *Credits Text* in its Class Defaults. |
| Labels ("Play", "Window mode"...) | The texts in each Widget Blueprint; window mode and quality names in `WBP_Settings`' Class Defaults. |
| Version on the main menu | *Project Settings > Description > Project Version*. |
| Layout | A copy of the Widget Blueprint (see 3.3). |

---

## 3. Extending it from the game

### 3.1 Add a new menu screen (level select, options page, game over)

1. **Create the Widget Blueprint** in `/Game/UI/` with parent class **StdScreenWidget**.
2. **Lay it out** with the theme names, so it is styled for free: `Border_Background` for the full-screen layer, `Border_Panel` for the panel, `Text_Title` for the title, plain Buttons with a Text Block inside.
3. **Set the focus.** In *Class Defaults > Interaction*, tick *Is Focusable* and set *Desired Focus Widget* to its first button. Keyboard and gamepad then start on that button.
4. **Open it** from another screen: in that screen's Graph, on the button's *On Clicked* call **Open Child Screen** (class = your screen). The parent hides, the child gets the focus, and Escape or gamepad B close it and go back.
5. **Close it** from a button with **Close Screen**. To save or clean up when it closes, implement **On Screen Closing**.

**A screen shown on its own, during gameplay** (game over, victory), from the controller:

```
Set Can Pause = false
Create Widget (WBP_GameOver, owning player = self) → Show As Root (Z order 60)
Set Input Mode UI Only → Set Show Mouse Cursor = true
```

Its buttons call **Return To Main Menu** or **Travel To Level**: the map change removes the widget and the menu controller restores the input mode.

### 3.2 Add a button to an existing menu

Make a copy of the Widget Blueprint (3.3), then add the Button and its *On Clicked* logic in the copy's Graph. For example, a "Level select" button calling **Open Child Screen**. Keep every existing `Button_*` name: the C++ class binds them by name.

### 3.3 Replace a plugin Widget Blueprint

1. Duplicate it (for example `WBP_PauseMenu`) into `/Game/UI/`.
2. Point the matching *Std Core > UI* setting to the copy.
3. Change freely, keeping the parent class and the names listed in the README's widget table. Optional widgets (`Text_*VolumeValue`, `Text_Version`) can be removed.

The plugin's originals stay untouched and serve as reference.

### 3.4 Add remappable actions and categories

Follow the README section *Add a gameplay action with its tag, remappable, without touching the plugin*:
1. tag `InputTag.X`;
2. Input Action with *Player Mappable Key Settings*;
3. the game's Input Config;
4. the game's Mapping Context (priority above 0).

The Controls tab picks the action up, and groups it by its *Display Category*; any new category name gets its own header.

### 3.5 Add Sound Classes

Create a Sound Class with parent `SC_SFX`, `SC_Music` or `SC_UI` (for example `SC_Footsteps` under `SC_SFX`). It follows that branch's volume slider and pause behavior with no further setup.

### 3.6 Style the game's own widgets

- In a widget that is not a `StdScreenWidget` (HUD, popups), call **Apply Std UI Theme** (self) in *Event Pre Construct*. It uses the same names and types as the menus (`Button_Tab*`, `Text_Title`, `Border_Panel`...).
- Widgets derived from `StdScreenWidget` get the theme automatically. Untick *Apply Theme* to opt out.

### 3.7 Add settings the plugin does not have (mouse sensitivity, subtitles...)

The plugin has no generic settings registry. The simplest way:
1. Store the value in the game: a `SaveGame` object or a config `UObject` subclass in the game module.
2. In a copy of `WBP_Settings`, add a row inside `Box_AudioVideo` (label + slider, or a button that cycles values) and bind it in the Graph.
3. Save in **On Screen Closing**, like the plugin saves volumes on Back.

---

## 4. Modifying the plugin

Do this when a change must reach every future project. Keep the plugin genre-neutral: game logic belongs in the game.

### 4.1 Conventions

- **English everywhere:** code, comments, logs, asset texts, docs. The repository is public.
- **Logging:** `LogStdCore` for all messages. Use `StdLog::WarnOnce(Key, Message)` for anything that could repeat every frame or every call.
- **Assets** are reached through `UStdProjectSettings`:
  1. add a soft pointer;
  2. add its default path in `StdCoreDefaults`;
  3. load it with `StdSettings::LoadAsset` / `LoadClass`, which warns once and returns null.

  Never `check()` on content.
- **Widget names are an API.** A new required widget (`BindWidget`) breaks every copied Widget Blueprint. Prefer `BindWidgetOptional` and handle null.
- **Development-only tools** go in `Dev/StdConsoleCommands.cpp` (`Std.*` commands, compiled out of Shipping).
- **Comments** explain why, not what. Match the surrounding style.

### 4.2 Source map

```
Source/StdCore/
  StdCoreModule        module startup; cook hook (adds plugin content to packaged builds)
  StdLog, StdGameplayTags
  Settings/            StdProjectSettings (+ StdCoreDefaults, StdSettings::LoadAsset), StdUserSettings, StdSettingsSubsystem
  Audio/               StdMusicSubsystem
  Flow/                StdFlowSubsystem, StdInputBlocker
  Input/               StdInputConfig, StdInputBinding (BindActionByTag), StdInputSubsystem, StdInputLibrary
  Framework/           StdGameMode, StdMenuGameMode, StdPlayerController (+ StdPauseKeyProcessor), StdMenuPlayerController
  UI/                  StdScreenWidget (base), Main/Pause/Settings/Credits/KeyBindingRow widgets, StdUITheme
  Dev/                 StdConsoleCommands (not in Shipping)
Source/StdCoreTests/   automation tests (editor only)
Content/Python/        stdcore_setup.py (rebuilds audio, input, maps, theme assets)
```

In the testbed only: `Scripts/stdcore_build_widgets.py` rebuilds the Widget Blueprints through the editor's MCP server.

### 4.3 Recipes

**A new asset setting** (for example a click sound):
1. In `StdProjectSettings.h`, add the default path to `StdCoreDefaults` and the `UPROPERTY` (`Config, EditAnywhere`). Set it in the constructor in `StdProjectSettings.cpp`.
2. Load it where it is used, with `StdSettings::LoadAsset(Settings->X, TEXT("X"), StdCoreDefaults::X)`.
3. Add it to the cook list in `StdCoreModule.cpp`, `StdCook::ModifyCook`.
4. Add it to `StdCore.Assets.SettingsDefaults` (`StdAssetTests.cpp`).
5. If the asset lives in the plugin, create it in `stdcore_setup.py` and add a row to the README's asset tables.

**A new volume channel** (for example Voice):
1. **C++ code:**
   - `EStdVolumeChannel` (new value);
   - `UStdUserSettings` (field, getter and setter with clamping);
   - `FStdClassVolumes` and `StdAudio::ComputeClassVolumes`;
   - the new `ApplyClass` line in `UStdSettingsSubsystem::ApplyVolumesInternal`;
   - `GetVolume` / `SetVolume`.
2. **Settings:** a `VoiceSoundClass` setting (see the previous recipe).
3. **Assets:** `SC_Voice` (parent `SC_Master`) and its adjuster in `SCM_Volumes`, both in `stdcore_setup.py` (`SOUND_CLASSES`, `CHILD_CLASSES`, mix).
4. **UI:** `Slider_VoiceVolume` and `Text_VoiceVolumeValue` as `BindWidgetOptional` in `UStdSettingsWidget`, plus the row in `WBP_Settings`.
5. **Tests:** `StdCore.Settings.Volumes` and `StdCore.Assets.Audio`.

**A new built-in input tag** (for example `InputTag.Sprint`):
1. `StdGameplayTags.h/.cpp`: declare and define it.
2. `stdcore_setup.py`: add the action to `INPUT_ACTIONS`, its rows to `MAPPINGS`, and the pair to `INPUT_TAGS`. Run the script.
3. `StdCore.Assets.Input`: update the expected actions, and the remappable list if the action is remappable.
4. Add the tag to the README's input tables.

Game-specific tags do not belong here (see 3.4).

**A new required widget on a screen:** avoid it (see 4.1). If it is unavoidable:
1. Add the widget to the screen's tree in `Scripts/stdcore_build_widgets.py`, and run it in the testbed.
2. Update the README's widget table.
3. Mention it in the changes, since every copied Widget Blueprint must add it.

**A new theme field:**
1. Add the `UPROPERTY` with a default in `UStdUITheme`'s constructor.
2. Use it in `Make*Style` or in `StdUITheme::ApplyToWidget` (rules by type and name).
3. Extend `StdCore.Theme.Styles`, and the README's *Restyle the menus* tables.

**A new screen in the plugin itself:**
1. Add a C++ subclass of `UStdScreenWidget`: override `GetDefaultFocusWidget`, bind buttons in `NativeOnInitialized`, and use `NativeOnScreenClosing` for saving.
2. Add a widget class setting, its Widget Blueprint (builder script), a test entry and the README rows.

### 4.4 Build, test and package loop

1. **Close every editor of this engine** (Live Coding blocks command-line builds).
2. **Build:**
   ```
   Build.bat <Project>Editor Win64 Development -Project=<path>.uproject -WaitMutex
   ```
   On the testbed machine, retry on C3859/C1076 errors, or add `-MaxParallelActions=6`.
3. **Assets**, if the change touches them:
   ```
   UnrealEditor-Cmd.exe <path>.uproject -run=pythonscript -script="<plugin>/Content/Python/stdcore_setup.py"
   ```
   It must end with 0 errors; a second run must report 0 created and 0 updated.
4. **Tests:**
   ```
   UnrealEditor-Cmd.exe <path>.uproject -ExecCmds="Automation RunTests StdCore;Quit" -unattended -nullrhi -nosplash -noxgeshadercompile -log
   ```
   All 7 (or more) must report `Result={Success}`.
5. **Manual check** in `-game` (menus, pause, Controls tab).
6. **Before a release:** package Development, check the log for `LogStdCore` warnings, then package Shipping (README, *Package the game*).
7. **Update the docs** that describe what changed (README tables, LIMITATIONS).

### 4.5 Bringing jam changes back to the template

Fixes made in the plugin during the jam belong in the template repository too:
1. Copy `Plugins/StdCore` back without `Binaries` and `Intermediate`, and review the diff: game-specific changes stay in the game.
2. Build, run the setup script (it reports any asset that drifted from the specification) and run the tests.
3. Commit per area, as usual.

---

## 5. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `LogStdCore: Warning: Std Core setting 'X' …` | A setting points to a missing asset. | Fix the setting, or restore the asset (setup script). |
| Play does nothing, log says the map does not exist | *Gameplay Map* is wrong, or the map is not cooked. | Check the setting; in a package, check the cook log for `Std Core: N packages added to the cook`. |
| Escape stops PIE instead of pausing | The editor's *Stop* shortcut. | Test in Standalone, or rebind *Play World > Stop*. |
| No pause in a gameplay map | The map does not use `StdGameMode` (or a subclass), or *Can Pause* is false. | Set the GameMode Override; check `bCanPause`. |
| Input tags never fire | The pawn's map uses another player controller, or the Input Config / Mapping Context settings are empty. | Use `StdPlayerController`; run `Std.DumpInput`; run the tests. |
| A remapped key does nothing in game | The game binds the Input Action directly with a key-specific check. | Bind by tag or action, never by key. |
| Menus unstyled or white | *Theme* empty or `DA_UITheme` missing. | Restore the asset or set the setting (fallback colors are dark, not white, in the plugin's WBPs). |
| Focus invisible on a new screen | The screen is not a `StdScreenWidget`, or *Highlight Focus* is off. | Reparent to `StdScreenWidget`. |
| Keyboard starts on nothing in a new screen | No default focus. | *Is Focusable* + *Desired Focus Widget* (3.1). |
| First start of a new project hangs | Shaders handed to an unlicensed IncrediBuild. | Start with `-noxgeshadercompile`. |
| Widget Blueprint does not compile after a plugin update | A new required widget name. | Add it (README widget table), or rebuild the plugin's WBPs with the builder script. |
