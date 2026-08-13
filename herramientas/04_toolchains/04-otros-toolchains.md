# Otros toolchains

El curso usa `arm-none-eabi-gcc`: es abierto, está ampliamente soportado y MCUXpresso
incluye una distribución basada en él. Este capítulo ubica las alternativas y distingue un
compilador de un SDK o un IDE que lo empaqueta.

---

## De dónde sacar el GCC para ARM

Antes de mirar alternativas, vale aclarar algo que confunde: **no hay "muchos GCC"**. Hay un
solo proyecto GCC y varias distribuciones del mismo compilador. Estas son las que te vas a
cruzar:

| Distribución | Quién la arma | Cómo se consigue |
|---|---|---|
| **Arm GNU Toolchain** | Arm | un `.tar.xz` del sitio oficial de Arm |
| **xPack GNU Arm Embedded** | la comunidad xPack | igual, y **trae `gdb` autocontenido**, que es su ventaja |
| **`gcc-arm-none-eabi` de Ubuntu** | Debian y Ubuntu | `sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi` |
| **La copia de MCUXpresso** | NXP, pero es la de ARM | ya la tenés si instalaste el IDE |
| **El paquete de este repo** | recortado de la anterior | `bash tools/install_toolchain.sh` |

Comparten el código base de GCC, pero pueden diferir en versión, parches, opciones de
configuración, bibliotecas, GDB y sistemas host soportados.

**Dos advertencias prácticas** que salen de haberlo probado en esta placa:

- La versión **sí importa**. Un gcc 13 y un gcc 7 generan código distinto y tienen warnings
  distintos. Si un ejemplo no compila, mirá la versión antes de dudar del ejemplo.
- **No saques el `gdb` de MCUXpresso.** La build de NXP está linkeada contra
  `libncursesw.so.5` y `libtinfo.so.5`, que ya no existen en Ubuntu moderno, y no arranca.
  Usá `sudo apt install gdb-multiarch` o el toolchain de xPack.

### Cómo usar una distribución binaria

Las distribuciones portables de Arm GNU Toolchain se entregan como una carpeta comprimida.
Podés descomprimirla en un directorio con permisos de usuario y ejecutarla desde ahí:

```bash
tar xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz
./arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc --version
```

No requiere un instalador ni `sudo`. GCC resuelve sus componentes principales mediante rutas
relativas a su ejecutable
([ver 03 - Anatomía de la carpeta](./03-anatomia-de-la-carpeta.md)). Lo único que conviene
hacer después es agregarlo al `PATH`, o apuntarle desde el `Makefile`, que es lo que hace la
[plantilla](../../plantilla/).

---

## LLVM / clang

El otro compilador abierto grande. A diferencia de GCC, **un solo binario de clang compila
para todas las arquitecturas**: no hay un `arm-none-eabi-clang`, hay un `clang` al que le
pasás el objetivo:

```bash
clang --target=arm-none-eabi -mcpu=cortex-m3 -mthumb -c main.c
```

| A favor | En contra |
|---|---|
| Diagnósticos y herramientas de análisis integradas | necesita igualmente una biblioteca C y un linker; puede reutilizar newlib o usar picolibc |
| Ecosistema de análisis (`clang-tidy`, `scan-build`) | varios SDK y scripts de fabricante asumen opciones de GCC |
| Incluye **clangd**, un servidor de lenguaje para varios editores | las opciones, intrínsecos y extensiones no siempre son intercambiables con GCC |
| Sanitizers y herramientas de análisis modernas | |

Aunque compiles con GCC, podés elegir **clangd** para autocompletado y navegación en VSCode,
vim, Zed u otros editores. La extensión C/C++ de Microsoft usa su propio motor, no clangd
([ver 05](./05-el-editor-y-el-entorno.md)).

---

## Los comerciales: IAR y Keil

Siguen muy presentes en la industria, sobre todo en automotriz, médico y aeroespacial.

**IAR Embedded Workbench.** Integra un compilador propio, depurador y herramientas de
análisis. Se licencia comercialmente y ofrece ediciones y paquetes orientados a distintos
niveles de seguridad y soporte.

**Keil MDK** (de Arm). **Arm Compiler 6** se basa en LLVM/Clang y usa librerías y herramientas
de Arm. Los proyectos antiguos pueden usar Arm Compiler 5 (`armcc`), con opciones y ABI que
no son idénticas a las de GCC.

### ¿Por qué alguien paga por esto?

Es una pregunta razonable si GCC es gratis y bueno. Las razones reales:

| Motivo | Detalle |
|---|---|
| **Certificación** | para normas como IEC 61508 o ISO 26262 hace falta un compilador **calificado**, con documentación de sus fallas conocidas. Eso se compra |
| **Soporte** | si el compilador tiene un bug, hay a quién llamar |
| **Tamaño de código** | en un chip de 16 kB, un 10 por ciento es la diferencia entre entrar y no entrar |
| **Herramientas integradas** | analizador de stack, traza, análisis de tiempo real, todo en el mismo paquete |

Para esta materia, GCC cubre las necesidades de build y depuración sin requerir una licencia
comercial.

---

## Los toolchains de fabricante

Varios fabricantes empaquetan GCC con sus propios agregados y le ponen otro nombre:

| Toolchain | De quién | Qué es por debajo |
|---|---|---|
| **MCUXpresso** | NXP | Eclipse + Arm GNU Toolchain + Redlib + LinkServer |
| **STM32CubeIDE** | ST | Eclipse + Arm GNU Toolchain + STM32CubeMX |
| **Code Composer Studio** | Texas Instruments | Eclipse + GCC o el compilador propio de TI |
| **MPLAB X + XC32** | Microchip | GCC con parches |
| **ESP-IDF** | Espressif | GCC para Xtensa o RISC-V, con su propio sistema de build |
| **Pico SDK** | Raspberry Pi | SDK y CMake; usa un toolchain Arm configurado aparte |

La tabla mezcla IDE, SDK y distribuciones de compilador. Muchos se apoyan en GCC o Clang,
pero el fabricante agrega bibliotecas, startup, descripciones del dispositivo, sistema de
build, configuración y herramientas de grabado. Esas capas también afectan la portabilidad.

En el caso estudiado de MCUXpresso aparecen **Redlib**, headers de Code Red/NXP, archivos
generados y LinkServer. Si el proyecto usa Redlib, una parte de la migración consiste en
reemplazar `-specs=redlib.specs` por
`-specs=nano.specs -specs=nosys.specs`. El detalle está en
[07-07](../07_lpc1769/07-mcuxpresso-por-dentro.md).

---

## Y los que no son C

Vale la pena saber que existen, porque el ecosistema embebido dejó de ser solo C:

| Lenguaje | Estado en Cortex-M |
|---|---|
| **C++** | soportado sin problemas por el mismo toolchain. Se usa mucho, con cuidado (sin excepciones ni RTTI en general) |
| **Rust** | maduro. `rustup target add thumbv7m-none-eabi`, y `probe-rs` como grabador. La seguridad de memoria en tiempo de compilación es un argumento fuerte en embebidos |
| **Zig** | trae compilación cruzada incorporada y sirve, además, como cross-compiler de C sin instalar nada más |
| **MicroPython, CircuitPython** | Python corriendo sobre un intérprete embebido. Cómodo para prototipos, caro en RAM |
| **TinyGo** | Go compilado con LLVM para micros |
| **Ada / SPARK** | sigue vivo en aeroespacial y ferroviario, donde la verificación formal importa |

Fijate el nombre del objetivo de Rust: `thumbv7m-none-eabi`. Sigue la misma idea que el
[triplet del capítulo anterior](./02-el-triplet.md), pero identifica con más precisión la
arquitectura Thumb y el perfil ARMv7-M.

---

## Lo que hay que recordar

- Las distribuciones basadas en GCC comparten código fuente, pero pueden cambiar versiones,
  parches, bibliotecas y componentes.
- Una distribución portable puede descomprimirse en una carpeta de usuario.
- Clang también compila para Cortex-M y clangd puede usarse como servidor de lenguaje aunque
  el build se haga con GCC.
- IAR y Keil se pagan por **certificación, soporte y tamaño de código**, no porque GCC no
  funcione.
- Los paquetes de fabricante suelen combinar compilador, SDK, sistema de build y grabador;
  conviene identificar cada capa antes de migrar.

---

**Toolchains:** [índice](./README.md) ·
**Anterior:** [03 - Anatomía de la carpeta](./03-anatomia-de-la-carpeta.md) ·
**Siguiente:** [05 - El editor y el entorno](./05-el-editor-y-el-entorno.md)
