# Herramientas: cómo se programa y se depura una placa

Esta es una **unidad aparte del curso**. El [curso](../curso/) enseña el LPC1769: sus
periféricos, sus registros, cómo se escribe firmware. Esta unidad enseña lo otro, lo que
está *alrededor* del chip y casi nunca se explica:

> ¿Qué pasa exactamente entre el archivo de texto que escribís y el transistor que se
> enciende en la placa? ¿Qué hardware hay entre tu PC y el micro? ¿Cómo hace un depurador
> para frenar un chip que está corriendo?

Es material complementario: puede consultarse durante la materia sin seguir el orden del curso. Los
principios de compilación, programación y diagnóstico se transfieren a otras plataformas, aunque
cada arquitectura, chip y placa cambie protocolos, memoria, arranque y herramientas.

---

## Cómo está organizada

La unidad va **de lo general a lo particular**. Las partes 01 a 06 presentan conceptos
transferibles y usan Cortex-M3 o el LPC1769 como ejemplos cuando ayuda. La parte 07 reúne el
procedimiento y los datos específicos de la placa de la cátedra.

```
   01  PANORAMA                 el mapa entero, en una lectura
        │
        ├── 02  PROTOCOLOS      JTAG, SWD, y el hardware de debug adentro del Cortex-M3
        ├── 03  DEBUG PROBES    qué es una sonda, CMSIS-DAP, cuáles existen
        ├── 04  TOOLCHAINS      el compilador cruzado, el triplet, qué hay en la carpeta
        ├── 05  DEL CODIGO AL BINARIO   secciones, linker script, startup, arranque
        └── 06  DEPURAR EN SERIO        imprimir, el método, hard faults, RTT
                │
                ▼
           07  LPC1769          todo lo anterior, aplicado a esta placa
```

| # | Parte | Qué responde |
|---|-------|--------------|
| 01 | [Panorama](./01_panorama/) | Las piezas que van de `main.c` al LED, nombradas una por una. Las dos formas físicas de grabar un micro. El vocabulario que usa todo el resto |
| 02 | [Protocolos y debug en el chip](./02_protocolos_y_debug_en_el_chip/) | Qué son JTAG y SWD, qué hardware de depuración trae el Cortex-M3 adentro del núcleo (CoreSight: DAP, FPB, DWT, ITM), y por qué se puede parar un chip sin que el programa colabore |
| 03 | [Debug probes](./03_debug_probes/) | Por qué una sonda es hardware activo, a menudo otro micro con firmware; qué especifica CMSIS-DAP y qué software del host controla cada modelo |
| 04 | [Toolchains](./04_toolchains/) | Qué es un toolchain, cómo se lee el **target triplet** (`arm-none-eabi`), qué hay adentro de la carpeta del compilador, el multilib, los `.specs`, y qué otros toolchains existen |
| 05 | [Del código al binario](./05_del_codigo_al_binario/) | Las secciones `.text`/`.data`/`.bss`, el linker script, el código de arranque, y la secuencia completa de 0 V a `main()` |
| 06 | [Depurar en serio](./06_depurar_en_serio/) | Señales de diagnóstico, un método ordenado, análisis de faults y RTT como consola en RAM leída por la interfaz de debug |
| 07 | [LPC1769](./07_lpc1769/) | La placa de la cátedra, su sonda, la boot ROM, el checksum que hace que "grabé y no hace nada", instalación en Linux y Windows, una guía por cada sonda, y cómo compila y graba MCUXpresso por dentro |

---

## Por dónde empezar

Depende de a qué viniste:

**"Solo quiero compilar y grabar, ya."**
[Instalación en Linux](./07_lpc1769/03-instalacion-linux.md) o
[en Windows](./07_lpc1769/04-instalacion-windows.md), copiás la
[plantilla](../plantilla/), y `make flash`. El resto lo leés cuando algo falle.

**"Quiero entender qué hace el IDE por mí."**
[El mapa completo](./01_panorama/01-el-mapa-completo.md), que es todo en una página, y de
ahí seguís por la parte que te haya dado más curiosidad.

**"¿Qué es esa cosita soldada en el rincón de la placa?"**
[Una sonda es otro micro](./03_debug_probes/01-un-probe-es-otro-micro.md) y después
[CMSIS-DAP](./03_debug_probes/02-cmsis-dap.md).

**"¿Cómo puede el debugger frenar el chip y ver mis variables?"**
[El debug adentro del Cortex-M3](./02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md).

**"Grabé, verificó, pero la aplicación no responde."**
Empezá por [la boot ROM, el ISP y el checksum](./07_lpc1769/02-la-boot-rom-el-isp-y-el-checksum.md)
y seguí con [el método del “no anda”](./06_depurar_en_serio/02-el-metodo-del-no-anda.md).

**"Me colgué en un hard fault."**
[Hard faults](./06_depurar_en_serio/03-hard-faults.md).

**"¿Qué quiere decir `arm-none-eabi`?"**
[El target triplet](./04_toolchains/02-el-triplet.md).

---

## Lo que ya está resuelto: la plantilla

En [`plantilla/`](../plantilla/) hay un proyecto de referencia para el LPC1769. Antes de copiarlo,
leé `make info` y verificá qué toolchain y servidor detectó:

```bash
cd plantilla
make            # compila  -> build/firmware.elf, .bin, .hex
make flash      # graba la placa
make debug      # graba y abre gdb, parado en main
make rtt        # abre la consola por el cable del debugger
make help       # todos los comandos
```

La unidad explica las capas que intervienen y relaciona los archivos de la plantilla con su función.
Los ejemplos permiten comparar ese flujo explícito con el build administrado de MCUXpresso.

---

## Qué del curso apunta acá

Esta unidad no es un desvío. Varios capítulos del curso llegan al límite de lo que se puede
explicar sin abrir el build, y mandan para acá:

| Desde | Para qué |
|---|---|
| [C7 - Preprocesador](../curso/00_lenguaje_c/07-preprocesador.md) | dónde viven físicamente `stdint.h` y `stdio.h` |
| [C10 - Dónde vive cada variable](../curso/00_lenguaje_c/10-donde-vive-cada-variable.md) | quién pone `.bss` en cero y de dónde sale el tamaño del stack |
| [C10B - Asignación dinámica](../curso/00_lenguaje_c/10b-asignacion-dinamica.md) | de dónde sale el heap y cómo se escribe `_sbrk` |
| [06.05 - Redirigir `printf` a la UART](./06_depurar_en_serio/05-redirigir-printf-a-uart.md) | los syscall stubs de newlib, sus costos y el heap que necesitan |
| [01 - Arquitectura y acceso a registros](../curso/01_arquitectura_y_acceso_a_registros/) | qué pasa entre el reset y la primera línea de `main` |
| [02 - Armá tu propia librería](../curso/02_arma_tu_propia_libreria/) | el `startup.c` y el `.ld` con los que se compiló `mygpio` |
| [09 - UART](../curso/09_uart/) | la consola de depuración, y por qué a veces conviene RTT en su lugar |
| [17 - Hardware y placa](../curso/17_hardware_y_placa/) | cuándo el problema no es software y hay que sacar el osciloscopio |

---

## Convención de esta unidad

Las partes 01 a 06 desarrollan conceptos generales, pero varios ejemplos y conteos pertenecen a
Cortex-M3 o al LPC1769 y se identifican en el texto. La parte 07 es deliberadamente específica de la
placa de la cátedra.

Si usás otra plataforma, trasladá el método (separar capas, verificar supuestos y medir efectos), no
los nombres de registros ni los comandos sin contrastarlos con su documentación.

---

**Volver al** [repositorio](../README.md) · **El curso** [acá](../curso/README.md)
