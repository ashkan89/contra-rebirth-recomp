#!/usr/bin/env bash
# Legacy 32-bit RetroPie / VideoCore IV native GLES2 launcher.
set -euo pipefail
cd -- "$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")"
if [[ ! -x ./contrarebirth-gles2 ]]; then
    echo 'Missing executable: contrarebirth-gles2 (build with USE_GLES2=ON).' >&2
    exit 1
fi
if [[ ! -r /opt/vc/lib/libbrcmGLESv2.so || ! -r /opt/vc/lib/libbrcmEGL.so ]]; then
    echo 'This launcher requires the legacy Broadcom GLES/EGL libraries in /opt/vc/lib.' >&2
    exit 1
fi
unset DISPLAY XAUTHORITY LIBGL_ALWAYS_SOFTWARE GALLIUM_DRIVER LP_NUM_THREADS
unset MESA_GL_VERSION_OVERRIDE MESA_GLES_VERSION_OVERRIDE
export SDL_VIDEODRIVER=RPI
export SDL_VIDEO_GL_DRIVER=/opt/vc/lib/libbrcmGLESv2.so
export SDL_VIDEO_EGL_DRIVER=/opt/vc/lib/libbrcmEGL.so
# Resolution comes from the saved menu setting; default is the Wii's 640x528.
# Remove the earlier hidden scale override so menu labels match actual rendering.
unset EFB_SCALE
export WINDOW_SCALE=1 FULLSCREEN=1
export WIDESCREEN="${WIDESCREEN:-0}"
export VSYNC="${VSYNC:-0}" MSAA=1 ANISOTROPY=1
export SMOOTH_TEXTURES="${SMOOTH_TEXTURES:-0}" PERF=1
exec ./contrarebirth-gles2 "$@"
