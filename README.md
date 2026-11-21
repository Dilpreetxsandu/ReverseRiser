# Reverse Riser

A free reverse-reverb riser plugin (VST3) for Windows. Capture or load any sound, and it builds a reversed-reverb swell that you can drag straight into your DAW's timeline. Works in FL Studio and any other DAW that loads VST3 plugins.

## Features
- Reverse reverb rendered automatically
- Sibilance reduction, so harsh "S" sounds don't echo backwards
- Controls: tail length, room size, damping, dry clip level, low cut, high cut, drive, stereo width, output level
- Preview playback inside the plugin
- Drag and drop the rendered WAV into your playlist
- Load a file or capture live audio (up to 20 seconds)

## Download
1. Go to the **Actions** tab of this repository.
2. Click the latest run with a tick.
3. Scroll to **Artifacts** at the bottom and download **ReverseRiser-Windows-VST3** (you need to be logged in to GitHub).
4. Right-click the zip and choose **Extract All**.

## Install (Windows)
1. Open the extracted folder and copy the **Reverse Riser.vst3** folder.
2. Paste it into `C:\Program Files\Common Files\VST3`. If the `VST3` folder doesn't exist, create it.

## Set up in FL Studio
1. Open FL Studio and go to **Options > Manage plugins**.
2. Click the small folder icon with a **+** above the plugin search paths list and add `C:\Program Files\Common Files\VST3`. Make sure it has a tick.
3. Click **Find installed plugins**.
4. Search for **Reverse** to confirm it shows up with the format VST3.

## How to use
1. Press **F9** to open the Mixer and select the track your sound plays through.
2. Click an empty effect slot and choose **Reverse Riser**.
3. Press **Capture**, play your project so the sound goes through that track, then press **Stop Capture**. The riser renders automatically.
4. Press **Play** in the plugin to preview it (FL Studio must be playing).
5. Drag the waveform box into the **Playlist**.

You can also press **Load File** to use a WAV, MP3, FLAC or OGG file instead of capturing.

## Troubleshooting
- **Plugin not in the list:** check that the folder is named exactly `Reverse Riser.vst3` and sits directly inside `C:\Program Files\Common Files\VST3`, then rescan.
- **No sound on preview:** FL Studio must be playing for the preview to be heard.
- **Capture is empty:** make sure your audio is actually routed through the mixer track that has the plugin on it.

## Build from source
Requires CMake and a C++ compiler:

```
cmake -S ReverseRiser -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ReverseRiser_VST3
```

The output is in `build/ReverseRiser_artefacts/Release/VST3/`. Built with [JUCE](https://juce.com).
