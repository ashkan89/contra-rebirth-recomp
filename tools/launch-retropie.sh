#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")"
if [[ ! "$(uname -m)" =~ ^(aarch64|armv7l|armv8l)$ ]]; then
    echo 'This package requires ARM Linux.' >&2
    exit 1
fi
export RESOLUTION_SCALE="${RESOLUTION_SCALE:-1}"
export WINDOW_SCALE="${WINDOW_SCALE:-1}"
export FULLSCREEN="${FULLSCREEN:-1}"
export MSAA="${MSAA:-1}"
export ANISOTROPY="${ANISOTROPY:-1}"
# Explicit opt-in: vc4 hardware does not support this renderer's OpenGL 3.3 API.
# A Mesa version override alone cannot supply the missing GPU functionality.
if [[ "${CONTRA_SOFTWARE_GL:-0}" == 1 ]]; then
    export LIBGL_ALWAYS_SOFTWARE=1
fi
if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo 'No desktop display found. The current renderer needs desktop OpenGL 3.3.' >&2
    echo 'For the experimental software test, launch from an X11 desktop/session.' >&2
    exit 1
fi
exec ./contrarebirth "$@"
