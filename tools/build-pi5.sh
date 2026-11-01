#!/usr/bin/env bash
# Native Raspberry Pi 5 ARM64 build with Mesa V3D hardware GLES rendering.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$(uname -s)" != Linux || "$(uname -m)" != aarch64 ]]; then
    echo 'Run this on 64-bit ARM Linux (Pi 5).' >&2
    exit 1
fi
cmake -S "$root/src" -B "$root/build/pi5-native" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DUSE_SDL=ON -DUSE_GLES2=ON -DPI_ZERO2=OFF \
    '-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG -mcpu=cortex-a76'
cmake --build "$root/build/pi5-native" --parallel "${BUILD_JOBS:-4}"
package="${INSTALL_DIR:-$HOME/contra-rebirth}"
mkdir -p "$package"
install -m 755 "$root/build/pi5-native/contrarebirth" "$package/contrarebirth.new"
mv -f "$package/contrarebirth.new" "$package/contrarebirth"
install -m 755 "$root/tools/launch-pi5.sh" "$package/launch.sh"
cp "$root/LICENSE" "$root/THIRD_PARTY_NOTICES.md" "$root/src/runtime/ipl-font-licenses.txt" "$package/"
echo "Installed ARM64 game: $package/contrarebirth"
