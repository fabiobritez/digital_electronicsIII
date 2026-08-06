# tools/

Scripts de infraestructura del repo. No son material de estudio: el contenido del curso está en
[`curso/`](../curso/).

## Toolchain ARM

### `install_toolchain.sh`

Deja `arm-none-eabi-gcc` y compañía en `tools/toolchain/` (ignorado por git). No toca el sistema,
no pide `sudo`, y es lo que esperan los Makefile del curso y los `.vscode/*.json`.

```bash
bash tools/install_toolchain.sh              # usa el paquete versionado en el repo (sin red)
bash tools/install_toolchain.sh --mcuxpresso # enlaza el que ya trae MCUXpresso (0 descarga)
bash tools/install_toolchain.sh --xpack      # baja el toolchain xPack de upstream (~130 MB)
bash tools/install_toolchain.sh --force      # reinstala aunque ya exista
```

Los tres modos terminan igual: `tools/toolchain/bin/arm-none-eabi-gcc`. El script verifica el
SHA256 del paquete y, antes de dar por buena la instalación, **compila y linkea de verdad** un
programa para Cortex-M3.

| Modo | Cuándo usarlo |
|------|---------------|
| por defecto | cualquier máquina Linux x86-64. Descomprime `toolchain-pkg/` (ver abajo): **no necesita internet** |
| `--mcuxpresso` | ya tenés MCUXpresso instalado. Crea un symlink a su `ide/tools`, sin copiar 1.4 GB |
| `--xpack` | otra arquitectura, o necesitás `gdb` incluido en el mismo paquete |

Si el paquete local no está, el modo por defecto cae a descargarlo de un GitHub Release y, si eso
también falla, a xPack. Variables de entorno: `TOOLCHAIN_URL` (fuerza una URL o un archivo local en
vez del paquete del repo), `TOOLCHAIN_REPO` y `TOOLCHAIN_TAG`.

### `toolchain-pkg/`

El paquete en sí: `arm-none-eabi-cortex-m3-linux-x64.tar.xz` (34.5 MB) y su `.sha256`. Está
**versionado en el repo** a propósito, para que `git clone` más `install_toolchain.sh` alcance para
empezar a compilar sin depender de que ninguna URL siga viva. Lo genera `pack_toolchain.sh`.

**El paquete no incluye `gdb`.** Son 173 MB, y además la build que trae MCUXpresso está linkeada
contra `libncursesw.so.5` y `libtinfo.so.5`, que ya no existen en Ubuntu moderno. Para depurar:
`sudo apt install gdb-multiarch`, o usá `--xpack`.

### `pack_toolchain.sh`

Regenera el `.tar.xz` que publica el release, a partir de una instalación local de MCUXpresso.

```bash
bash tools/pack_toolchain.sh                                    # autodetecta
bash tools/pack_toolchain.sh /usr/local/mcuxpressoide-11.10.0_3148
```

Sobrescribe `toolchain-pkg/` con el tarball nuevo y su `.sha256`. Como ese directorio está
versionado, después hay que commitear el resultado.

Recorta 1.4 GB a unos 170 MB (35 MB comprimido) quedándose solo con:

- los binarios de host sin `gdb` ni Fortran, pasados por `strip`
- `cc1` y `cc1plus`, sin `f951`
- **un solo multilib**: `thumb/v7-m/nofp`, el del Cortex-M3, de los 39 que trae
- los headers de newlib, los `ldscripts` y los `.specs`

Y deja afuera, además del tamaño, todo lo que es propiedad de NXP y no se puede redistribuir:
`libcr_*.a` (Redlib), `redlib.specs`, los headers `cr_*.h`. En su lugar se usa newlib-nano, que es
equivalente para lo que hace el curso. Lo que queda es la Arm GNU Toolchain 13.2.Rel1 tal cual la
publica Arm, bajo licencias libres.

> Si regenerás el paquete, el SHA256 cambia (el `.tar.xz` no es reproducible byte a byte: guarda
> las fechas de los archivos). Hay que actualizar la constante `SHA256` en `install_toolchain.sh`
> con el valor que imprime el script, que además te avisa si te olvidaste.

Qué es cada cosa que se copia y por qué, está explicado en detalle en
[`curso/anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md`](../curso/anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md).

## Manual

### `split_manual.py`

Divide `UM10360.pdf` en un archivo por capítulo dentro de `manual/`. Usa el `pdftotext` de
`tools/bin/`.
