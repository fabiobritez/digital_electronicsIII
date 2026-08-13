# C0 - Introducción a C y al proceso de compilación

## 1. ¿Por qué aprender C hoy?

Aunque tiene más de 50 años, el lenguaje C sigue siendo **el estándar de facto** para programar sistemas embebidos. ¿Por qué?

* Es **compacto y rápido**
* Permite **acceso directo al hardware**
* No tiene sobrecarga innecesaria (como lenguajes de más alto nivel)
* Tiene mucho soporte y portabilidad para plataformas embebidas (ARM, AVR, RISC-V...)



C aparece en todo tipo de dispositivos: lavarropas, routers, teclados, drones, controladores de
motores, satélites y muchos otros equipos.

---

##   2. Breve historia

C fue creado en los años 70 por Dennis Ritchie en Bell Labs para desarrollar el sistema operativo UNIX.



Fue una evolución de B, un lenguaje creado por Ken Thompson en los años 60.

Una buena lectura para acompañar el curso es la segunda edición de *The C Programming Language*, de
Brian Kernighan y Dennis Ritchie.


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
> [C7 - El preprocesador](./07-preprocesador.md#por-qué-existen-los-h-cada-c-se-compila-solo).

---

## 6. Compilación simple en Linux y en un entorno embebido

Para compilar un programa C en terminal:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -o programa main.c
./programa
```

No tomes los warnings como decoración. La compilación solo debería considerarse correcta cuando no
queda ninguno sin entender:

```c
int main(void) {
    int medicion;
    return medicion;  // se usa sin haber recibido un valor
}
```

```console
$ gcc -std=c17 -Wall -Wextra -Wpedantic main.c
warning: ‘medicion’ is used uninitialized [-Wuninitialized]
```

El ciclo mínimo de trabajo es:

1. editar una única intención;
2. compilar con warnings habilitados;
3. leer el **primer** diagnóstico y corregir su causa;
4. ejecutar y comprobar un resultado observable;
5. modificar un dato o condición y predecir qué debería cambiar antes de volver a ejecutar.

Un error detiene la traducción; un warning indica que el compilador encontró algo legal o tolerado,
pero probablemente distinto de lo que querías. No se “arregla” agregando un cast o deshabilitando la
advertencia sin explicar antes la regla involucrada.

En un entorno embebido (como STM32, LPC, AVR, etc.), se usa un **toolchain cruzado**, por ejemplo:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -o main.elf main.c
```

Y luego se sube al microcontrolador con una herramienta como OpenOCD, STLink, etc.

La diferencia conceptual es importante: el programa compilado con `gcc` se ejecuta en la PC y puede
volver a un sistema operativo. El producido por `arm-none-eabi-gcc` usa instrucciones ARM Thumb,
arranca mediante el código de startup del micro y normalmente permanece en un bucle infinito. Por
eso compilar el `.elf` no alcanza para “ejecutarlo”: hay que grabarlo en el LPC1769 o usar un
emulador adecuado.

## 7. Comprobación de C0

Partí del `main` mínimo y realizá tres cambios separados:

1. imprimí un segundo mensaje y comprobá el orden;
2. provocá deliberadamente un warning por variable no usada, leelo y corregilo;
3. compilá con `-S` y abrí el `.s` generado para reconocer que el resultado ya no es C.

El objetivo no es entender todavía el ensamblador, sino poder describir la cadena:

```text
fuente .c → preprocesado → ensamblador → objeto .o → ejecutable/firmware .elf
```

## Fuentes y para seguir leyendo

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf),
  cláusula 5.1.1 (traducción) y 5.1.2 (entornos de ejecución).
- [GCC: Overall Options](https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html), para `-E`, `-S`,
  `-c` y las etapas del proceso.
- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html), definición y
  alcance de `-Wall`, `-Wextra` y `-Wpedantic`.
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., capítulo 1.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Siguiente:** [C1 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md)
