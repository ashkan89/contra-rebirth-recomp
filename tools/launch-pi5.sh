#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
    if [[ -S "$XDG_RUNTIME_DIR/wayland-0" ]]; then
        export WAYLAND_DISPLAY=wayland-0
    elif [[ -S /tmp/.X11-unix/X0 ]]; then
        export DISPLAY=:0
    fi
fi
export FULLSCREEN="${FULLSCREEN:-1}"
exec ./contrarebirth "$@"
