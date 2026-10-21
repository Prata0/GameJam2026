# Std Core

Genre-neutral infrastructure for a small Unreal Engine 5.8 game: main menu, pause menu, settings (volumes, window mode, quality, key remapping), map transitions with a fade, persistent music and tag-based input. No gameplay logic.

Everything lives in this folder: C++ in `Source/StdCore`, assets in `Content` (mount point `/StdCore/`). The only engine plugin it depends on is Enhanced Input.

## Contents

- [Install in a new project](#install-in-a-new-project)
- [Assets to create](#assets-to-create) (Phase 2 specification)
- [Daily use](#daily-use)
- [How it works](#how-it-works)
- [Console commands](#console-commands)

---

## Install in a new project

1. **The project must be a C++ project.** The plugin has its own C++ module and the host project must be able to compile it. In a Blueprint-only project, add any C++ class from the editor (Tools > New C++ Class) first.
2. **Copy the folder** `Plugins/StdCore` into the new project's `Plugins` folder (create it if needed). Do not copy `Binaries` or `Intermediate`.
3. **Enable the plugin** in the `.uproject` (the editor does it for you if you enable "Std Core" in Edit > Plugins):
   ```json
   { "Name": "StdCore", "Enabled": true }
   ```
4. **Regenerate project files and compile** (or open the `.uproject` and accept the rebuild).
5. **Project Settings**, the only steps the plugin cannot do by itself:
   - *Maps & Modes > Game Default Map* = `/StdCore/Maps/L_MainMenu`. This writes `GameDefaultMap` in `DefaultEngine.ini`.
   - *Maps & Modes > Default GameMode*: optional. The plugin maps set their own Game Mode in their World Settings. For the game's own maps either set `StdGameMode` (or a subclass) here or in each map's World Settings.
   - *Engine > Input > Default Input Component Class* must be `EnhancedInputComponent` and *Default Player Input Class* `EnhancedPlayerInput`. This is the default for projects created with UE5 templates; check it in old projects.
6. **Nothing else is needed in the `.ini` files:**
   - Volumes are saved by the plugin's own `StdUserSettings` object in `GameUserSettings.ini`; `GameUserSettingsClassName` is **not** changed.
   - Key remapping uses the Enhanced Input user settings, enabled by default in 5.8 (*Engine > Enhanced Input > Enable User Settings*). If a project turns them off, remapping stops and the log says so.
   - Packaging: `Plugins/StdCore/Config/Game.ini` adds `/StdCore` to *Directories to Always Cook*, merged into the project's config automatically. If a Shipping build ever misses plugin assets, add this to the project's `Config/DefaultGame.ini`:
     ```ini
     [/Script/UnrealEd.ProjectPackagingSettings]
     +DirectoriesToAlwaysCook=(Path="/StdCore")
     ```
7. **Check it works.** Play (Standalone or PIE) from `L_MainMenu`: the menu fades in, Play goes to `L_TestGameplay` with a fade, Escape pauses. Filter the Output Log on `LogStdCore`: every missing or misconfigured asset is reported there once, with the setting name and the expected path.

All plugin settings are in *Project Settings > Game > Std Core* (`UStdProjectSettings`, saved in `DefaultGame.ini`). Defaults point to the assets below; change them to use the game's own assets without touching the plugin.

---

## Assets to create

The C++ code expects these assets. Until they exist the plugin compiles, starts and degrades: missing assets are logged once and skipped. Create them **in this order**, so each one can reference the previous ones. All paths are inside the plugin content (`Plugins/StdCore/Content`).

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

Texts (button labels, window mode and quality names) are ordinary properties of the Blueprints; the C++ defaults are English.

### 4. Maps (`/StdCore/Maps/`)

| # | Asset | World Settings > GameMode Override | Content |
|---|---|---|---|
| 18 | `L_MainMenu` | `StdMenuGameMode` | Empty is fine (the menu is UI only). Optionally a camera, a light and a backdrop. |
| 19 | `L_TestGameplay` | `StdGameMode` | A floor (a scaled cube or plane with collision), a Directional Light (plus Sky Light if desired) and a Player Start. |

### 5. Project settings references

The defaults of *Project Settings > Game > Std Core* already point to all of the above. Nothing to set unless an asset gets a different name or path. In the testbed project, *Game Default Map* = `L_MainMenu`.

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

### Replace the widgets with restyled versions

Create the game's own Widget Blueprints (e.g. duplicate `WBP_MainMenu` into `/Game/UI/`), keep the parent class and the widget names, restyle freely, then point *Std Core > UI* to the new classes. The plugin's Blueprints stay untouched. The Controls tab's category header can be restyled by overriding **Create Category Header** in the settings Blueprint.

---

## How it works

- **Settings**: `UStdSettingsSubsystem` (game instance). Volumes are stored in `UStdUserSettings` (`[/Script/StdCore.StdUserSettings]` in `Saved/Config/<Platform>/GameUserSettings.ini`), applied with `SetSoundMixClassOverride` + `PushSoundMixModifier` on `SCM_Volumes`: `SC_Music` = Music x Master, `SC_SFX` = SFX x Master, `SC_UI` = Master, `SC_Master` = 1. The mix is pushed again on every new world. Sliders apply immediately; Back (or leaving the pause menu) saves. Window mode and quality use `UGameUserSettings`; at startup the saved scalability is loaded before the settings are read, and a mixed ("Custom") level is reset to Medium.
- **Transitions**: `UStdFlowSubsystem` draws a Slate overlay on the game viewport client, which survives map loads. Fade out > `OpenLevel` > new world > short delay > fade in, all on real time. Input is blocked by a Slate input pre-processor during the whole transition; travel and pause requests are refused. A map that does not arrive within *Travel Timeout Seconds* (or a travel failure) removes the fade, unblocks input and logs an error.
- **Input**: `UStdInputSubsystem` (local player) adds the configured mapping contexts and registers them with the Enhanced Input user settings when the player controller changes, so saved keys apply before the first input. Remapping covers keyboard and mouse entries only; gamepad keys, axes and *Reserved Keys* (Escape) are refused; a key already used by another entry swaps the two. Remapped keys are saved by Enhanced Input in `Saved/SaveGames/EnhancedInputUserSettings.sav`.
- **Pause**: `AStdPlayerController` pauses on `InputTag.Pause`. While the pause menu is open the input mode is UI only, so an input pre-processor handles the pause keys: Escape and gamepad B go back one screen (Settings saves) or resume from the pause menu itself; Start resumes from anywhere. Without a pause menu widget the game still pauses and Escape resumes.
- **Menus**: `UStdScreenWidget` handles focus (first button on open; taken back if it falls on the viewport), screen stacking (Settings and Credits open over their menu and return to it) and Back.

---

## Console commands

Development builds only:

| Command | Effect |
|---|---|
| `Std.Travel /Path/To/Map` | Travel with the fade, e.g. `Std.Travel /Engine/Maps/Entry`. |
| `Std.MainMenu` | Travel to the main menu map. |
| `Std.TogglePause` | Pause or resume through `AStdPlayerController`. |
| `Std.Volume master\|music\|sfx 0..1` | Set and save a volume. |
| `Std.DumpInput` | Log the remappable keys and the key of each plugin tag. |
