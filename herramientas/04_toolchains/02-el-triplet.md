# El target triplet

Todos los programas del toolchain empiezan con lo mismo:

```
arm-none-eabi-gcc
arm-none-eabi-ld
arm-none-eabi-objcopy
└────┬────┘
     este prefijo
```

Ese prefijo se llama **target triplet** (o simplemente *triplet*), y no es decorativo:
describe **para qué máquina** genera código ese toolchain. Saber leerlo es lo que te permite
mirar el nombre de un compilador y saber, sin abrir nada, si sirve para tu placa.

---

## De dónde viene

La convención nace en el mundo GNU, con `autoconf`, para poder describir máquinas de forma
uniforme. Se llama "triplet" porque originalmente eran tres campos, pero en la práctica
pueden ser dos, tres o cuatro:

```
   <arquitectura> - <fabricante> - <sistema operativo> - <ABI / libc>
```

El parser rellena los campos que faltan según la posición y unas cuantas reglas históricas.
Por eso vas a ver la misma máquina escrita de varias formas, todas válidas:

```
x86_64-linux-gnu           ≡   x86_64-pc-linux-gnu       ≡   x86_64-unknown-linux-gnu
```

Y por eso hay que leerlo con criterio y no como una gramática rígida. Lo que sigue son los
campos y qué significa cada valor.

---

## Campo 1: la arquitectura

Qué instrucciones entiende el procesador de destino. Es el campo que **nunca** falta.

| Valor | Qué es |
|---|---|
| `arm` | ARM de 32 bits: todos los Cortex-M, Cortex-R y los Cortex-A viejos |
| `aarch64` | ARM de 64 bits: Cortex-A modernos, Raspberry Pi 4 en adelante, Apple Silicon |
| `x86_64` / `amd64` | tu PC, si es Intel o AMD de 64 bits |
| `i686` | PC de 32 bits |
| `riscv32` / `riscv64` | RISC-V |
| `avr` | los AVR de 8 bits (Arduino UNO) |
| `xtensa` | los ESP8266 y ESP32 |
| `msp430` | los MSP430 de Texas |
| `mips`, `powerpc`, `sparc` | routers, sistemas viejos, aeroespacial |

---

## Campo 2: el fabricante

Suele aportar menos información técnica que la arquitectura o el sistema, aunque a veces
identifica un ecosistema concreto.

| Valor | Qué quiere decir |
|---|---|
| `none` | nada, es un relleno |
| `unknown` | nada, es otro relleno |
| `pc` | nada, es el relleno habitual en Linux de escritorio |
| `apple` | acá sí importa: identifica el ecosistema de macOS |
| `w64` | el proyecto MinGW-w64, para generar ejecutables de Windows |

En `arm-none-eabi`, el `none` que ves ocupa **este** campo. La lectura formal es entonces
*"ARM, fabricante ninguno, entorno EABI"*.

> Vas a encontrar mucha documentación (y una versión anterior de este mismo material) que lee
> ese `none` como "sistema operativo: ninguno". La conclusión práctica es la misma, porque
> `eabi` ya implica bare metal, pero conviene saber cuál es la estructura real: los campos se
> llenan por posición, y el que sigue a la arquitectura es el fabricante.

---

## Campo 3 y 4: el sistema operativo y la ABI

Acá está la información que de verdad importa, y los dos campos suelen venir fusionados.

| Valor | Qué significa |
|---|---|
| `linux-gnu` | Linux, con la biblioteca C de GNU (glibc) |
| `linux-musl` | Linux, con musl (más chica, típica de Alpine) |
| `linux-gnueabi` | Linux en ARM, ABI de coma flotante **por software** |
| `linux-gnueabihf` | Linux en ARM, ABI de coma flotante **por hardware** (*hard float*) |
| `linux-android` | Android |
| `darwin` | macOS |
| `mingw32` / `windows-msvc` | Windows |
| **`eabi`** en `arm-none-eabi` | entorno bare metal que usa la *Embedded Application Binary Interface* de Arm |
| **`elf`** | sin sistema operativo, y el formato de salida es ELF. Es lo que usan RISC-V y otros |
| `none` | sin sistema operativo, sin más precisiones |

### Qué es una ABI, y por qué aparece acá

Una **ABI** (*Application Binary Interface*) es el conjunto de acuerdos que tienen que
respetar dos pedazos de código compilados por separado para poder llamarse:

- En qué registro va cada argumento de una función, y dónde vuelve el resultado.
- Qué registros puede pisar la función llamada y cuáles tiene que preservar.
- Cómo se alinean las estructuras en memoria, y cuánto ocupa cada tipo.
- Cómo se pasan los `float` y los `double`.

Si dos objetos no comparten la ABI, el linker suele detectarlo mediante atributos del ELF.
Si no lo detecta, los argumentos o los datos pueden interpretarse de forma incompatible en
ejecución. Por eso todos los objetos y bibliotecas deben usar opciones de ABI coherentes.

La **EABI** reúne convenciones binarias definidas por Arm para sistemas embebidos. EABI no
significa por sí sola "sin sistema operativo": también aparece en objetivos Linux como
`arm-linux-gnueabi`. En `arm-none-eabi`, la combinación completa es la que indica el
entorno bare metal.

### Coma flotante: el triplet no alcanza

En targets Linux vas a ver sufijos como `gnueabi` y `gnueabihf`. En bare metal se conserva
habitualmente el nombre `arm-none-eabi` y la ABI de coma flotante se elige con banderas:

| | `-mfloat-abi=soft` | `-mfloat-abi=softfp` | `-mfloat-abi=hard` |
|---|---|---|---|
| Paso de argumentos `float` | registros enteros | registros enteros | registros de la FPU |
| Instrucciones de FPU | no | sí, si se indica `-mfpu` | sí, si se indica `-mfpu` |
| Compatibilidad de llamada | soft | soft | hard |

`softfp` permite usar instrucciones de FPU sin cambiar la convención de llamadas. En cambio,
mezclar objetos `soft/softfp` con `hard` cambia dónde se pasan los argumentos. El linker
normalmente rechaza esa combinación, pero no conviene depender del diagnóstico: usá las
mismas opciones en todo el proyecto.

---

## Leamos el nuestro

```
arm-none-eabi
 │    │    └── sin sistema operativo, con la EABI de ARM. Bare metal
 │    └─────── fabricante: ninguno (relleno)
 └──────────── arquitectura: ARM de 32 bits
```

Comparado con el `gcc` de tu PC, `x86_64-linux-gnu`, la diferencia de fondo es **el tercer
campo**: `linux-gnu` contra `eabi`. Y esa diferencia arrastra todo lo demás:

| | `x86_64-linux-gnu` | `arm-none-eabi` |
|---|---|---|
| ¿Hay sistema operativo? | sí | **no** |
| ¿Quién carga el programa en memoria? | el kernel | nadie: ya está en la FLASH |
| ¿A dónde escribe `printf`? | a un descriptor de archivo | **a donde vos digas** |
| ¿Quién inicializa las globales? | el runtime y el cargador | **el startup del firmware** |
| ¿Qué pasa cuando `main` retorna? | vuelve al shell | no hay a dónde volver |
| Biblioteca C | glibc | newlib o newlib-nano |
| ¿El binario corre en la máquina que compila? | sí | **no** |

Esa última fila es la definición de **compilación cruzada**: tu PC solo **fabrica** el
firmware, y quien lo ejecuta es el LPC1769. Por eso el prefijo existe: para que puedas tener
en la misma máquina el `gcc` de tu PC y uno o varios compiladores cruzados sin que se pisen.

---

## Otros triplets que te vas a cruzar

| Triplet | Para qué sirve |
|---|---|
| **`arm-none-eabi`** | **cualquier ARM de 32 bits bare metal.** Cortex-M0, M3, M4, M7, y también Cortex-A sin sistema operativo |
| `aarch64-none-elf` | ARM de 64 bits bare metal |
| `arm-linux-gnueabihf` | una Raspberry Pi 2 o 3 corriendo Linux de 32 bits |
| `aarch64-linux-gnu` | una Raspberry Pi 4 o 5, o un servidor ARM |
| `riscv32-unknown-elf` | microcontroladores RISC-V, bare metal |
| `xtensa-esp32-elf` | variantes ESP32 con núcleo Xtensa; otras usan RISC-V |
| `avr` | Arduino UNO. Un solo campo, porque el AVR no da lugar a ambigüedad |
| `msp430-elf` | MSP430 |
| `x86_64-w64-mingw32` | generar `.exe` de Windows desde Linux |

Fijate lo que **no** hay: no existe un `lpc1769-gcc` ni un `cortexm3-gcc`. Lo que nos lleva al
punto más importante de esta página.

---

## Lo que el triplet NO te dice

Y es mucho.

> **`arm-none-eabi` cubre una familia muy amplia de Arm de 32 bits en bare metal.** El mismo
> toolchain puede generar código para núcleos muy distintos; el triplet no elige uno en
> particular.

Lo que falta se lo decís con banderas:

| Bandera | Qué define |
|---|---|
| `-mcpu=cortex-m3` | **el núcleo concreto**. Es la más importante de todas |
| `-mthumb` | el estado de instrucciones Thumb; el repertorio exacto depende del núcleo (el Cortex-M3 implementa Thumb-2) |
| `-mfloat-abi=soft\|softfp\|hard` | cómo se pasan los `float` |
| `-mfpu=fpv4-sp-d16` | qué FPU tiene el chip, si tiene |
| `-T lpc1769.ld` | el mapa de memoria: dónde está la FLASH y dónde la RAM |

Un `-mcpu` equivocado puede producir un error durante el build o, en el peor caso, una
imagen que usa instrucciones no admitidas por el núcleo y falla al ejecutarse. Verificá estas
banderas junto con las bibliotecas multilib seleccionadas.

Y del otro lado está la buena noticia, que es la razón de que todo esto valga la pena
aprenderlo:

> Al portar el proyecto a un STM32F4 puede mantenerse el toolchain `arm-none-eabi`, pero
> cambian las opciones de CPU/FPU, el mapa de memoria, el startup, las bibliotecas de
> dispositivo, el algoritmo de FLASH y la configuración del servidor. Las etapas generales
> del flujo siguen siendo las mismas.

---

## Comandos útiles

Preguntarle a un compilador para qué máquina compila:

```bash
arm-none-eabi-gcc -dumpmachine
# arm-none-eabi

gcc -dumpmachine
# x86_64-linux-gnu
```

Ver qué variante de biblioteca va a elegir con tus banderas (esto se explica en
[03 - Anatomía de la carpeta](./03-anatomia-de-la-carpeta.md)):

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -print-multi-directory
# thumb/v7-m/nofp
```

Y para verificar en un binario ya compilado con qué ABI quedó:

```bash
arm-none-eabi-readelf -A build/firmware.elf | grep -iE "cpu_arch|fp_arch|abi_vfp"
```

---

## Lo que hay que recordar

- El triplet describe **la máquina de destino**, no el chip exacto.
- La estructura es `arquitectura - fabricante - so - abi`, con campos que se omiten y un
  campo de fabricante que casi nunca significa nada.
- **`arm-none-eabi` = ARM de 32 bits, sin sistema operativo, con la EABI de ARM.**
- La ABI de coma flotante debe coincidir en todos los objetos. En bare metal se controla con
  `-mfloat-abi` y `-mfpu`.
- El triplet **no dice qué Cortex**. Eso lo define `-mcpu`; una opción incorrecta puede
  fallar en el build o en ejecución.
- El prefijo permite distinguir el toolchain cruzado del compilador nativo instalado en el
  mismo host.

---

**Toolchains:** [índice](./README.md) ·
**Anterior:** [01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md) ·
**Siguiente:** [03 - Anatomía de la carpeta](./03-anatomia-de-la-carpeta.md)
