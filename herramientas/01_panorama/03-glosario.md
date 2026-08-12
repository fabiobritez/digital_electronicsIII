# Glosario

El vocabulario de esta unidad. No hace falta leerlo de corrido: es para volver cuando
aparezca un término que se usa sin aclarar.

Están agrupados por tema, y al final hay una sección con las **confusiones típicas**, que es
la parte que más sirve.

---

## Las dos máquinas

| Término | Qué es |
|---|---|
| **Host** | Tu PC. La máquina donde compilás y donde corre el grabador o el depurador |
| **Target** | El microcontrolador. La máquina para la que compilás y a la que le grabás |
| **Compilación cruzada** (*cross-compilation*) | Compilar en el host para un target de otra arquitectura. Es lo normal en embebidos: nadie compila en el micro |
| **Bare metal** | Software que corre directamente sobre el hardware, sin un sistema operativo de propósito general. Puede incluir bibliotecas o un RTOS |

## El hardware de por medio

| Término | Qué es |
|---|---|
| **Debug probe** / **sonda** / **emulador** | El hardware entre la PC y los pines de depuración del target. Suele ser otro microcontrolador con firmware propio; no es un cable pasivo ([03-01](../03_debug_probes/01-un-probe-es-otro-micro.md)) |
| **JTAG** | Estándar de acceso para prueba estructural y, en muchos chips, para depuración. Usa cuatro señales principales y una de reset opcional ([02-01](../02_protocolos_y_debug_en_el_chip/01-jtag.md)) |
| **SWD** (*Serial Wire Debug*) | Puerto serie de ARM para acceder al sistema de depuración con dos señales. Es habitual en Cortex-M ([02-02](../02_protocolos_y_debug_en_el_chip/02-swd.md)) |
| **SWO** (*Serial Wire Output*) | Pin opcional y unidireccional por el que el sistema de traza puede emitir eventos o texto, una vez configurado |
| **CMSIS-DAP** | Especificación abierta de Arm para comunicar el host con una sonda. Existen varias implementaciones de firmware y la soportan herramientas como OpenOCD y pyOCD ([03-02](../03_debug_probes/02-cmsis-dap.md)) |
| **VTref** | Referencia de tensión del target. La sonda la usa para adaptar o habilitar sus niveles lógicos; normalmente es una entrada, no la alimentación de la placa |
| **ISP** (*In-System Programming*) | Programar el dispositivo sin retirarlo de la placa. En los LPC suele aludir al bootloader de fábrica por UART |
| **IAP** (*In-Application Programming*) | Programar la FLASH desde código que corre en el propio micro; en el LPC1769 se hace mediante rutinas de la ROM |

## El software del lado de la PC

| Término | Qué es |
|---|---|
| **Toolchain** | Conjunto de herramientas para construir software: compilador, ensamblador, linker y utilidades. Una distribución también puede incluir biblioteca C y depurador ([parte 04](../04_toolchains/)) |
| **Target triplet** | Nombre canónico que identifica la plataforma objetivo de un toolchain, por ejemplo `arm-none-eabi`. Aunque se lo llame *triplet*, puede tener más de tres campos ([04-02](../04_toolchains/02-el-triplet.md)) |
| **binutils** | El paquete GNU con `as`, `ld`, `objcopy`, `objdump`, `nm`, `readelf`, `size`, `strip` |
| **newlib** / **newlib-nano** | Implementación de la biblioteca estándar de C usada en muchos sistemas embebidos. Newlib-nano prioriza menor tamaño y reduce algunas funciones |
| **Flash loader** / *flash algorithm* | Código específico del chip que el grabador suele cargar en la RAM del target para borrar y escribir su FLASH. Los paquetes CMSIS usan archivos `.FLM`; NXP también usa `.cfx` |
| **gdbserver** | Programa que traduce entre GDB y el target mediante el **GDB Remote Serial Protocol**, normalmente sobre TCP. Así, GDB no necesita manejar USB ni SWD |
| **OpenOCD** | El gdbserver + grabador libre más usado. Soporta cientos de sondas y de chips |
| **pyOCD** | Lo mismo, en Python, especializado en Cortex-M |
| **Semihosting** | Mecanismo por el que el firmware pide al depurador que ejecute operaciones en el host, como mostrar texto. La implementación clásica detiene el núcleo en cada pedido y puede ser muy lenta |
| **RTT** (*Real Time Transfer*) | Consola por el cable del debugger, sin frenar el micro: el firmware escribe en una cola en RAM y el host la lee ([06-04](../06_depurar_en_serio/04-consola-por-el-debugger-rtt.md)) |

## Las capas de código

Acá es donde más se confunde la gente, porque los tres términos suenan parecido y los
fabricantes los usan como quieren.

| Término | Qué es |
|---|---|
| **CMSIS** | *Common Microcontroller Software Interface Standard*. Lo define **ARM** y es la capa del **núcleo**: los nombres de los registros del Cortex-M (`NVIC`, `SysTick`, `SCB`), las funciones intrínsecas, y el header con los periféricos del chip que aporta el fabricante (`LPC17xx.h`) |
| **HAL** / **driver** | *Hardware Abstraction Layer*. Funciones que abstraen los registros de los periféricos: `UART_Send()`, `GPIO_SetValue()`. Puede escribirla el fabricante o el propio proyecto; en este repo se usan drivers `lpc17xx_*.c` de NXP |
| **BSP** | *Board Support Package*. La capa de la **placa**, no del chip: "el LED está en P0.22", "el botón en P2.10". Rara vez viene por separado en placas chicas |
| **SDK** | El paquete completo que da el fabricante: CMSIS + HAL + BSP + ejemplos + a veces un RTOS. El SDK de MCUXpresso es esto |
| **IDE** | Entorno que integra edición, build y depuración. Para generar código invoca un toolchain incluido o configurado; MCUXpresso se basa en Eclipse y usa `arm-none-eabi-gcc` |

Y ojo con una trampa de nombres de esta materia: **CMSIS-DAP** (el firmware de sondas) y
**CMSIS** (la capa de software del núcleo) comparten las primeras cinco letras, pero
designan componentes distintos del ecosistema de ARM. Ver
[03-02](../03_debug_probes/02-cmsis-dap.md).

## Los archivos

| Término | Qué es |
|---|---|
| **`.o`** | Archivo objeto reubicable: contiene código, datos y símbolos, pero todavía no tiene sus direcciones finales |
| **`.a`** | Un montón de `.o` empaquetados: una biblioteca estática |
| **`.elf`** | El ejecutable "rico": código ubicado, símbolos, e información de depuración. Es lo que abre gdb |
| **`.axf`** | Un `.elf` con otra extensión. Herencia de las herramientas de ARM, lo usa MCUXpresso |
| **`.bin`** | Los bytes pelados, sin metadatos, tal cual van a la FLASH. No dice a qué dirección va |
| **`.hex`** | Intel HEX: texto con los bytes **y sus direcciones**. Es un formato habitual en herramientas de ISP |
| **`.map`** | El informe del linker: muestra dónde quedó cada símbolo y cuánto ocupa. Es clave para analizar el uso de memoria |
| **`.ld`** | El **linker script**: el mapa de memoria del chip ([05-02](../05_del_codigo_al_binario/02-linker-y-startup.md)) |
| **`.specs`** | Archivo que modifica las reglas con las que el driver de GCC invoca compilador, linker y bibliotecas. Se usa, por ejemplo, para seleccionar newlib-nano |

---

## Las confusiones típicas

**"Depurador" es tres cosas distintas.** Cuando alguien dice "el debugger", puede estar
hablando de: el **programa** de la PC (gdb), el **hardware** que se enchufa al USB (la
sonda), o el **hardware adentro del chip** (la unidad de depuración del Cortex-M3). En esta
unidad se llama a cada uno por su nombre, justamente por eso.

**Una sonda no es un cable adaptador.** Es hardware activo: en la mayoría de las sondas
actuales hay un microcontrolador con firmware propio que habla USB de un lado y SWD o JTAG
del otro. Puede actualizarse, fallar o quedar ocupado por otra herramienta.

**El IDE no es el compilador.** MCUXpresso integra un editor, un sistema de build y una
distribución de `arm-none-eabi-gcc`, pero son componentes distintos. Un proyecto puede
compilarse fuera del IDE si reproducís sus opciones, archivos generados y dependencias.

**"Grabó bien" no es "anda".** `Verified OK` significa que los bytes quedaron escritos en la
FLASH. Que el chip los ejecute es otra cosa, y depende de que la imagen sea válida para su
boot ROM. Es exactamente el problema que se explica en
[07-02](../07_lpc1769/02-la-boot-rom-el-isp-y-el-checksum.md).

**JTAG y SWD no son "protocolos de grabación".** Son puertos de **acceso** a la memoria y al
CPU. Grabar la FLASH es una cosa que se hace *usando* ese acceso, y por eso el grabador
necesita saber además cómo es la FLASH de tu chip en particular.

**`-mcpu` no es lo mismo que el triplet.** El triplet (`arm-none-eabi`) dice qué toolchain
usás; `-mcpu=cortex-m3` le dice a ese toolchain para cuál ARM en particular generar el
código. Con el triplet correcto y un `-mcpu` equivocado, el build puede completarse y producir
código incompatible con el núcleo.

---

**Panorama:** [índice](./README.md) ·
**Anterior:** [02 - Cómo se graba un micro](./02-como-se-graba-un-micro.md) ·
**Siguiente parte:** [02 - Protocolos y debug en el chip](../02_protocolos_y_debug_en_el_chip/)
