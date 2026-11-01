# Pi Zero 2 W validation — 2026-11-01

## Target and build

- Pi Zero 2 W Rev 1.0, four Cortex-A53 cores, 32-bit Raspbian Buster.
- Linux 5.10.103-v7+, armv7l, GCC 8.3, CMake 3.16.3, SDL 2.0.10+5rpi.
- ARM/GPU split 256/256 MiB; about 237 MiB Linux-usable RAM; 2 GiB swap.
- Legacy Broadcom display stack, no `/dev/dri`, 480x320 RGB565 framebuffer.
- Existing firmware settings: arm_freq=1350, core/GPU=500, over_voltage=6,
  force_turbo=1. This work did not change boot settings.
- Last gameplay check: 49.4 C, `get_throttled=0x0`, game RSS about 105 MiB.

The initially requested ARM64 target did not match the installed OS. The
deployed game is native ARM32 hard-float, appropriate for this installation.

The executable was cross-compiled on Windows using Zig 0.13.0 / LLVM,
ARM Linux glibc 2.28 target, Cortex-A53, C++17, `-O2`,
`-fno-strict-aliasing`, and `USE_SDL`, `USE_GLES2`, `PI_ZERO2`.
The mixer uses `-O3`. It links the Pi's SDL2, Broadcom GLES2, atomic,
GNU libstdc++ and libgcc dynamically, plus its GCC 8 libstdc++fs archive.
The GNU filesystem runtime fixed save failures with the earlier libc++ build.
Target `ldd` checks resolved dependencies.

## Data and correctness checks

The user-supplied NTSC-U WAD was extracted with libWiiPy. All five decrypted
content sizes and SHA-1 hashes matched the TMD. All eight prepared game files
matched SHA-256 after transfer to `/home/pi/contra-rebirth/game`.

- Title: `00010001574B5445` (WKTE).
- `main.dol`: 2410528 bytes.
- DOL SHA-1: `825fb4ec04335500ca9e172d05b2e593c5ab7dc9`.

The package contains no WAD, extracted game data, or saves.

- Guest memory check passes: boundaries, endian and unaligned accesses,
  independent MEM1/MEM2/locked-cache storage and uncached RAM mirrors.
- GLES context reports **VideoCore IV HW / OpenGL ES 2.0**. Measured limits:
  eight attributes, eight varying vectors, eight texture units,
  136 fragment uniform vectors. VAOs, depth textures and uint indices are absent.
- Physical DispmanX screen captures show title, selection and Stage 1.
  User confirmed Xbox A selects Bill and Stage 1 starts, and controller input
  remained responsive during an SSH interruption.
- NAND saves created: `banner.bin` (61600 bytes), `bak000` (128 bytes).
- Audio optimization matched reference output and guest voice state in 36
  native-GCC cases: ADPCM, PCM8/16, ratios, ramps, LPF and inactive voices.
- Native-GCC FIFO test passed 300000 ordered BP commands plus variable-sized
  NOPs, crossing 32-bit cursor wrap and exercising repeated wakeups. It passed
  with normal scheduling and optional core affinity.
- Deployed executable SHA-256:
  `602bda77449da976d2f77b657e266c1421a7f4610b17f280854bfe3c4fddb265`.

Resolution settings round-trip tests passed on the Pi with native GCC: 640x528,
320x280 and 480x320 each preserved their selected mode after save/reload.
The existing 2x Wii mode still maps to 1280x1056.
Live keyboard navigation through the actual settings menu verified resize
events in this order: 480x320, 320x280, 480x320, 640x528. Physical display
captures were taken at each size. The menu was closed and the game left
running at 640x528, with `resolution_scale=1` and `render_resolution=0` saved.

## Performance and limits

Initial desktop GL rendering used Mesa llvmpipe at only 2–5 FPS.
The GLES2 backend uses the GPU. Observed rendering ranges from about 59 FPS
on simple screens to roughly 5–10 FPS in Stage 1. Earlier half-size title
measurements were about 25–28 FPS. These are live observations across different
scenes, not a controlled benchmark or guaranteed averages. Those measurements
used the earlier half-size EFB configuration. The current default is restored
to 640x528; the menu adds exact 320x280 and 480x320 internal render sizes.
No equivalent performance claim is made for the restored resolution.

Latest Stage 1 VI remained about 59.9/s and audio processing about 333/s.
Recent intervals showed zero emulator-ring underruns, dropped frames and
skipped AI periods. Fixed core affinity caused ALSA underruns at the title
screen; affinity was removed from the running process and remains disabled
by default. Counters alone do not establish subjective audio quality or
full-speed gameplay.

A live CPU sample reduced FIFO writer share from about 6.4% to 3.2% across
different Stage 1 captures. This supports reduced FIFO overhead, not a
controlled total FPS improvement. Remaining CPU work spans guest functions,
vertex decoding, graphics submission and audio. Shader specialization has
not demonstrated a total FPS improvement in a controlled comparison.

The port is **not smooth or full-speed** on this Zero 2 W. No claim is made
that clock changes or more host threads would achieve 60 FPS. Guest threads
share one emulated CPU state and must remain serialized. Full playthroughs,
later effects and complete graphics accuracy remain untested. Depth-format
EFB copies fail explicitly; non-COPY logic operations and LOD bias are
unsupported by the GLES2 compatibility layer.

## Installed files

- Game: `/home/pi/contra-rebirth/contrarebirth-gles2`.
- Launcher: `/home/pi/contra-rebirth/launch-hardware.sh`.
- Ports: `/home/pi/RetroPie/roms/ports/Contra ReBirth.sh`.
- Resolution validation log: `/home/pi/contra-rebirth/resolution-menu-test.log`.

The software renderer and previous GPU executable remain as fallbacks.
An Xorg fbdev session was tested earlier; the native RPI/GLES2 launcher does
not use it. The display install added 13 packages, without upgrades/removals.
