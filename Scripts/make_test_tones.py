"""Testbed only: synthesizes two test sounds and imports them into /Game/StdCoreTest.

S_TestMusic: an 8 s looping chord, for Std.Music and the music volume.
S_TestBeep: a 0.2 s beep, for Std.PlaySound (SFX and UI classes).

Both are generated here, so the repository holds no third-party audio. Idempotent: existing assets are left alone.

    UnrealEditor-Cmd.exe Test.uproject -run=pythonscript -script="<project>/Scripts/make_test_tones.py"
"""

import math
import os
import struct
import wave

import unreal

SAMPLE_RATE = 44100
DESTINATION = "/Game/StdCoreTest"
SOURCE_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "StdCoreTest")


def write_wav(path, duration, partials, fade):
    """Sum of sine partials (frequency, amplitude) with linear fades at both ends, 16-bit mono."""
    count = int(SAMPLE_RATE * duration)
    fade_count = int(SAMPLE_RATE * fade)
    frames = bytearray()
    for i in range(count):
        t = i / SAMPLE_RATE
        value = sum(amplitude * math.sin(2 * math.pi * frequency * t) for frequency, amplitude in partials)
        envelope = min(1.0, i / fade_count, (count - 1 - i) / fade_count) if fade_count else 1.0
        frames += struct.pack("<h", int(max(-1.0, min(1.0, value * envelope)) * 32767))
    with wave.open(path, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(bytes(frames))


def import_sound(name, path, looping):
    package_path = DESTINATION + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(package_path):
        unreal.log("[StdCore test tones] {}: unchanged".format(package_path))
        return

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sound = unreal.EditorAssetLibrary.load_asset(package_path)
    if not sound:
        unreal.log_error("[StdCore test tones] {}: import failed".format(package_path))
        return
    sound.set_editor_property("looping", looping)
    unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False)
    unreal.log("[StdCore test tones] {}: created".format(package_path))


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([DESTINATION], True)
    os.makedirs(SOURCE_DIR, exist_ok=True)

    music_path = os.path.join(SOURCE_DIR, "S_TestMusic.wav")
    # A minor triad, quiet; the fades keep the loop point click-free.
    write_wav(music_path, 8.0, [(220.0, 0.12), (261.63, 0.10), (329.63, 0.10)], fade=0.5)
    import_sound("S_TestMusic", music_path, looping=True)

    beep_path = os.path.join(SOURCE_DIR, "S_TestBeep.wav")
    write_wav(beep_path, 0.2, [(880.0, 0.3)], fade=0.02)
    import_sound("S_TestBeep", beep_path, looping=False)


main()
