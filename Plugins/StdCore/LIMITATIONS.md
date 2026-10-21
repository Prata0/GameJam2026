# Std Core: known limitations and possible improvements

Things deliberately left out of Phase 1 or worth improving later. None of them is implemented.

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
- **Plain category headers.** The Controls tab's category header is a default Text Block; restyle it by overriding `Create Category Header` in the settings Blueprint.
- **Focus recovery is limited.** A screen takes the focus back only when it falls on the game viewport (a click on empty space).
- **Volumes are saved on Back.** Quitting with Alt+F4 while the Settings screen is open loses the volume changes made there (key changes are saved immediately).

## Audio

- **Music is one track at a time.** No playlists, per-map music or ducking.

## Testing and packaging

- **No automated tests.**
- **Verified so far:** clean compile; the game runs without assets (missing assets logged once, startup fade, travel, pause/resume, refused requests, volumes saved).
- **Not verified yet (needs the Phase 2 assets):** menus, key remapping, music, Sound Mix, gamepad navigation.
- **Packaging:** `Config/Game.ini` adds `/StdCore` to *Directories to Always Cook*; whether the cooker picks it up from the plugin is to be confirmed with the first Shipping build (Phase 4).

## Build environment (testbed machine)

- **Live Coding blocks every editor target of the same engine.** UBT checks Live Coding per executable, and every project uses the same `UnrealEditor.exe`: close all editors of this engine before building from the command line.
- **Random PCH memory errors (C3859 / C1076).** IncrediBuild without a license runs too many compiles in parallel; running the same build command again completes it.
