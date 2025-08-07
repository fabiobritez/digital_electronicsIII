 

#   Introducción a C para Sistemas Embebidos

---

## 1. ¿Por qué aprender C hoy?

Aunque tiene más de 50 años, el lenguaje C sigue siendo **el estándar de facto** para programar sistemas embebidos. ¿Por qué?
 
* Es **compacto y rápido**
* Permite **acceso directo al hardware**
* No tiene sobrecarga innecesaria (como lenguajes de más alto nivel) 
* Tiene mucho soporte y portabilidad para plataformas embebidas (ARM, AVR, RISC-V...)

 

C se usa en todos tipo de dispositivos: Microcontroladores de lavadoras, routers, teclados, drones, controladores de motores, etc.

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

### 🔎 ¿Qué vemos acá?

* `#include <stdio.h>`: **directiva de preprocesador** que indica al compilador incluir código de otra parte (en este caso, para usar `printf`)
* `main()`: es la función donde **empieza la ejecución**
* `return 0;`: devuelve al sistema operativo un código indicando éxito

Este es un programa **mínimo** válido en C.

---

## 4. ¿Cómo se compila un programa en C?

La compilación de un programa en C tiene varias **etapas automáticas**:

### 🔧 Etapas del proceso de construcción

```
main.c ──▶ [Preprocesador] ─▶ main.i
        ──▶ [Compilador]     ─▶ main.s
        ──▶ [Assembler]      ─▶ main.o
        ──▶ [Linker]         ─▶ main.elf / main.exe
```

### 🔍 ¿Qué hace cada etapa?

1. **Preprocesador (`#`)**

   * Sustituye macros, incluye archivos (`#include`, `#define`)
   * Elimina comentarios
   * Resultado: código expandido (`.i`)
Más detalles en [2-preprocesador.md](./2-preprocesador.md)
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

## 7. Compilación simple en Linux

Para compilar un programa C en terminal:

```bash
gcc -o programa main.c
./programa
```

Si usás un entorno embebido (como STM32, LPC, AVR, etc.), se usa un **toolchain cruzado**, como:

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -o main.elf main.c
```

Y luego se sube al microcontrolador con una herramienta como OpenOCD, STLink, etc.

---

## 8. Clases:

Para profundizar en C, se recomienda leer los siguientes apuntes:

* [0-declaraciones.md](./0-declaraciones.md)
* [1-operadores.md](./1-operadores.md)
* [2-preprocesador.md](./2-preprocesador.md) 
* ...