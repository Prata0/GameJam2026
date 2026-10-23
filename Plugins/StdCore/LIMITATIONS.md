# Std Core: known limitations and possible improvements

Things deliberately left out, or worth improving later. None of them is implemented.

## Input

- **Conflicts are checked only among remappable entries.** A fixed keyboard mapping added by the game (for example Tab bound with *Ignore Settings*) is not detected: assigning that key to a remappable action binds the key twice.
- **One key per action.** The Controls tab shows the first keyboard/mouse slot only; there is no secondary key column.
- **`On Input Tag` fires every frame for axes.** Blueprint listeners receive Triggered events for Move and Look every frame. There is no Blueprint node that binds to a single tag (for example an async action filtered on one tag).
- **Single local player.** Pause, menus and remapping assume one local player; split screen is untested.

## Transitions

- **The input block is application-wide.** The Slate input pre-processor that blocks input during a fade also blocks the editor while playing in PIE (about half a second per transition).
- **No animated loading screen.** During a blocking map load the last black frame stays on screen; there is no spinner or progress (MoviePlayer is not used).

## Menus and settings

- **Video settings are minimal.** No resolution selector, VSync or frame-rate limit. Switching to windowed mode uses two thirds of the desktop size.
- **Simple category headers.** The Controls tab's category header is a Text Block in the theme's heading font and accent color; for anything richer, override `Create Category Header` in the settings Blueprint.
- **Focus recovery is limited.** A screen takes the focus back only when it falls on the game viewport (a click on empty space).
- **Volumes are saved on Back.** Quitting with Alt+F4 while the Settings screen is open loses the volume changes made there (key changes are saved immediately).
- **Mouse and keyboard can highlight two widgets.** Moving the mouse onto a button moves the focus there. But if the keyboard then moves the focus away while the mouse stays still, the button under the mouse keeps Slate's own hover look next to the focused one, until the mouse moves.
- **The theme matches widgets by name.** Only `Border_Background`, `Border_Panel`, `Text_Title`, `Text_Heading*`, `Text_Version` and `Button_Tab*` get their special styles; a differently named widget gets the generic style of its type.
- **No menu animations or sounds.** Screens appear and disappear without transitions. The theme's hover and press sounds are empty, since the repository ships no audio.
- **Engine workaround.** `UInputKeySelector::SetButtonStyle` and `SetTextStyle` give Slate a pointer to their argument, which dangles once a temporary style goes out of scope, so the key selector's background disappears. `StdUITheme::SetKeySelectorStyle` sets the style twice, the second time with the copy the widget keeps. Check this again after an engine upgrade.
- **Escape stops PIE.** In Play In Editor, Escape is the editor's *Stop* shortcut, so the session ends instead of pausing. Test the pause in Standalone (or `-game`), or rebind *Editor Preferences > Keyboard Shortcuts > Play World > Stop* (for example to Shift+Escape). Packaged games are not affected.

## Audio

- **Music is one track at a time.** No playlists, per-map music or ducking.

## Testing and packaging

- **No automated tests.**
- **Verified so far:**
  - **Build and startup**
    - Clean compile.
    - Without assets: missing assets logged once, startup fade, travel, pause/resume, refused requests, volumes saved.
    - With assets: no missing-asset warnings.
  - **Setup script** runs twice with 0 changes on the second run.
  - **Menus and pause (keyboard, `-game`)**
    - Main menu and Credits.
    - Play travels to `L_TestGameplay` with the fade.
    - Escape pauses.
    - Settings from pause; Escape back saves; Escape again resumes.
    - Main menu from pause; Quit with the fade.
  - **Controls tab**
    - 6 entries in 2 categories.
    - Swap with a message.
    - Escape cancels a capture without closing the screen.
    - Reset.
    - Keys kept after a restart, in a stable order.
  - **PIE**: first map and travel.
  - **Console commands**: `Std.Music` and `Std.PlaySound` run without errors.
  - **Theme and layout (`-game`)**
    - Every screen at 1280×720; main menu and Settings also at 1920×1080.
    - The theme is visible in the designer.
    - Changing *Accent Color* restyles every menu after a restart.
    - Without a theme the menus stay readable.
  - **Focus and navigation**
    - The focus is visible on buttons, tabs, sliders and key selectors.
    - Left and Right change a volume and update its percentage.
    - The key list scrolls to the focused row; Down from the last row reaches *Reset to defaults*.
  - **Mouse**: hovering moves the highlight; clicking opens Settings and Credits and starts Play.
- **Not verified yet:**
  - Audio by ear: music volume, SFX silent while paused, UI sounds while paused.
  - Gamepad navigation.
  - Very wide or very tall windows (21:9, portrait).
- **Packaging:** `Config/Game.ini` adds `/StdCore` to *Directories to Always Cook*; whether the cooker picks it up from the plugin is to be confirmed with the first Shipping build (Phase 4).

## Assets

- **The setup script restores the specification.** Running `stdcore_setup.py` again reverts hand edits to the assets it manages. Game-specific changes belong in the game's own assets.
- **The setup script does not build the Widget Blueprints.** Python cannot edit a widget tree. In the testbed, `Scripts/stdcore_build_widgets.py` rebuilds them through the editor's MCP server; elsewhere, rebuild them by hand from the README tables. That script replaces each tree completely, so hand edits to the plugin's Widget Blueprints are lost when it runs.
- **Saving needs the game closed.** While a `-game` process of the same project is running, the editor can fail to save an asset that the game has loaded.

## Build environment (testbed machine)

- **Live Coding blocks every editor target of the same engine.** UBT checks Live Coding per executable, and every project uses the same `UnrealEditor.exe`: close all editors of this engine before building from the command line.
- **Random PCH memory errors (C3859 / C1076).** IncrediBuild without a license runs too many compiles in parallel; running the same build command again completes it.
