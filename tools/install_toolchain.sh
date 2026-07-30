#!/usr/bin/env bash
# Deja el toolchain ARM listo en tools/toolchain (no toca el sistema, no requiere sudo).
#
#   bash tools/install_toolchain.sh              # baja el paquete del repo (~35 MB)
#   bash tools/install_toolchain.sh --mcuxpresso # enlaza el que ya trae MCUXpresso (0 descarga)
#   bash tools/install_toolchain.sh --xpack      # baja el toolchain xPack de upstream (~130 MB)
#   bash tools/install_toolchain.sh --force      # reinstala aunque ya exista
#
# Los tres caminos dejan lo mismo en tools/toolchain/bin/arm-none-eabi-*, que es lo que
# esperan los Makefile del curso y los .vscode/*.json.

set -euo pipefail

REPO="${TOOLCHAIN_REPO:-fabiobritez/digital_electronicsIII}"
TAG="${TOOLCHAIN_TAG:-toolchain-13.2.rel1}"
ASSET="arm-none-eabi-cortex-m3-linux-x64.tar.xz"
SHA256="8cfa8dd5d6bfaa3a019d626d0f4e9c05b7be7ffe721a27ee4c323821ad66376f"
XPACK_VER="13.3.1-1.1"

DIR="$(cd "$(dirname "$0")" && pwd)/toolchain"
MODE="release"; FORCE=0
for a in "$@"; do
  case "$a" in
    --mcuxpresso) MODE="mcuxpresso" ;;
    --xpack)      MODE="xpack" ;;
    --release)    MODE="release" ;;
    --force)      FORCE=1 ;;
    -h|--help)    sed -n '2,12p' "$0"; exit 0 ;;
    *) echo "Opcion desconocida: $a" >&2; exit 1 ;;
  esac
done

ok() { [ -x "$DIR/bin/arm-none-eabi-gcc" ]; }
if ok && [ "$FORCE" -eq 0 ]; then
  echo "Ya instalado: $("$DIR/bin/arm-none-eabi-gcc" --version | head -1)"
  echo "(usa --force para reinstalar)"
  exit 0
fi
rm -rf "$DIR"

find_mcuxpresso() {
  ls -d /usr/local/mcuxpressoide-* /opt/mcuxpressoide-* "$HOME"/mcuxpressoide-* 2>/dev/null | sort -V | tail -1
}

case "$MODE" in
  mcuxpresso)
    M=$(find_mcuxpresso || true)
    if [ -z "${M:-}" ] || [ ! -x "$M/ide/tools/bin/arm-none-eabi-gcc" ]; then
      echo "No encontre MCUXpresso IDE instalado. Proba sin --mcuxpresso." >&2
      exit 1
    fi
    echo "Enlazando el toolchain de $M (1.4 GB, no se copia nada)"
    ln -sfn "$M/ide/tools" "$DIR"
    ;;

  release)
    URL="${TOOLCHAIN_URL:-https://github.com/$REPO/releases/download/$TAG/$ASSET}"
    TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
    echo "Descargando $ASSET (~35 MB)..."
    if ! curl -fL --progress-bar "$URL" -o "$TMP/$ASSET"; then
      echo "No pude bajar el paquete del repo ($URL)." >&2
      echo "Reintentando con el toolchain xPack de upstream..." >&2
      exec "$0" --xpack --force
    fi
    echo "Verificando checksum..."
    echo "$SHA256  $TMP/$ASSET" | sha256sum -c - >/dev/null || { echo "SHA256 no coincide, abortando." >&2; exit 1; }
    mkdir -p "$DIR"
    tar -xJf "$TMP/$ASSET" -C "$DIR"
    ;;

  xpack)
    URL="https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v${XPACK_VER}/xpack-arm-none-eabi-gcc-${XPACK_VER}-linux-x64.tar.gz"
    TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
    echo "Descargando arm-none-eabi-gcc xPack ${XPACK_VER} (~130 MB)..."
    curl -fL --progress-bar "$URL" -o "$TMP/tc.tar.gz"
    mkdir -p "$DIR"
    tar -xzf "$TMP/tc.tar.gz" -C "$DIR" --strip-components=1
    ;;
esac

ok || { echo "Algo fallo: no hay $DIR/bin/arm-none-eabi-gcc" >&2; exit 1; }

# Verificacion real: compilar y linkear para Cortex-M3.
TMP2=$(mktemp -d); trap 'rm -rf "${TMP:-/nonexistent}" "$TMP2"' EXIT
printf 'int main(void){return 0;}\n' > "$TMP2/t.c"
"$DIR/bin/arm-none-eabi-gcc" -mcpu=cortex-m3 -mthumb -specs=nano.specs -specs=nosys.specs \
    "$TMP2/t.c" -o "$TMP2/t.elf" 2>/dev/null \
  || { echo "El toolchain no linkea para Cortex-M3." >&2; exit 1; }

echo
"$DIR/bin/arm-none-eabi-gcc" --version | head -1
echo "Listo. Compilador en: tools/toolchain/bin/arm-none-eabi-gcc"
if [ ! -x "$DIR/bin/arm-none-eabi-gdb" ]; then
  echo
  echo "Nota: este paquete no incluye gdb (pesa 173 MB y la build de NXP no corre en Ubuntu"
  echo "      moderno). Para depurar:  sudo apt install gdb-multiarch"
fi
