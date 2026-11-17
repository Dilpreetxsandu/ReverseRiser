# Reverse Riser (VST3)

An original reverse-reverb riser plugin: capture or load audio, and it renders a
reversed-reverb swell you can drag straight into FL Studio's playlist.

## Build without installing anything (Windows)
1. Create a free GitHub account and a new repository.
2. Upload every file in this folder (keep the folder structure, including `.github`).
3. Open the **Actions** tab, wait for "Build" to finish (about 10-15 min).
4. Download the `ReverseRiser-Windows-VST3` artifact and unzip it.
5. Copy the `Reverse Riser.vst3` folder to `C:\Program Files\Common Files\VST3`.
6. In FL Studio: Options > Manage plugins > Find plugins.

## Use
1. Put it on a mixer insert in FL Studio.
2. Press **Capture**, play the vocal or sound through that insert, press **Stop Capture**
   (or use **Load File** instead).
3. The riser renders automatically; use **Play** to preview.
4. Drag the waveform box into the playlist.
Knobs re-render automatically while "Auto re-render" is on.

## Local build
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
