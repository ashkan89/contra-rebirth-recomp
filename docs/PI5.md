# Raspberry Pi 5 ARM64

This target uses the native 64-bit ARM recompilation and the hardware GLES
renderer through Mesa V3D. It does not use the legacy Broadcom `/opt/vc`
libraries or the SDL RPI display driver from the Zero 2 W installation.

## Native build

On 64-bit Raspberry Pi OS, install CMake, Ninja, GCC, SDL2 and OpenGL/GLES
development files. The inspected Pi 5 has GCC 14 and SDL 2.32.4.

```sh
sudo apt-get install cmake ninja-build g++ libsdl2-dev libgl-dev libgles-dev
bash tools/build-pi5.sh
```

The script builds with Cortex-A76 tuning, four jobs by default, and installs
to `$HOME/contra-rebirth`. Override `BUILD_JOBS` or `INSTALL_DIR` if needed.
It requires sufficient memory for the generated translation units.

Add your own prepared NTSC-U game files in `$HOME/contra-rebirth/game`.
The supplied WAD's extracted files were copied separately to the tested Pi;
game data and saves are not part of a redistributable binary package.

## Run

```sh
cd ~/contra-rebirth
./contrarebirth --check-memory
./launch.sh --check-graphics
./launch.sh game
```

The launcher finds the logged-in user's Wayland display, with X11 as a
fallback. A graphical desktop session must be running. It does not alter
the desktop's output resolution, clocks or firmware configuration.

Installed settings use the original 640x528 internal framebuffer. Open the
settings menu with F1/Escape or Start+Back on a controller. Resolution changes
apply immediately; 320x280 and 480x320 are available and saved across launches.
Select Quit game to exit. Saves are in `nand` beside the executable.

## Inspected target — 2026-11-01

- Raspberry Pi 5 Model B Rev 1.1, 16 GB RAM, four Cortex-A76 cores up to 2.4 GHz.
- 64-bit Debian 13.7 (Raspberry Pi OS repositories), aarch64 userspace.
- Kernel 6.18.50+rpt-rpi-2712, Mesa 26.2.2, V3D 7.1.10.2 acceleration.
- Driver advertises desktop OpenGL 3.1 / GLSL 1.40 and OpenGL ES 3.1.
  The GLES renderer is selected to match this API support.
- Desktop: labwc/Wayland, LG TV currently at 1920x1080, 120 Hz.
- NVMe root filesystem, about 186 GB free at inspection.
- Approximately 37 C idle and 52 C during compilation; no throttling reported.
- All eight copied game-data files matched their local SHA-256 hashes.

## Build validation

The complete executable compiled on the Pi with GCC 14, C++17, `-O2`,
`-mcpu=cortex-a76`, `USE_SDL` and runtime `USE_GLES2`.
`file` identifies a native 64-bit AArch64 ELF executable using
`/lib/ld-linux-aarch64.so.1`. All `ldd` dependencies resolved.
The guest-memory test passed on the 64-bit host. The graphics check created
a hardware context reporting `V3D 7.1.10.2 / OpenGL ES 3.1 Mesa 26.2.2`.

Installed game: `/home/ashkan/contra-rebirth/contrarebirth`.
Launcher: `/home/ashkan/contra-rebirth/launch.sh`.
Rebuildable source: `/home/ashkan/contra-rebirth-build/source`.
The desktop application menu has a Contra ReBirth entry.
Executable SHA-256:
`220a1eba05a2e479012c4f4bbe8549568db63c29ec0f8dab7a58af5d1331f6c3`.

Game startup was verified visually on the actual display at 640x528, with the
Wii safety screen rendering at about 60 FPS. This is a simple startup screen,
not a Stage 1 performance benchmark or full gameplay validation. No controller
was connected during this check; SDL reported keyboard input available.

The GLES compatibility layer still has the documented limitations described
in [the renderer notes](RETROPIE.md). A complete playthrough is not validated.
