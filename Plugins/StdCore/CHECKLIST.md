# Std Core: checklist for a new project

One page for the day the plugin is copied into the game. Details are in the [README](README.md).

## 1. Install (about 10 minutes plus the first compile)

- [ ] The project is a C++ project (else: *Tools > New C++ Class* once).
- [ ] Copy `Plugins/StdCore` without `Binaries` and `Intermediate`.
- [ ] Add `{ "Name": "StdCore", "Enabled": true }` to the `.uproject` plugins.
- [ ] Compile: open the `.uproject` and accept the rebuild, or run `Build.bat <Project>Editor Win64 Development -Project=<path>`.
- [ ] First start: shaders compile for a few minutes. If it hangs with no progress (IncrediBuild without a license, see LIMITATIONS), start with `-noxgeshadercompile`.
- [ ] *Project Settings > Maps & Modes > Game Default Map* = `/StdCore/Maps/L_MainMenu`.
- [ ] *Engine > Input*: *Default Player Input Class* `EnhancedPlayerInput`, *Default Input Component Class* `EnhancedInputComponent` (already set in UE5 templates).

## 2. Automated checks (2 minutes)

- [ ] Run the tests: *Tools > Session Frontend > Automation*, filter `StdCore`, all green. Or from the command line:
  ```
  UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests StdCore;Quit" -unattended -nullrhi -nosplash -log
  ```
- [ ] Output Log filtered on `LogStdCore`: no warnings when playing from `L_MainMenu`.

## 3. Manual checks the tests do not cover (10 minutes, Standalone or `-game`)

**Menus**
- [ ] Keyboard only: arrows and Enter reach every button, the focus is always visible, and Escape goes back.
- [ ] Mouse only: hover highlights, and clicks work on every screen.
- [ ] Gamepad (if the game supports it): left stick / D-pad navigate, A confirms, B goes back, Start pauses and resumes.

**Audio** (by ear; play a track with `Std.Music <path>` and a sound with `Std.PlaySound <path> [ui]`)
- [ ] Master at 0 silences everything.
- [ ] Music at 0 silences only the music.
- [ ] Effects at 0 silences only the effects.
- [ ] While paused, the music and the UI sounds go on, and the game effects stop.

**Controls tab**
- [ ] Remap a key, see the swap message, restart the game, and the key is still remapped.
- [ ] *Reset to defaults* restores everything.

**Settings**
- [ ] Window mode cycles through fullscreen, borderless and window.
- [ ] Quality cycles through its levels.
- [ ] Both survive a restart.

**Layout**
- [ ] Main menu, pause, settings and credits are readable at 1280×720 and at the native resolution, and nothing is cut.

## 4. Make it the game's (as needed)

- [ ] *Project Settings > Std Core > Gameplay Map* → the game's first map.
- [ ] Restyle: edit `DA_UITheme` (accent color, fonts), or point *Theme* to a copy in `/Game`.
- [ ] Texts: game title in `WBP_MainMenu`'s `Text_Title`, and the credits in `WBP_Credits`.
- [ ] Project version in *Project Settings > Description*: it is shown on the main menu.
- [ ] The game's own input: actions, tags, Input Config and Mapping Context (README, *Daily use*).

## 5. Before submitting

- [ ] Package Shipping (*Platforms > Windows > Package Project*, Shipping).
- [ ] Run the packaged `.exe` on a clean start: menu, Play, pause, settings, remap and quit.
- [ ] Delete `%LOCALAPPDATA%\<Project>\Saved` once to test a first-run experience.
