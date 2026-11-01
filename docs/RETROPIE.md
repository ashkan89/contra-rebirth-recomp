# Contra ReBirth on Raspberry Pi / RetroPie

The native ARM port now has a hardware GLES2 renderer. It was tested on a
Pi Zero 2 W running **32-bit Raspbian Buster / RetroPie**, with legacy Broadcom
graphics and a 480x320 screen. Stage 1 starts and accepts Xbox controller input.
Game logic runs as compiled ARM code, without a Wii CPU emulator.

**Performance remains experimental:** Stage 1 measures roughly 5–10 rendered
frames/second on this target. Hardware acceleration works, but this is not a
smooth or full-speed port. The VI timer and audio processing rates are separate
from rendered FPS; a 60 Hz VI counter does not mean 60 FPS gameplay.

## Installed target

The game and extracted user-supplied data are in `/home/pi/contra-rebirth`.
The Ports entry is `~/RetroPie/roms/ports/Contra ReBirth.sh` and invokes
`~/contra-rebirth/launch-hardware.sh`. Start directly from a terminal with:

```sh
cd ~/contra-rebirth
./launch-hardware.sh game
```

The launcher requires the legacy SDL RPI backend and
`/opt/vc/lib/libbrcmGLESv2.so` / `libbrcmEGL.so`. It renders directly through
VideoCore IV, without Xorg or Mesa llvmpipe. Other graphics stacks may require
a different SDL display backend. Stop another fullscreen frontend before
launching directly.

Default internal rendering is the Wii's **640x528**, with fullscreen output,
no MSAA or anisotropy, nearest texture filtering, and vsync off. In the settings
menu, **Resolution** also offers **320x280** and **480x320**, alongside the
existing Wii resolution multipliers. Changes resize the actual internal
framebuffer immediately and are saved for the next launch. Width and height
are scaled independently for the two new sizes; output still fits the display
using the selected aspect ratio. The old hidden `EFB_SCALE` override is removed.

Normal Linux scheduling is the default. The optional
`CPU_AFFINITY=1` experiment assigns host tasks to specific cores; it caused
audio underruns during testing and is not recommended on this image.
The guest CPU remains serialized for correctness. Graphics, audio and timer
work use separate host threads. Launchers do not alter clocks, voltage,
firmware, or the memory split.

Start+Back opens settings; select **Quit game** and confirm to exit. Saves
are under `nand/title/00010001/574b5445/data` beside the executable.

## Build on ARM Linux

Check `uname -m` and `getconf LONG_BIT`: a 64-bit-capable Pi may run a 32-bit OS.
This tested executable is ARM32 hard-float, not ARM64.

The native build requires CMake 3.16+, Ninja, a C++17 compiler, SDL2 development
files and GLES2 development files matching the target graphics stack.
On the tested legacy image, Broadcom libraries and headers live in `/opt/vc`.

```sh
bash tools/build-retropie.sh
```

The script defaults to `USE_GLES2=ON`, uses one compilation job to limit RAM
use, and writes `build/ContraReBirth-linux-<arch>-experimental`. Generated
translation units are large; the 512 MiB Pi may need swap. A larger ARM machine
or cross compiler with a matching target sysroot is preferable.
GCC 8's separate `stdc++fs` library is linked automatically.

No WAD or game data is included in the package. Supply the NTSC-U version
matching the generated code; `tools/prepare_wad.py` prepares a game directory.

```sh
./contrarebirth-gles2 --check-memory
./launch-hardware.sh --check-graphics
```

## Implementation and limits

GLES2 shaders respect VC4's eight attributes, eight varying vectors and eight
texture units. Packed vertices and 16-bit indices replace desktop layouts.
VAOs, sampler objects and framebuffer blits have GLES2 equivalents. Rendering
and presentation share one context; framebuffer copies stay on the GPU.

ARM32 maps MEM1, MEM2 and locked cache separately. ARM64 uses a virtual base
compatible with older 39-bit-address-space kernels. Audio mixing skips inactive
voices and accelerates constant-volume loops. FIFO storage is reduced to 4 MiB
on Pi; padded 32-bit cursors, fewer wakeups and ARM wait hints reduce overhead.
File operations use the target GNU C++ filesystem runtime; saves work.

Depth-texture EFB copies fail explicitly if requested. Non-COPY logic operations
and sampler LOD bias are unsupported. These limits did not prevent tested
Stage 1 startup; later stages and complete playthroughs remain unverified.

The older desktop OpenGL 3.3 executable and `launch-software.sh` remain a slow
fallback using Xorg fbdev / Mesa llvmpipe. The hardware launcher needs no Xorg.
See [target validation](RETROPIE_VALIDATION.md) for measurements and limits.
