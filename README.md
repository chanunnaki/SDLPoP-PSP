# SDLPoP-PSP
Native port of **Prince of Persia** (SDLPoP) for the Sony PlayStation Portable (PSP-1000 / PSP-2000 / PSP-3000 / PSP-Go / PS Vita Adrenaline) and PPSSPP emulator, built using the modern `pspdev` toolchain.

---

## What's New in v1.1.0

- **Real-Time Rewind (Headline Feature)**:
  - **Hold L Shoulder** anytime to rewind time! Missed a jump, mistimed a parry, or fell on spikes? Hold **L** to roll back your mistake and try again.
  - **Variable Speed**: While holding L, tap or hold **Left** or **Right** on the D-Pad to speed up rewind from 1x up to 4x.
  - **Configurable**: Choose between **30 Seconds**, **60 Seconds**, or **Off** in **Settings $\to$ GAMEPLAY $\to$ Rewind**.
- **Tear-Free Screen Flashes**:
  - Sword pickup and healing potions now produce clean, authentic screen flashes with zero screen tearing or scanline glitches.
  - Fixed cutscene display glitches after Level 1.

---

## Features

- **Crystal-Clear Menus & Crisp HUD**:
  - Menus and text overlays are pixel-sharp, easy to read, and never blur or distort regardless of your chosen screen aspect ratio.
  - Health bars and status messages are scaled cleanly with zero flickering.
- **Widescreen & Classic Display Modes**:
  - Choose between **16:10** (Original PC), **16:9** (Full PSP Widescreen), or **4:3** (Classic CRT) from the pause menu.
- **Switch Graphics on the Fly (DOS / SNES)**:
  - Switch between original **DOS** graphics, enhanced **Super Nintendo** (SNES) visuals, or the hybrid **SNES Alt** pack directly from **Visuals $\to$ Graphics pack**—no restarts required.
- **In-Game Mod Manager**:
  - Play custom levelsets and full conversion mods right on your PSP! Drop mod folders into `mods/` and switch between them inside **Settings $\to$ MODS**.
- **Original Soundtrack & Audio**:
  - Full authentic 22-track musical score and sound effects, balanced cleanly for the PSP speakers and headphones.
- **Handheld-Tuned Controls**:
  - Responsive, natural control mapping built specifically for the PSP's D-pad, face buttons, and triggers.

---

## Controls

| PSP Button | Function in Game | Function in Menus |
|---|---|---|
| **D-Pad / Analog Stick** | Move Left / Right, Crouch Down, Jump Up | Navigate Options |
| **Cross ($\times$)** | Action / Shift (Grab ledge, strike sword, careful walk) | Confirm / Select / Cycle Setting |
| **Square ($\square$)** | Action / Shift (Grab ledge, strike sword, careful walk) | Confirm / Select |
| **Triangle ($\triangle$)** | Jump Up / Climb Ledge | - |
| **Circle ($\bigcirc$)** | Jump Up / Climb Ledge | Cancel / Back |
| **L Shoulder** | **Hold to Rewind Time** (or Shift if Rewind is OFF) | - |
| **R Shoulder** | Action / Shift (Grab ledge, strike sword, careful walk) | - |
| **Start** | Pause Game / In-Game Menu | Close / Return to Game |
| **Select** | Display Time Remaining | - |

---

## Installation & Playing on Real Hardware

1. Download or build `EBOOT.PBP`.
2. Connect your PSP to your computer via USB (or insert the Memory Stick).
3. Copy the `SDLPoP-PSP` folder to:
   ```
   ms0:/PSP/GAME/SDLPoP-PSP/
   ├── EBOOT.PBP
   ├── SDLPoP.ini
   ├── data/
   └── mods/
       ├── Princess of Persia/
       └── ...
   ```
4. Disconnect USB and launch **Prince of Persia** from **Game $\to$ Memory Stick** on your PSP XMB.

---

## Custom Mods Support

SDLPoP-PSP features full native support for custom levelsets and total conversion mods (available from [popot.org](https://www.popot.org/custom_levels.php)):

### Installing Mods
1. Create or open the `mods/` directory inside your game folder:
   ```
   ms0:/PSP/GAME/SDLPoP-PSP/mods/
   ```
2. Place any mod folder inside `mods/` (for example: `mods/Princess of Persia/`).
3. **Important for PSP / FAT32**: All `.DAT` files inside the mod directory must have uppercase names and extensions (e.g. `LEVELS.DAT`, `PRINCE.DAT`, `TITLE.DAT`).
4. Custom EXE modifications (`PRINCE.EXE`) and mod configs (`mod.ini`) inside the mod folder are fully supported and loaded automatically.

### Switching Mods In-Game
1. Press **Start** during gameplay to open the pause menu.
2. Navigate to **Settings $\to$ MODS**.
3. Highlight your desired mod (or **Original Game**) and press **Cross ($\times$)**.
4. Confirm the restart prompt. SDLPoP-PSP will immediately write the choice to `SDLPoP.ini`, reload all assets, and smoothly soft-reset directly into the mod's title and intro sequence!

---

## Building from Source

### Prerequisites
- [pspdev](https://github.com/pspdev/pspdev) toolchain installed (includes GCC, `psp-cmake`, `pack-pbp`, SDL2, SDL2_image).

### Build Command
Run the included build script:
```bash
./build.sh
```

This will:
1. Run `psp-cmake` targeting PRX mode (`BUILD_PRX=ON`) with CMake 3.10+.
2. Compile and link `prince.prx`.
3. Package the final `EBOOT.PBP` with PSP icon and title metadata.
4. Assemble a ready-to-use distribution bundle into `dist/SDLPoP/`.

---

## Automated Deployment

```bash
./deploy.sh
```

Automatically detects and deploys to:
1. **PPSSPP Emulator**: `~/.config/ppsspp/PSP/GAME/CAT_Homebrew/SDLPoP-PSP/` and `~/.config/ppsspp/PSP/GAME/SDLPoP-PSP/`.
2. **Physical PSP Hardware**: Direct USB mounts (`/Volumes/NO NAME/...`) and network automounts (e.g. via `ssh n`).

---

## In-Game Configuration

Configure settings in `SDLPoP.ini` or on-the-fly in the in-game **Settings $\to$ Visuals** menu:

```ini
; Levelset / Mod Selector:
; * original          = Play authentic Prince of Persia levels (default)
; * <Folder_Name>     = Load custom levelset from mods/<Folder_Name> (e.g. Princess of Persia)
levelset = original

; Graphics Pack Selector:
; * dos      = Classic DOS PC graphics (res_dos.pak) (default)
; * snes     = Super Nintendo 16-bit graphics (res_snes.pak)
; * snes_alt = Alternative SNES visual rework (res_snes_alt.pak)
graphics_pack = dos

; Display mode on PSP (480x272 screen):
; * 16:10 = Authentic Prince of Persia aspect ratio (436x272, 22px pillars). (default)
; * wide  = Full widescreen stretch across the entire screen (480x272).
; * 4:3   = Authentic DOS CRT pillarbox (362x272, 59px pillars).
psp_display_mode = 16:10

; HUD 2x integer split during gameplay (levels 1-14):
; Splits the screen into 256px playfield + 16px (2x integer) status bar.
enable_hud_split = true

; Decoupled 1:1 integer overlay:
; Renders pause/settings menu at 1:1 integer scale (320x200) over dimmed backdrop.
decouple_menu_overlay = true
```

---

## Original Documentation

For upstream PC/DOS documentation, modding specifications, CusPoP support, replays, and technical notes, see [README.upstream.md](README.upstream.md).

---

## Credits

- **Dávid Nagy** and contributors: [SDLPoP](https://github.com/NagyD/SDLPoP)
- **Jordan Mechner**: Creator of the original Prince of Persia
- **PSPDEV**: [pspdev](https://github.com/pspdev/pspdev) toolchain and SDK libraries
- **striga, sharkwouter**: Initial PSP scaffolding
- **chanunnaki**: Enhanced PSP port, rewind system, widescreen rendering, and in-game mod manager

### Graphics Packs & Custom Level Authors
- **KJ**: Creator of *SNES Remnants* (`0000149` / `0000296` on [popot.org](https://www.popot.org/custom_levels.php?mod=0000149)), source for the **SNES Graphics Pack** (`res_snes.pak`):
  - **brain**: Authentic SNES guard palettes and sprites, Prince sprites, and Palace environment tiles (*Total Pack 3.0*).
  - **PoP_MoreSNES**: Character sprites (Kid, Fat guard, Skeleton, Jaffar/Vizier, Shadow, Princess & Vizier).
  - **Total Pack 3.0**: *SNES Blue* dungeon environment graphics.
  - **tV2 & programmer**: SNES-style intro cinematic backdrop (*Revolution mod*).
- **Maurice Kaltofen (mk1995)**: Creator of *SNES-PC-set* (`0000042` on [popot.org](https://www.popot.org/custom_levels.php?mod=0000042)), source for the **SNES Alt Graphics Pack** (`res_snes_alt.pak`):
  - 256-color palette adaptation and sprite rework bringing DOS PC Prince of Persia assets closer to the SNES visual presentation.
