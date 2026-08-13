# Módulo 0: Lenguaje C para sistemas embebidos

Este módulo enseña C desde un programa mínimo hasta las decisiones de representación e interfaz que
aparecen en firmware. Conviene seguirlo en orden: primero se construyen programas sin punteros;
después se introduce el modelo de direcciones y, a partir de ahí, se estudian callbacks, memoria,
MMIO y layout.

## Recorrido C0–C14

| Etapa | Tema | Resultado comprobable |
|---|---|---|
| [C0](./00-introduccion-y-compilacion.md) | Qué es un programa, `main`, edición, compilación, warnings y ejecución | Compilar y modificar un programa mínimo |
| [C1](./01-declaraciones-y-tipos.md) | Declaraciones, tipos, literales, alcance inicial y `stdint.h` | Predecir y comprobar tamaños y rangos |
| [C2](./02-expresiones-operadores-conversiones.md) | Expresiones, operadores, promociones y conversiones | Resolver casos `signed`/`unsigned` sin adivinar |
| [C3](./03-control-de-flujo.md) | `if`, `switch`, `for`, `while` e invariantes simples | Implementar una función con casos límite |
| [C4](./04-funciones.md) | Contrato, parámetros por valor, retorno y prototipos | Separar un problema en funciones comprobables |
| [C5](./05-arreglos-y-strings.md) | Arreglos y strings, primero por índice | Recorrer sin salir de límites |
| [C6](./06-estructuras-y-enums.md) | `struct`, `enum` y modelado de estado | Representar un dispositivo y sus estados |
| [C7](./07-preprocesador.md) | Headers, módulos, preprocesador y compilación separada | Construir un proyecto de varios archivos |
| [C8](./08-punteros.md) | Direcciones, desreferencia, `NULL` y const-correctness | Modificar mediante puntero con pre/postcondiciones |
| [C9](./09-punteros-avanzado.md) | Relación arreglo-puntero, strings, parámetros puntero, callbacks y tablas | Explicar `decay` y crear un callback sencillo |
| [C10A](./10-donde-vive-cada-variable.md) | Duración de almacenamiento, stack, `.data`, `.bss`, heap y linker | Ubicar cada objeto y justificar su vida útil |
| [C10B](./10b-asignacion-dinamica.md) | `malloc`, `free`, fragmentación y alternativas estáticas | Justificar si la asignación dinámica es aceptable |
| [C11](./11-c-para-hardware.md) | `volatile`, `uintptr_t`, MMIO y ancho fijo | Acceder a un registro simulado sin UB evidente |
| [C12](./12-layout-alineacion-unions-y-bitfields.md) | Layout, alineación, padding, aliasing, unions y bitfields | Verificar offsets con `offsetof` y `_Static_assert` |
| [C13](./13-static-const-inline-e-interfaces.md) | `static`, `const`, `inline` y diseño de interfaces | Encapsular un módulo y reducir su API pública |
| [C14](./14-punto-fijo-vs-flotante.md) | Punto fijo, punto flotante y presupuesto de recursos | Elegir una representación con error cuantificado |

C10B amplía el tema del heap y puede leerse después de C10A. No hace falta completarlo antes de
comenzar con el acceso al hardware.

## Progresión de los temas que reaparecen

Algunos conceptos vuelven a aparecer cuando hacen falta en un contexto nuevo:

| Concepto | Primero aparece en | Se retoma en |
|---|---|---|
| `static` | C1: duración y visibilidad inicial | C4: local persistente; C13: funciones/objetos privados, enlace y diseño de API |
| `const` | C1: objeto no modificable por ese nombre | C8: const-correctness de punteros; C11: `const volatile`; C13: contrato y almacenamiento |
| `volatile` | C1: motivación y caso de una ISR | C11: garantías exactas, MMIO, atomicidad y CMSIS |
| bitfields | C12: layout, portabilidad y verificación | C12: comparación con máscaras y costo read-modify-write |
| `inline` | C13: función pequeña como posible sustitución de una llamada | C13: semántica de enlace, headers, optimización y LTO |

## Después del lenguaje: arquitectura de firmware

Los temas siguientes están agrupados en [arquitectura/](./arquitectura/). Conviene trabajarlos
cuando ya se enseñaron [GPIO](../05_gpio/), [SysTick](../06_systick/) e
[interrupciones](../07_interrupciones/):

1. [El superloop y el código no bloqueante](./arquitectura/17-superloop-y-codigo-no-bloqueante.md).
2. [Máquinas de estado](./arquitectura/18-maquinas-de-estado.md).
3. [Cuándo el superloop no alcanza: introducción a RTOS](./arquitectura/19-intro-a-rtos.md).

## `printf` por UART y depuración

La redirección de `printf` se trabaja en
[Herramientas 06 - Depurar en serio](../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md),
después de [UART](../09_uart/). Allí se estudian el *retargeting* de newlib, los
syscall stubs, las mediciones de Flash/RAM/tiempo, buffering, DMA, RTT y semihosting.

## Qué leer de herramientas y cuándo

| Momento | Lectura | Qué completa |
|---|---|---|
| Antes de C0, si se trabajará por terminal | [Instalación en Linux](../../herramientas/07_lpc1769/03-instalacion-linux.md) o [Windows](../../herramientas/07_lpc1769/04-instalacion-windows.md) | Toolchain, plantilla y comandos básicos |
| Después de C7 | [Anatomía del toolchain](../../herramientas/04_toolchains/03-anatomia-de-la-carpeta.md) | Origen de headers, compilador y biblioteca |
| Durante C10 | [Del código al binario](../../herramientas/05_del_codigo_al_binario/) | Startup, linker script, secciones, stack y heap |
| Cuando algo no funcione | [Depurar en serio](../../herramientas/06_depurar_en_serio/) | Método, faults, consola y perturbación de la medición |
| Después de UART | [Redirigir `printf` a UART](../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md) | Instrumentación serie y costos reales |

## Convenciones de lectura

- Los bloques “Para los curiosos (avanzado)” son opcionales y declaran sus prerrequisitos.
- Los ejemplos de LPC1769 concretan el concepto, pero la regla de C se presenta separada del dato del
  microcontrolador.
- Los warnings forman parte del resultado: los ejemplos deberían compilar con `-Wall -Wextra` y,
  donde se indique, convertir warnings críticos en errores.
- Cada capítulo termina con fuentes normativas, documentación del compilador o manuales del
  fabricante para extender el estudio.

Después de C14, el paso siguiente es
[01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/), donde las reglas
del lenguaje se aplican al mapa real del microcontrolador.
