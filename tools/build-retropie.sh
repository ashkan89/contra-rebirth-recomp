#!/usr/bin/env bash
# Native ARM build; GLES2 uses the Pi Zero 2 W GPU on legacy RetroPie.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$(uname -s)" != Linux || ! "$(uname -m)" =~ ^(aarch64|armv7l|armv8l)$ ]]; then
    echo 'Requires ARM Linux. Run this on the Pi or another compatible ARM machine.' >&2
    exit 1
fi
for tool in cmake ninja c++; do
    command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 1; }
done
use_gles2="${USE_GLES2:-ON}"
echo "Building experimental ARM port; GLES2=$use_gles2. Gameplay speed is not guaranteed."
cmake -S "$root/src" -B "$root/src/build-retropie" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DUSE_SDL=ON -DPI_ZERO2=ON -DUSE_GLES2="$use_gles2"
# The generated translation units are large. More jobs can exhaust the Pi's RAM.
cmake --build "$root/src/build-retropie" --parallel "${BUILD_JOBS:-1}"
arch=armhf
if [[ "$(getconf LONG_BIT)" == 64 ]]; then arch=arm64; fi
package="$root/build/ContraReBirth-linux-$arch-experimental"
mkdir -p "$package"
if [[ "$use_gles2" == ON ]]; then
    cp "$root/src/build-retropie/contrarebirth" "$package/contrarebirth-gles2"
else
    cp "$root/src/build-retropie/contrarebirth" "$package/contrarebirth"
fi
cp "$root/tools/launch-retropie.sh" "$package/launch.sh"
cp "$root/tools/launch-retropie-software.sh" "$package/launch-software.sh"
cp "$root/tools/launch-retropie-hardware.sh" "$package/launch-hardware.sh"
cp "$root/dist/retropie/xorg-fbdev.conf" "$package/"
cp "$root/dist/retropie/Contra ReBirth.sh" "$package/"
cp "$root/LICENSE" "$root/THIRD_PARTY_NOTICES.md" "$root/src/runtime/ipl-font-licenses.txt" "$package/"
cp "$root/docs/RETROPIE.md" "$package/README.md"
if [[ -f "$root/docs/RETROPIE_VALIDATION.md" ]]; then
    cp "$root/docs/RETROPIE_VALIDATION.md" "$package/"
fi
chmod +x "$package/launch.sh" "$package/launch-software.sh" "$package/launch-hardware.sh"
if [[ "$use_gles2" == ON ]]; then chmod +x "$package/contrarebirth-gles2"; else chmod +x "$package/contrarebirth"; fi
chmod +x "$package/Contra ReBirth.sh"
echo "Package: $package"
