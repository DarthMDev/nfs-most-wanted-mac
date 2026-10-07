# Getting started

The setup kit turns **your own copy** of Need for Speed: Most Wanted into a native Mac app. It checks your game files,
analyses and translates your `speed.exe`, builds the app, and prepares the optional extras from your own files.
It runs on your Mac only. Nothing is uploaded, and nothing it makes is meant to be shared.

## What you need

| | |
|---|---|
| **Mac** | Apple Silicon (M1 or later). Developed and tested on macOS 27 (M5 Pro); earlier macOS versions are untested. The app is built for the macOS version your Mac runs. About 10 GB free: the setup's tools and working files take about 4–5 GB, and the finished app is about the size of your game folder. |
| **The game** | Your own **Need for Speed: Most Wanted (2005)** for PC (Black Edition), as an **installed game folder** with its own working `speed.exe` (the folder that holds `speed.exe`, `GLOBAL`, `TRACKS`, `MOVIES` and so on). Copy it to the Mac from a Windows PC where it is installed, or from a backup. The setup checks first that the exe is one it can use, and stops without changing anything if it is not. |
| **Xcode Command Line Tools** | Apple's free developer tools. The Setup app offers to install them; in Terminal, `xcode-select --install`. |
| **Time** | About 10 minutes the first time on an M5 Pro, including the one-time analysis of `speed.exe`; allow longer on older Macs. Updates are quicker. |

The setup downloads the free developer tools it needs into its own folder (`~/Library/NFSMW-Native-Setup`):
**Ghidra** (the NSA's open-source disassembler, used once to map the functions in your `speed.exe`), a **Java
runtime** for it, and **Python 3.12** if your Mac has no Python 3.11 or later. It also builds Ghidra's decompiler for
Apple Silicon once, which uses Gradle's own folder (`~/.gradle`). Nothing is installed system-wide.

This project does not provide, link to, or explain how to obtain any game file. Use your own legally obtained copy.

## Set up with the Setup app (easiest)

1. Download the **NFS-Most-Wanted-Native-Setup** disk image (`.dmg`) from the
   [latest release](https://github.com/elforeign/nfs-most-wanted-mac/releases/latest) and open it.
2. Open **NFS Most Wanted Native Setup**. The app is not notarized by Apple, so the first time macOS blocks it: click
   **Done**, then go to **System Settings → Privacy & Security** and click **Open Anyway** next to its name.
3. **Choose your game folder.** The app checks your `speed.exe` straight away and changes nothing if it can't be used.
4. If asked, **install Apple's Command Line Tools**; the app waits for them.
5. Optional: add your copy of `XenonEffects.tpk`, or choose where the game goes (Applications by default).
6. Click **Build**. You see each step, the progress and the time; the Mac stays awake while it runs. When it finishes,
   click **Open the game**.

The app keeps its working files in `~/Library/NFSMW-Native-Setup`, so building again (for example with a newer
Setup app) reuses the one-time analysis and is much quicker.

## Set up in Terminal

```sh
git clone https://github.com/elforeign/nfs-most-wanted-mac.git
cd nfs-most-wanted-mac
./setup.sh "/path/to/Need for Speed Most Wanted"
```

The setup prints each step and what it found:

1. **Check:** the game folder and its `speed.exe`.
2. **Link:** your game folder is linked (not copied or changed) into `original/retail`.
3. **Analyse:** Ghidra maps the functions in your `speed.exe` (once; later runs reuse it).
4. **Translate and build:** the game code is translated to C and compiled with the native host.
5. **Extras from your game:** the light pools are built from your game's own lamp data and effects.
6. **Optional packs:** a TexWizard texture pack in your game folder (`TRACKS/TexWizardX360`, for example the Xbox 360
   Stuff Pack's textures) and `XenonEffects.tpk` (in its `scripts` folder) are found and added. How to add them on
   your Mac: the [companion guide](COMPANION_GUIDE.md#optional-mods).
7. **Package:** `dist/NFS Most Wanted Native.app` is written, with your game data inside it.

Move the app to Applications if you like and open it. The first launch takes a little longer while Metal prepares
its shaders.

### Options

| Option | |
|---|---|
| `--xenon-effects FILE` | Use this `XenonEffects.tpk` for the sparks and light trails (otherwise the game folder's `scripts/XenonEffects.tpk` is used if it's there). |
| `--no-pools` | Skip building the light pools. |
| `--out DIR` | Write the app somewhere other than `dist/`. |
| `--check-only` | Only check the game folder and its `speed.exe`, then stop. |

Run `./setup.sh --help` for the full list.

## Updating

With the Setup app: download the newer Setup app, open it, choose your game folder and click **Build**.

In Terminal:

```sh
git pull
./setup.sh "/path/to/Need for Speed Most Wanted"
```

The analysis is reused, so an update only translates, builds and packages again. Your saves and settings are kept:
they live in your profile folder, not in the app (see the [companion guide](COMPANION_GUIDE.md#saves-and-settings)).

## Removing it

Delete the game app, the Setup app (or the cloned folder), `~/Library/NFSMW-Native-Setup` and `~/.gradle` (used once
to build Ghidra's decompiler). Your saves and settings are in `~/Library/Application Support/NFS Most Wanted Native/`;
delete that too if you want them gone.
