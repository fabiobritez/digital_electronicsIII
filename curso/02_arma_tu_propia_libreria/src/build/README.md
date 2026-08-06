# Build de mygpio (toolchain local)

Compila la librería [`mygpio`](../) a un firmware real para el LPC1769, **sin instalar nada en el
sistema**: usa el toolchain `arm-none-eabi-gcc` que está en `tools/toolchain/` del repo.

```bash
./build.sh      # opción A: NO necesita 'make', solo el toolchain local
# o, si tenés make instalado:
make            # opción B: genera mygpio.elf y mygpio.bin, e imprime el tamaño
make clean
```

> En la VPS no estaba `make` instalado, por eso se incluye `build.sh` (invoca el compilador directo).
> El `Makefile` queda para entornos que sí lo tengan (MCUXpresso lo usa por dentro).

- `startup.c`: startup mínimo para Cortex-M3: tabla de vectores (SP inicial + Reset + excepciones),
  copia de `.data` a RAM y puesta a cero de `.bss`, y salto a `main`. Es el equivalente didáctico,
  en C, de lo que MCUXpresso genera por vos (ver módulo 1).
- `lpc1769.ld`: linker script con Flash en `0x0` (512K), SRAM en `0x10000000` (32K), `_estack` arriba
  de la RAM.
- El `.bin` resultante (~756 bytes) se puede cargar en la placa.

> El toolchain se instaló localmente con `tools/install_toolchain.sh`. No está en git
> (ver `.gitignore`).
