<div align="center">

# 🎛️ Reverse Riser

### Instant reverse-reverb risers for FL Studio

![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![Format](https://img.shields.io/badge/Format-VST3-7C5CFF?style=for-the-badge)
![DAW](https://img.shields.io/badge/FL%20Studio-Ready-FF8C00?style=for-the-badge)
![Price](https://img.shields.io/badge/Price-Free-2ECC71?style=for-the-badge)

**Capture or load any sound, get a reversed-reverb swell, drag it into your playlist.**

</div>

---

## ✨ Features

| | Feature |
|---|---|
| 🔁 | Reverse reverb rendered automatically |
| 🐍 | Sibilance reduction, so harsh "S" sounds don't echo backwards |
| 🎚️ | Tail, room size, damping, dry clip, low cut, high cut, drive, width, output |
| ▶️ | Preview playback inside the plugin |
| 🖱️ | Drag and drop the rendered WAV into your playlist |
| 📂 | Capture live audio or load a file (up to 20 seconds) |

---

## ⬇️ Download

1. Open the **Actions** tab of this repository.
2. Click the latest run with a tick ✅.
3. Scroll to **Artifacts** and download **ReverseRiser-Windows-VST3**.
4. Right-click the zip and choose **Extract All**.

> [!NOTE]
> You need to be logged in to GitHub to download artifacts.

---

## 💾 Install (Windows)

1. Open the extracted folder and copy the **Reverse Riser.vst3** folder.
2. Paste it into:

```
C:\Program Files\Common Files\VST3
```

> [!TIP]
> If the `VST3` folder doesn't exist, create it yourself.

---

## 🎹 Set up in FL Studio

1. Go to **Options > Manage plugins**.
2. Click the small folder icon with a **+** above the plugin search paths and add `C:\Program Files\Common Files\VST3`. Make sure it has a tick.
3. Click **Find installed plugins**.
4. Search for **Reverse** and check that it shows as **VST3**.

---

## 🚀 How to use

1. Press **F9** to open the Mixer and select the track your sound plays through.
2. Click an empty effect slot and choose **Reverse Riser**.
3. Press **Capture**, play your project, then press **Stop Capture**.
4. Press **Play** in the plugin to preview.
5. Drag the waveform box into the **Playlist**. 🎉

> [!IMPORTANT]
> Capture only records audio that passes through the mixer track the plugin is on.

---

## 🛠️ Troubleshooting

> [!WARNING]
> **Plugin not in the list?** Check that the folder is named exactly `Reverse Riser.vst3` and sits directly inside `C:\Program Files\Common Files\VST3`, then rescan.

> [!CAUTION]
> **No sound on preview?** FL Studio must be playing for the preview to be heard.

---

## 🧱 Build from source

```
cmake -S ReverseRiser -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ReverseRiser_VST3
```

Output: `build/ReverseRiser_artefacts/Release/VST3/`

<div align="center">



</div>
