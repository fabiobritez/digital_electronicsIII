#!/usr/bin/env bash
# Instala arm-none-eabi-gcc LOCALMENTE en tools/toolchain (no toca el sistema, no requiere sudo).
# Uso: bash tools/install_toolchain.sh
set -e
VER="13.3.1-1.1"
URL="https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v${VER}/xpack-arm-none-eabi-gcc-${VER}-linux-x64.tar.gz"
DIR="$(cd "$(dirname "$0")" && pwd)/toolchain"
mkdir -p "$DIR"
echo "Descargando arm-none-eabi-gcc ${VER}..."
curl -sL "$URL" -o /tmp/arm-toolchain.tar.gz
echo "Extrayendo en $DIR ..."
tar -xzf /tmp/arm-toolchain.tar.gz -C "$DIR" --strip-components=1
rm -f /tmp/arm-toolchain.tar.gz
"$DIR/bin/arm-none-eabi-gcc" --version | head -1
echo "Listo. Compilador en: $DIR/bin/arm-none-eabi-gcc"
