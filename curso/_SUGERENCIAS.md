# Sugerencias: temas faltantes o a profundizar

Revisión crítica del curso desde la óptica de **alguien que recién aprende**. El curso cubre muy bien
el camino registro→driver de los periféricos. Lo que sigue son huecos que, llenados, lo harían más
completo y más cercano a "saber hacer un proyecto de verdad". Está priorizado: arriba lo que más
impacto pedagógico tiene.

> **Estado:** **TODAS IMPLEMENTADAS** (marcado en cada título): A1 (mód. 16), A2 (mód. 17),
> A3 (mód. 7 pág. 03), A4 (mód. 19 PWM), A5 (mód. 3 pág. 04), B1 y B2 (mód. 0 caps. 11–12),
> B3 (mód. 10 pág. 03), B4 (mód. 5 pág. 03), B5 (mód. 1 pág. 04), C1/C2/C3 (mód. 20), C4 (mód. 18),
> y el grupo D completo en el **módulo 21** (RTC, WDT, QEI, CAN, I2S, Ethernet).

---

## A. Temas nuevos que faltan (alto valor)

### A1. El proceso de build: linker script, startup y mapa de memoria  [IMPLEMENTADO -> modulo 16]
**Por qué importa:** el alumno usa MCUXpresso como caja negra. No sabe **cómo** su `main` termina en
la Flash, quién inicializa las variables globales, qué es el stack y dónde está, ni por qué existe la
tabla de vectores. Es la otra mitad del módulo 1 (ya explicamos que el programa arranca en 0x0, pero
no el cómo).
**Qué cubriría:** etapas de compilación aplicadas al micro (ya hay base en `00_lenguaje_c`), el
**linker script** (`.ld`): secciones `.text`/`.data`/`.bss`, qué va a Flash y qué a RAM; el **startup
code**: copia de `.data`, puesta a cero de `.bss`, `SystemInit`, salto a `main`; la **tabla de
vectores** en detalle. Conecta directo con los hard faults del módulo 12 (stack overflow).
**Manual:** complementa con el Cap. 34 (Cortex-M3).

### A2. Arquitectura de firmware: superloop, maquinas de estado y (luego) RTOS  [IMPLEMENTADO -> modulo 17]
**Por qué importa:** este es, quizás, **el hueco más grande**. El curso enseña a manejar cada
periférico, pero no a **estructurar un programa** que use varios a la vez sin volverse un caos de
`if` y flags globales. Es la diferencia entre "hacer parpadear un LED" y "hacer un proyecto".
**Qué cubriría:** el patrón *superloop* + interrupciones (lo que ya usan implícitamente); **máquinas
de estado** explícitas (con `enum` de estados y `switch`); por qué los delays bloqueantes son el
enemigo; cooperative scheduling simple (tareas con su propio tiempo); y una intro conceptual a un
**RTOS** (FreeRTOS): qué es una tarea, por qué a veces conviene. Sin esto, los proyectos integradores
salen frágiles.

### A3. Secciones críticas, atomicidad y `volatile`  [IMPLEMENTADO → módulo 7, pág. 03]
**Por qué importa:** ya se dice "usá `volatile`", pero no se explica el problema más sutil:
**condiciones de carrera** entre el `main` y una ISR cuando comparten una variable de varios bytes o
hacen lectura-modificación-escritura. Un alumno que comparte un `uint32_t` o una estructura entre ISR
y main va a tener bugs intermitentes imposibles de encontrar.
**Qué cubriría:** qué es atómico y qué no en Cortex-M3; `__disable_irq()`/`__enable_irq()` y secciones
críticas; por qué `volatile` no alcanza para atomicidad; el patrón "leer una vez, copiar, procesar".
Encaja como tercera página del [módulo 7](./07_interrupciones/).

### A4. PWM dedicado  [IMPLEMENTADO → módulo 19]
**Por qué importa:** el PWM se menciona en Timers pero no se desarrolla, y es de lo más usado:
controlar el brillo de un LED, la velocidad de un motor, la posición de un servo, audio simple. El
LPC1769 tiene un periférico PWM dedicado (PWM1) más capaz que el match externo del timer.
**Manual:** Cap. 24 ([`../manual/ch24_pulse-width-modulator.pdf`](../manual/ch24_pulse-width-modulator.pdf)).
Ya está el PDF dividido.

### A5. Modos de bajo consumo (Sleep / Deep Sleep / Power-down)  [IMPLEMENTADO → módulo 3, pág. 04]
**Por qué importa:** todo dispositivo a batería los necesita, y el curso solo los menciona al pasar en
el módulo 3. Cierra el tema "Power" que quedó a medias.
**Qué cubriría:** los modos, cómo entrar (`__WFI()`), cómo despertar (interrupción, wake-up timer),
qué se apaga en cada uno, el trade-off consumo/latencia. Encaja como página extra del
[módulo 3](./03_clock_y_power/) o del 7.

---

## B. Temas existentes que conviene profundizar

### B1. C embebido: `static`, `const`, `inline`, *storage classes* y *bitfields*  [IMPLEMENTADO → módulo 0, cap. 11]
El módulo 0 (heredado) cubre lo básico, pero para registros y buen estilo embebido faltan:
**`static`** (variables persistentes y funciones privadas de archivo), **`const`** aplicado a fondo,
**`inline`/`static inline`** (clave para drivers rápidos), las *storage classes*, y los **campos de
bits** (`struct { uint32_t en:1; }`) como alternativa a las máscaras. Sería un capítulo extra en
`00_lenguaje_c` o ampliar el cap. 07 y 08.

### B2. Punto fijo vs punto flotante (el Cortex-M3 no tiene FPU)  [IMPLEMENTADO → módulo 0, cap. 12]
El LPC1769 **no tiene unidad de punto flotante**: cada `float` se emula por software y es lento. El
curso usa `float` en ejemplos (ADC→tensión, temperatura) sin advertirlo. Un alumno debería saber
**cuándo evitar `float`** y la idea de aritmética de punto fijo (enteros escalados). Media página en
el módulo 0 o en ADC/DAC.

### B3. ADC/DAC: nociones de señal (muestreo, Nyquist, aliasing)  [IMPLEMENTADO → módulo 10, pág. 03]
El módulo 10 enseña a leer el ADC, pero no **qué frecuencia de muestreo** elegir ni por qué. Para
cualquier aplicación de medición real (audio, sensores rápidos) falta la intuición de Nyquist y
aliasing, y por qué a veces hace falta un filtro antialiasing analógico. Media página conceptual.

### B4. Debounce y filtrado de entradas, bien hecho  [IMPLEMENTADO → módulo 5, pág. 03]
Se menciona en GPIO e interrupciones, pero merece un tratamiento propio: rebote mecánico, debounce por
tiempo (con SysTick) vs por conteo, filtro de software para entradas ruidosas. Es causa típica de
"anda en simulación pero no en la placa".

### B5. Bit-banding (característica del Cortex-M3)  [IMPLEMENTADO → módulo 1, pág. 04]
El material viejo de GPIO lo nombra pero no se enseña. Permite acceso **atómico** a bits individuales
mapeándolos a direcciones propias. Encaja como nota avanzada en el módulo 1 o 5.

---

## C. Hardware, herramientas y método (transversal, muy valioso para principiantes)

### C1. Leer el esquematico y la placa  [IMPLEMENTADO -> modulo 20, pag. 01]
Un hueco práctico enorme: el alumno no sabe **qué pin físico** corresponde a `P0.22`, cómo está
cableado el LED o el botón de su placa, ni cómo leer el esquemático. Una guía de "tu placa por dentro"
(con el esquemático de la placa que usan) evitaría muchísima frustración inicial.

### C2. Electrónica mínima para no romper el micro  [IMPLEMENTADO → módulo 20, pág. 02]
Niveles lógicos de 3.3 V (¡no son 5 V!), **corriente máxima por pin** (~4 mA típico, no podés manejar
un motor directo), por qué un LED necesita resistencia, pull-ups/pull-downs externos, capacitores de
desacople. Conceptos de Electrónica que se dan por sabidos y a menudo no lo están.

### C3. Instrumentos de medicion  [IMPLEMENTADO -> modulo 20, pag. 03]
Cómo verificar que una señal **realmente** sale como esperás. Imprescindible para depurar I2C/SPI/UART
(un analizador lógico barato muestra las tramas y resuelve el 80% de los problemas de comunicación).
Complementa el [módulo 12](./12_debug/).

### C4. Manejo de proyecto / entorno de desarrollo  [IMPLEMENTADO → módulo 18]
Cubierto y **ampliado** por el módulo 18: además del flujo en MCUXpresso, se explica qué es el
toolchain, cómo armar un entorno propio en VSCode (con archivos de config listos) y cómo grabar/depurar
la placa **sin** MCUXpresso (OpenOCD, pyOCD, ISP serial, gdb). Incluye includePath de CMSIS, tareas de
build y errores típicos de compilación/linkeo.

---

## D. Periféricos restantes  [IMPLEMENTADO → módulo 21]

Cubiertos en el [módulo 21 (periféricos adicionales)](./21_perifericos_adicionales/), una página cada
uno con el mismo molde registro → driver y punteros al manual y a los ejemplos:
- **RTC** (Cap. 27): reloj de tiempo real, alarmas, mantener la hora con bajo consumo.
- **WDT / Watchdog** (Cap. 28): resetear un sistema colgado; clave para robustez. Corto y valioso.
- **QEI** (Cap. 26): encoders en cuadratura, para control de motores/posición.
- **CAN** (Cap. 16): bus robusto automotriz/industrial.
- **I2S** (Cap. 20): audio digital.
- **Ethernet** (Cap. 10): redes (avanzado, mucho trabajo).

---

## Prioridad sugerida (si hubiera que elegir)

Para el mayor impacto en alguien que aprende, en este orden:

1. **A2: Arquitectura de firmware** (cómo estructurar un programa real).
2. **A1: Build, linker y startup** (sacar la caja negra).
3. **A3: Secciones críticas / atomicidad** (cierra interrupciones).
4. **C1 + C2: Esquemático y electrónica mínima** (evita frustración y micros quemados).
5. **A4: PWM** y **A5: bajo consumo** (periféricos muy usados, ya casi listos).
6. El resto (B y D), según el tiempo y el enfoque de la cursada.

> Nota: A1, A2 y A3 no son periféricos, son **las ideas que convierten "saber usar periféricos" en
> "saber hacer firmware"**. Son las que más diferencia harían en los proyectos integradores y, hoy,
> son el principal hueco del material.
