#!/usr/bin/env bash
# Starts a temporary framebuffer X server when launched from RetroPie's console.
# The game runs as the invoking user; Xorg alone needs root on legacy fbdev.
set -euo pipefail
cd -- "$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")"
for tool in xinit xauth mcookie glxinfo; do
    command -v "$tool" >/dev/null || { echo "Missing display tool: $tool" >&2; exit 1; }
done
if [[ ! -x /usr/lib/xorg/Xorg ]]; then
    echo 'Missing Xorg server (/usr/lib/xorg/Xorg).' >&2
    exit 1
fi
auth="$PWD/.Xauthority"
display="${DISPLAY:-:0}"
game_env=(
    "DISPLAY=$display" "XAUTHORITY=${XAUTHORITY:-$auth}"
    CONTRA_SOFTWARE_GL=1 GALLIUM_DRIVER=llvmpipe
    "LP_NUM_THREADS=${LP_NUM_THREADS:-2}"
    SDL_VIDEODRIVER=x11 SDL_VIDEO_X11_FORCE_EGL=0
    "RESOLUTION_SCALE=${RESOLUTION_SCALE:-1}" "WINDOW_SCALE=${WINDOW_SCALE:-1}"
    "FULLSCREEN=${FULLSCREEN:-1}" "WIDESCREEN=${WIDESCREEN:-0}"
    "VSYNC=${VSYNC:-0}" "MSAA=${MSAA:-1}" "ANISOTROPY=${ANISOTROPY:-1}"
    "SMOOTH_TEXTURES=${SMOOTH_TEXTURES:-0}" PERF=1
)
if env "${game_env[@]}" glxinfo -B >/dev/null 2>&1; then
    exec env "${game_env[@]}" "$PWD/launch.sh" "$@"
fi
if [[ -S /tmp/.X11-unix/X0 ]]; then
    echo 'Display :0 is already occupied and could not be accessed.' >&2
    exit 1
fi
touch "$auth"
chmod 600 "$auth"
xauth -f "$auth" add :0 . "$(mcookie)"
game_env[0]=DISPLAY=:0
game_env[1]="XAUTHORITY=$auth"
# Root xinit owns the X server lifecycle. Only its client is dropped back to
# the invoking user, so saves and configuration stay owned by that user.
exec sudo -n /usr/bin/env "XAUTHORITY=$auth" xinit \
    /usr/bin/sudo -n -u "$(id -un)" /usr/bin/env \
    "${game_env[@]}" "$PWD/launch.sh" "$@" -- /usr/lib/xorg/Xorg :0 \
    -config "$PWD/xorg-fbdev.conf" -auth "$auth" -logfile "$PWD/Xorg.log" \
    -nolisten tcp -nocursor vt1
