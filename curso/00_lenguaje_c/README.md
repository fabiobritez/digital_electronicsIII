# Módulo 0: Lenguaje C para sistemas embebidos

Este módulo es la base: el lenguaje. Está pensado para leerse en orden. Los primeros capítulos son
C "general"; los últimos ya apuntan al hardware y conectan con el módulo
[01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/).

> Si ya sabés C de otra materia, podés saltar directo a los capítulos **08** (ancho fijo y
> `volatile`) y **07** (structs/unions), que son los que más se usan para manejar registros, y de ahí
> al módulo 01.

**Fundamentos:**
1. [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md): variables, tipos, arreglos, constantes
2. [02 - Operadores](./02-operadores.md): aritméticos, lógicos, **bitwise** (clave para registros), asignación
3. [03 - Control de flujo](./03-control-de-flujo.md): if/else, switch, loops, break/continue

**Modularización:**
4. [04 - Funciones](./04-funciones.md): declaración, definición, parámetros, retorno, recursión

**Memoria y punteros:**
5. [05 - Punteros](./05-punteros.md): conceptos, aritmética de punteros
6. [06 - Punteros avanzados](./06-punteros-avanzado.md): arreglos y *decay*, cadenas, y **punteros a función / callbacks** (tablas de dispatch, máquinas de estado)

**Tipos compuestos y C para hardware:**
7. [07 - Estructuras, uniones y enums](./07-estructuras-uniones-enums.md): la base de los *struct overlay* de registros
8. [08 - Tipos de ancho fijo, `volatile` y `const`](./08-tipos-de-ancho-fijo-y-volatile.md): **C embebido**: `uint32_t`, `volatile`, datos en Flash
9. [09 - Asignación dinámica](./09-asignacion-dinamica.md): `malloc`/`free` y por qué se evitan en embebidos

**Compilación:**
10. [10 - Preprocesador](./10-preprocesador.md): `#include`, `#define`, `#ifdef`, `#pragma`

**C embebido fino (recomendado tras ver algún periférico):**
11. [11 - `static`, `const`, `inline` y campos de bits](./11-static-const-inline-y-bitfields.md): el C que usa CMSIS y el buen código embebido
12. [12 - Punto fijo vs punto flotante](./12-punto-fijo-vs-flotante.md): el Cortex-M3 no tiene FPU: cuándo evitar `float`

**Práctica integradora (la librería estándar contra el hardware):**
13. [13 - Redirigir `printf` a la UART](./13-redirigir-printf-a-uart.md): clase práctica: *retargeting* de newlib (`_write`, `_sbrk`),
    hacer que `printf` salga por el cable serie para depurar, el costo de `printf`/`%f`, buffering y alternativas

> **Convención:** varios capítulos incluyen bloques marcados **"Para los curiosos (avanzado)"**. Son
> opcionales: si recién empezás, podés saltearlos sin perder el hilo; están ahí para quien quiera ir más a fondo.

> **Lo que sigue después de este módulo:** una vez que tenés el lenguaje, el módulo 01 muestra que
> *un registro de hardware es solo una dirección de memoria*, y el módulo 02 te invita a **construir
> tu propia librería** estilo CMSIS para que veas que nada de esto está cerrado ni fuera de tu alcance.


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

int main() {
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
     
Más detalles en [Directivas de preprocesador](./10-preprocesador.md)


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

## 6. ¿Cómo se organiza un programa grande en C?

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

---

## 7. Compilación simple en Linux y en un entorno embebido

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

