#!/usr/bin/env bash
# Genera el .tar.xz del toolchain recortado para Cortex-M3 (LPC1769) a partir de una
# instalacion local de MCUXpresso IDE, y lo deja listo para subir como GitHub Release.
#
#   bash tools/pack_toolchain.sh [ruta/a/mcuxpressoide]
#
# Por que existe este script:
#   El toolchain completo de MCUXpresso pesa 1.4 GB (arm-none-eabi-gdb solo son 173 MB,
#   por encima del limite de 100 MB por archivo de GitHub). Este script se queda con lo
#   estrictamente necesario para compilar y linkear Cortex-M3 -> ~170 MB, ~35 MB comprimido.
#
# Que se queda afuera y por que:
#   - arm-none-eabi-gdb  : 173 MB y ademas la build de NXP pide libncursesw.so.5 / libtinfo.so.5,
#                          que ya no existen en Ubuntu moderno. Usar `apt install gdb-multiarch`.
#   - f951 / libgfortran : Fortran, no lo usa nadie aca.
#   - multilibs no-M3    : el LPC1769 solo usa thumb/v7-m/nofp (armv7-m, sin FPU).
#   - libcr_*.a, redlib/ : librerias Redlib/CodeRed, propiedad de NXP (EULA de MCUXpresso, no
#                          redistribuibles). Se reemplazan por newlib / newlib-nano, que son
#                          equivalentes para lo que hace el curso.
#
# Lo que queda es la Arm GNU Toolchain 13.2.Rel1 tal cual la publica Arm (ver
# 13.2.Rel1-x86_64-arm-none-eabi-manifest.txt en la instalacion), bajo GPL/licencias libres.

set -euo pipefail

MCUX="${1:-}"
if [ -z "$MCUX" ]; then
  MCUX=$(ls -d /usr/local/mcuxpressoide-* /opt/mcuxpressoide-* 2>/dev/null | sort -V | tail -1 || true)
fi
T="$MCUX/ide/tools"
if [ ! -x "$T/bin/arm-none-eabi-gcc" ]; then
  echo "No encuentro el toolchain de MCUXpresso." >&2
  echo "Uso: bash tools/pack_toolchain.sh /usr/local/mcuxpressoide-11.10.0_3148" >&2
  exit 1
fi

GCCVER=$("$T/bin/arm-none-eabi-gcc" -dumpversion)                       # 13.2.1
MULTI=$("$T/bin/arm-none-eabi-gcc" -mcpu=cortex-m3 -mthumb -print-multi-directory)  # thumb/v7-m/nofp
OUT="$(cd "$(dirname "$0")/.." && pwd)/dist"
S="$OUT/stage"
TARBALL="$OUT/arm-none-eabi-cortex-m3-linux-x64.tar.xz"

echo "MCUXpresso : $MCUX"
echo "gcc        : $GCCVER"
echo "multilib   : $MULTI"
echo

rm -rf "$S"; mkdir -p "$S"/{bin,libexec/gcc/arm-none-eabi/$GCCVER,lib/gcc/arm-none-eabi/$GCCVER,arm-none-eabi/bin,arm-none-eabi/lib/$MULTI}

echo "[1/5] binarios de host (sin gdb, sin fortran)"
for f in gcc gcc-$GCCVER cpp g++ c++ gcc-ar gcc-nm gcc-ranlib gcov \
         as ld ld.bfd ar ranlib nm objcopy objdump size strip readelf addr2line c++filt strings elfedit; do
  cp -a "$T/bin/arm-none-eabi-$f" "$S/bin/"
done
cp -a "$T/arm-none-eabi/bin/." "$S/arm-none-eabi/bin/"   # as/ld que invoca gcc por dentro

echo "[2/5] internals de gcc (cc1, collect2, lto)"
for f in cc1 cc1plus collect2 liblto_plugin.so lto1 lto-wrapper; do
  cp -a "$T/libexec/gcc/arm-none-eabi/$GCCVER/$f" "$S/libexec/gcc/arm-none-eabi/$GCCVER/"
done

echo "[3/5] lib/gcc: headers + libgcc solo del multilib Cortex-M3"
G="$T/lib/gcc/arm-none-eabi/$GCCVER"; D="$S/lib/gcc/arm-none-eabi/$GCCVER"
cp -a "$G"/include "$G"/include-fixed "$G"/crt*.o "$G"/libgcc.a "$G"/libgcov.a "$D/"
mkdir -p "$D/$MULTI"; cp -a "$G/$MULTI/." "$D/$MULTI/"

echo "[4/5] sysroot: headers de newlib + ldscripts + specs + libs Cortex-M3"
cp -a "$T/arm-none-eabi/include/." "$S/arm-none-eabi/include/"
rm -f "$S"/arm-none-eabi/include/cr_*.h                  # headers propietarios de NXP
cp -a "$T/arm-none-eabi/lib/ldscripts" "$S/arm-none-eabi/lib/"
for f in nano.specs nosys.specs rdimon.specs rdpmon.specs redboot.specs; do
  cp -a "$T/arm-none-eabi/lib/$f" "$S/arm-none-eabi/lib/"   # gcc busca -specs= aca, no en el multilib
done
for f in "$T/arm-none-eabi/lib/$MULTI"/*; do
  case "$(basename "$f")" in
    libcr_*|redlib.specs|cpu-init|libgfortran*|libgloss-linux.a|linux*|aprofile*|iq80310*|pid.specs) continue ;;
  esac
  cp -a "$f" "$S/arm-none-eabi/lib/$MULTI/"
done

echo "[5/5] strip + comprimir"
find "$S/bin" "$S/libexec" "$S/arm-none-eabi/bin" -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true

# Prueba de humo antes de empaquetar: compilar y linkear de verdad para Cortex-M3.
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
printf 'int main(void){return 0;}\n' > "$TMP/t.c"
"$S/bin/arm-none-eabi-gcc" -mcpu=cortex-m3 -mthumb -specs=nano.specs -specs=nosys.specs \
    "$TMP/t.c" -o "$TMP/t.elf" 2>/dev/null   # los warnings de _write/_read son los stubs de nosys
"$S/bin/arm-none-eabi-objcopy" -O binary "$TMP/t.elf" "$TMP/t.bin"
echo "      prueba de humo OK ($("$S/bin/arm-none-eabi-size" "$TMP/t.elf" | tail -1 | awk '{print $4}') bytes)"

tar -cJf "$TARBALL" -C "$S" .
rm -rf "$S"
sha256sum "$TARBALL" | tee "$TARBALL.sha256"
echo
echo "Listo: $TARBALL  ($(du -h "$TARBALL" | cut -f1))"
echo
echo "Para publicarlo (necesita 'gh auth login'):"
echo "  gh release create toolchain-13.2.rel1 \"$TARBALL\" \\"
echo "     --title 'Toolchain arm-none-eabi 13.2.Rel1 (Cortex-M3)' \\"
echo "     --notes 'Arm GNU Toolchain 13.2.Rel1 recortado a Cortex-M3. Ver tools/install_toolchain.sh'"
