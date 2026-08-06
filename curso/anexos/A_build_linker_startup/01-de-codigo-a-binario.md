# De código a binario: las secciones

## Repaso: las etapas, ahora apuntando al micro

Ya vimos en el [módulo 0](../../00_lenguaje_c/README.md) las etapas de compilación. Para un micro son
las mismas, pero con un **compilador cruzado** (corre en tu PC pero genera código para ARM):

```
main.c ─[preprocesador]→ main.i ─[compilador]→ main.s ─[assembler]→ main.o ─┐
mygpio.c ────────────────────────────────────────────────→ mygpio.o ───────┤
startup.c ───────────────────────────────────────────────→ startup.o ──────┤
                                                                            ▼
                                                              [linker + linker script]
                                                                            ▼
                                                                      firmware.elf
                                                                            ▼
                                                                [objcopy] firmware.bin / .hex
```

La diferencia clave con una PC: en la PC, el ejecutable lo carga el sistema operativo en RAM. Acá
**no hay sistema operativo**: el `.bin` se graba tal cual en la **Flash** del micro, y el hardware lo
ejecuta desde ahí. Por eso el linker tiene que saber exactamente dónde va cada cosa (lo vemos en la
[página 2](./02-linker-y-startup.md)).

> El comando real (lo que MCUXpresso hace por vos, y lo que corrimos para `mygpio`):
> ```
> arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -T lpc1769.ld mygpio.c main.c startup.c -o mygpio.elf
> arm-none-eabi-objcopy -O binary mygpio.elf mygpio.bin
> ```

## .elf vs .bin

- **`.elf`** (Executable and Linkable Format): el archivo "rico". Tiene el código, los datos, **y**
  metadatos: símbolos, info de debug, en qué dirección va cada sección. El **debugger** (módulo 12)
  usa el `.elf` para saber qué línea de C corresponde a cada instrucción.
- **`.bin`** / **`.hex`**: el contenido "pelado" que se graba en la Flash, sin metadatos. Es lo que
  termina byte a byte en el chip.

## Las secciones: dónde vive cada cosa

El linker agrupa todo tu programa en **secciones**, según qué es cada dato y dónde tiene que vivir.
Las cuatro que tenés que conocer:

| Sección | Qué contiene | ¿Dónde vive en ejecución? |
|---------|--------------|---------------------------|
| `.text` | el **código** (las instrucciones) y las constantes (`const`) | **Flash** |
| `.rodata` | datos de solo lectura (strings literales, tablas `const`) | **Flash** |
| `.data` | variables globales/estáticas **con valor inicial** (`int x = 5;`) | **RAM** (copia desde Flash) |
| `.bss` | variables globales/estáticas **sin inicializar** o en cero (`int y;`) | **RAM** (puesta a cero) |

### El detalle clave: `.data` vive en dos lados

Una variable global `int x = 5;` tiene un problema interesante: en ejecución tiene que estar en
**RAM** (porque puede cambiar), pero su valor inicial (`5`) tiene que estar guardado en algún lado
que sobreviva al apagado, o sea en **Flash**. La solución:

1. El valor inicial (`5`) se guarda en **Flash**, pegado al final del `.text`.
2. Al arrancar, el **startup** copia esos valores de la Flash a la RAM (eso es inicializar `.data`).
3. Recién entonces, en RAM, `x` vale `5` y puede modificarse.

Por eso en el linker script vas a ver que `.data` tiene una dirección "en RAM" pero se carga "desde
Flash" (`AT >`). Lo vemos en detalle en la página siguiente.

### `.bss` no ocupa Flash

Las variables sin valor inicial (`uint8_t buffer[1024];`) **no** guardan nada en la Flash: sería un
desperdicio grabar 1024 ceros. Solo se reserva el lugar en RAM y el startup las **pone en cero** al
arrancar. Por eso en C una global sin inicializar arranca en 0 (el startup lo garantiza).

## Leer el tamaño de tu firmware

La herramienta `size` te dice cuánto ocupa cada sección. Para `mygpio`:

```
   text    data     bss     dec     hex   filename
    756       0       0     756     2f4   mygpio.elf
```

- `text` (756 bytes) → va a **Flash**. Tu programa entra de sobra en los 512 KB del LPC1769.
- `data` + `bss` → ocupan **RAM**. `mygpio` no tiene globales, así que 0.

Interpretarlo:
- **Flash usada** = `text` + `data` (el `.data` ocupa Flash para guardar los valores iniciales).
- **RAM usada (estática)** = `data` + `bss`. A eso, en ejecución, se le suma el **stack** y el heap.

Si tu `text` no entra en la Flash, o tu `data`+`bss`+stack no entra en la RAM, el **linker te avisa**
con un error de "region overflowed". Saber leer esto te ahorra dolores.

## La memoria en ejecución, completa

Juntando todo, así queda la RAM del micro mientras corre tu programa:

```
0x10008000  ┌──────────────┐  ← tope de la RAM (_estack): acá arranca el stack
            │    STACK      │  crece hacia ABAJO (llamadas a funciones, variables locales)
            │      ↓        │
            │     ...       │  ← espacio libre
            │      ↑        │
            │     HEAP      │  crece hacia ARRIBA (malloc; en embebidos casi no se usa)
            ├──────────────┤
            │    .bss       │  globales en cero
            │    .data      │  globales con valor inicial (copiadas de Flash)
0x10000000  └──────────────┘  ← base de la RAM
```

El **stack overflow** (sugerencia: causa típica de hard fault, módulo 12) ocurre cuando el stack
crece tanto (recursión profunda, arreglos locales gigantes) que **pisa** el `.bss`/`.data`. No hay
red de seguridad por hardware por defecto: por eso conviene tener locales chicas y evitar recursión
sin fin.

> Este diagrama visto desde el lado del **lenguaje** —qué declaración cae en cada zona, qué es un
> *stack frame*, y cómo **medir** cuánto stack usa tu firmware con `-fstack-usage`, el `ASSERT` del
> linker y la técnica de pintar la RAM— está en
> [Módulo 0, capítulo 10 - Dónde vive cada variable](../../00_lenguaje_c/10-donde-vive-cada-variable.md).

En la [próxima página](./02-linker-y-startup.md) vemos el archivo que define todo este mapa (el
linker script) y el código que lo pone en marcha (el startup).

---

**Módulo:** [Build, linker y startup](./README.md) ·
**Siguiente:** [02 - El linker script y el startup](./02-linker-y-startup.md)
