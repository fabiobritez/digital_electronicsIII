# Módulo 0: Lenguaje C para sistemas embebidos

Este módulo es la base: el lenguaje. Está pensado para leerse en orden. Los primeros capítulos son
C "general"; los últimos ya apuntan al hardware y conectan con el módulo
[01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/).

> Si ya sabés C de otra materia, podés saltar directo a los capítulos **12** (`volatile` y tipos para
> hardware) y **13** (structs para hardware), que son los que más se usan para manejar registros, y
> de ahí al módulo 01.

El orden sigue una regla: **primero todo lo que se puede aprender sin punteros**, después los
punteros como el gran salto conceptual, y al final todo lo que depende de ellos y de saber dónde vive
cada dato en la memoria del micro.

> **Antes de empezar (opcional pero recomendado).** Varios capítulos muestran salidas de consola del
> compilador (`sizeof` reales, warnings, desensamblado) e invitan a que las compruebes vos mismo. Para
> poder hacerlo necesitás el toolchain instalado: lo instala `bash tools/install_toolchain.sh` desde
> la raíz del repo, y el paso a paso completo está en el
> [anexo B](../anexos/B_toolchain_y_entorno/): [Linux](../anexos/B_toolchain_y_entorno/06-instalacion-linux.md)
> o [Windows](../anexos/B_toolchain_y_entorno/07-instalacion-windows.md). Si vas a usar MCUXpresso,
> podés saltear esto y leer el módulo igual: ninguna de esas comprobaciones es obligatoria.

**Fundamentos (sin punteros):**

1. [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md): variables,
   `static`/`const`/`volatile`, tipos y su tamaño real en el M3, literales y constantes
2. [02 - Arreglos, conversiones y promociones](./02-arreglos-conversiones-y-promociones.md): arreglos,
   conversión de tipos y las **promociones enteras**, la fuente de bugs sutiles más común del curso
3. [03 - Operadores](./03-operadores.md): aritméticos, lógicos, **bitwise** (clave para registros), asignación
4. [04 - Control de flujo](./04-control-de-flujo.md): if/else, switch, loops, break/continue
5. [05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md): agrupar datos de distinto tipo y
   ponerle nombre a las constantes; la mitad del tema que no necesita punteros

**Modularización:**

6. [06 - Funciones](./06-funciones.md): declaración, definición, parámetros, retorno, recursión
7. [07 - El preprocesador](./07-preprocesador.md): `#include`, `#define`, `#ifdef`, `#pragma`, y
   **cómo se reparte el código entre archivos `.h` y `.c`**, que es lo que necesitás en cuanto tenés
   más de un archivo

**Punteros y memoria (el salto conceptual):**

8. [08 - Punteros](./08-punteros.md): conceptos, aritmética de punteros, `NULL`, const-correctness
9. [09 - Punteros avanzados](./09-punteros-avanzado.md): arreglos y *decay*, cadenas, y **punteros a
   función / callbacks** (tablas de dispatch, máquinas de estado)
10. [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md): **stack, heap y estáticos**:
    las cuatro zonas de memoria de un programa corriendo, el *stack frame* visto en ensamblador,
    el *stack overflow* y cómo medir cuánto stack usás
11. [11 - Asignación dinámica](./11-asignacion-dinamica.md): `malloc`/`free` y por qué se evitan en embebidos

**C para hardware:**

12. [12 - `volatile`, `const` y tipos propios](./12-volatile-y-tipos-para-hardware.md): el puente al
    hardware. Qué garantiza `volatile` **y qué no**, `const volatile` para registros de solo lectura,
    `uintptr_t` y cómo crear tipos propios con `typedef` y el sufijo `_t`
13. [13 - Structs para hardware](./13-structs-para-hardware.md): el operador `->`, *padding* y
    alineación, `packed`, *bitfields*, uniones y el **mapeo de registros** del micro

**C embebido fino (recomendado tras ver algún periférico):**
14. [14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md): el C que usa CMSIS y el buen código embebido
15. [15 - Punto fijo vs punto flotante](./15-punto-fijo-vs-flotante.md): el Cortex-M3 no tiene FPU: cuándo evitar `float`

**Práctica integradora (la librería estándar contra el hardware):**
16. [16 - Redirigir `printf` a la UART](./16-redirigir-printf-a-uart.md): clase práctica: *retargeting* de newlib (`_write`, `_sbrk`),
    hacer que `printf` salga por el cable serie para depurar, el costo de `printf`/`%f`, buffering y alternativas

**Arquitectura de firmware (el cierre: del lenguaje al programa entero):**

17. [17 - El superloop y el código no bloqueante](./17-superloop-y-codigo-no-bloqueante.md): el patrón
    base de todo firmware, por qué `delay()` es el enemigo, la comparación de tiempos que sobrevive al
    desbordamiento del contador, y una tabla de tareas con punteros a función
18. [18 - Máquinas de estado](./18-maquinas-de-estado.md): `enum` + `switch` para modelar
    comportamiento con etapas; acciones de entrada y salida, FSM dirigida por tabla, y por qué el
    estado explícito elimina toda una clase de bugs
19. [19 - Cuando el superloop no alcanza: intro a RTOS](./19-intro-a-rtos.md): qué es un cambio de
    contexto sobre el Cortex-M3, qué te da un RTOS y qué cuesta, la inversión de prioridad, y cuándo
    **no** lo necesitás

> **Los tres últimos capítulos ya no son sobre el lenguaje sino sobre cómo se ordena un programa.**
> Se entienden mejor con GPIO, SysTick e interrupciones vistos (módulos 5, 6 y 7). Si venís derecho,
> leelos igual para tener el mapa y volvé después de los periféricos.

> **Los capítulos 05 y 13 son las dos mitades del mismo tema** (tipos compuestos), separadas por el
> capítulo de punteros: lo que se entiende sin punteros va en el 05, lo que los necesita va en el 13.
> Lo mismo pasa con 01 y 02.

> **Convención:** varios capítulos incluyen bloques marcados **"Para los curiosos (avanzado)"**. Son
> opcionales: si recién empezás, podés saltearlos sin perder el hilo; están ahí para quien quiera ir más a fondo.

> **Lo que sigue después de este módulo:** el módulo 01 muestra que *un registro de hardware es solo
> una dirección de memoria*, y el módulo 02 te invita a **construir tu propia librería** estilo CMSIS
> para que veas que nada de esto está cerrado ni fuera de tu alcance.

---

## Anexos: qué leer del resto del curso, y cuándo

Varios capítulos de acá llegan hasta el borde de lo que se puede explicar sin abrir el build, y ahí
apuntan a los [anexos](../anexos/). **Son opcionales**: no entran en los parciales, y si usás
MCUXpresso no los necesitás para nada. Si te quedaste con la duda, este es el orden que tiene sentido:

| Cuándo | Qué leer | Qué termina de explicar |
|---|---|---|
| Después del **10** y el **11** | [Anexo A - Build, linker y startup](../anexos/A_build_linker_startup/) | Quién pone `.bss` en cero antes de `main`, quién copia `.data` desde la Flash, de dónde salen el tamaño del stack y el del heap, y qué pasa entre el reset y tu primera línea de código |
| Después del **16** (`printf` a la UART) | [Anexo B - El camino completo](../anexos/B_toolchain_y_entorno/00-el-camino-completo.md) | Las once piezas que van de `main.c` al LED encendido, de una sola vez |
| Cuando te dé curiosidad el **07** | [Anexo B - Adentro del toolchain](../anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md) | Por qué `stdint.h` lo da GCC y `stdio.h` lo da newlib, y qué hay en cada carpeta del compilador |
| Cuando quieras compilar sin IDE | [Anexo B - Instalación](../anexos/B_toolchain_y_entorno/06-instalacion-linux.md) + [la plantilla](../../plantilla/) | `make`, `make flash`, `make debug`, sin MCUXpresso |


#   Introducción a C para Sistemas Embebidos

---

## 1. ¿Por qué aprender C hoy?

Aunque tiene más de 50 años, el lenguaje C sigue siendo **el estándar de facto** para programar sistemas embebidos. ¿Por qué?
 
* Es **compacto y rápido**
* Permite **acceso directo al hardware**
* No tiene sobrecarga innecesaria (como lenguajes de más alto nivel) 
* Tiene mucho soporte y portabilidad para plataformas embebidas (ARM, AVR, RISC-V...)

 

C se usa en todos tipo de dispositivos: Microcontroladores de lavarropas, routers, teclados, drones, controladores de motores, satelites, etc.

---

##   2. Breve historia  

C fue creado en los años 70 por Dennis Ritchie en Bell Labs para desarrollar el sistema operativo UNIX.



Fue una evolución de B, un lenguaje creado por Ken Thompson en los años 60.

Recomendación: Leer el libro "The C Programming Language" de Brian Kernighan y Dennis Ritchie. Segunda edición.


En 1989, se crea el estándar ANSI C (C89), que se considera la primera versión de C. Luego, se fueron agregando características, como el soporte para Unicode, el soporte para funciones de variable número de argumentos, etc. 

| Estándar | Año |
|---------|-----|
| C89     | 1989 |
| C99     | 1999 | 
| C11     | 2011 |
| C17     | 2018 |
| C23     | 2023 |


Su diseño refleja una **filosofía minimalista**. El lenguaje en si, es muy simple, el C89 solo tiene 32 palabras reservadas, un conjunto de operadores,tipos de datos, estructuras, etc. 

Luego, se agregan las llamadas "librerias estandar", que son un conjunto de funciones y macros que se pueden usar en cualquier programa en C. No forman parte del nucleo del lenguaje, pero vienen en las versiones que se distribuyen.



---

## 3. Estructura general de un programa en C

Un programa en C se construye a partir de **funciones**. Toda aplicación debe tener una función principal llamada `main()`:

```c
#include <stdio.h>

int main(void) {
    printf("Hello world!\n");
    return 0;
}
``` 

* `#include <stdio.h>`: **directiva de preprocesador** que indica al compilador incluir código de otra parte (en este caso, para usar `printf`)
* `main()`: es la función donde **empieza la ejecución**
* `return 0;`: devuelve al sistema operativo un código indicando éxito

Este es un programa **mínimo** válido en C.

---

## 4. ¿Cómo se compila un programa en C?

La compilación de un programa en C tiene varias **etapas automáticas**:

### Etapas del proceso de construcción

```
main.c ──▶ [Preprocesador] ─▶ main.i
        ──▶ [Compilador]     ─▶ main.s
        ──▶ [Assembler]      ─▶ main.o
        ──▶ [Linker]         ─▶ main.elf / main.exe
```

### ¿Qué hace cada etapa?

1. **Preprocesador (`#`)**

   * Sustituye macros, incluye archivos (`#include`, `#define`)
   * Elimina comentarios
   * Resultado: código expandido (`.i`)
     
Más detalles en [Directivas de preprocesador](./07-preprocesador.md)


2. **Compilador**

   * Traduce C a **ensamblador**
   * Optimiza el código
   * Resultado: `.s`

3. **Assembler**

   * Traduce ensamblador a código binario (instrucciones de máquina)
   * Resultado: `.o` (objeto)

4. **Linker**

   * Une funciones del sistema, bibliotecas y objetos en un solo binario
   * Resultado: ejecutable final (`.elf`, `.bin`, `.hex`...)

> En embebidos, este archivo final **se carga directamente en la memoria del microcontrolador.**

---

## 5. ¿Cómo se organiza un programa grande en C?

En proyectos embebidos reales, no se escribe todo en un solo archivo `.c`. Se separa en:

* **Archivos `.c`** con el código (implementaciones)
* **Archivos `.h`** con declaraciones (headers)

### Ejemplo:

```
main.c        → función principal
led.c         → funciones para manejar un LED
led.h         → declaración de funciones públicas de led.c
config.h      → parámetros generales (#define)
```

### ¿Por qué usar headers?

* Permiten **reutilizar código**
* Hacen más fácil dividir el trabajo
* Son necesarios para que otros archivos conozcan qué funciones existen

> Qué va exactamente en cada archivo y por qué, en
> [07 - El preprocesador](./07-preprocesador.md#por-qué-existen-los-h-cada-c-se-compila-solo).

---

## 6. Compilación simple en Linux y en un entorno embebido

Para compilar un programa C en terminal:

```bash
gcc -o programa main.c
./programa
```

En un entorno embebido (como STM32, LPC, AVR, etc.), se usa un **toolchain cruzado**, por ejemplo:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -o main.elf main.c
```

Y luego se sube al microcontrolador con una herramienta como OpenOCD, STLink, etc.

---

