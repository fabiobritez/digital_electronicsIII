# El debugger y un método para depurar

Imprimir está bueno, pero tiene un límite: solo ves lo que decidiste imprimir, y agregar prints
cambia los tiempos. El **debugger por hardware** te deja **parar el micro y mirar todo**, sin tocar el
código.

## El debugger JTAG/SWD

El Cortex-M3 tiene hardware de depuración integrado, accesible por dos interfaces: **JTAG** (clásica)
y **SWD** (Serial Wire Debug, solo 2 pines: SWDCLK y SWDIO, más un tercero opcional, **SWO**, por el
que el núcleo puede sacar datos de traza). En las placas de la materia, el LPC-Link / depurador de
MCUXpresso usa SWD. Con él, desde MCUXpresso podés:

- **Breakpoints:** marcar una línea y que la ejecución se **frene** ahí. El micro queda detenido,
  esperándote. El hardware trae 6 breakpoints de instrucción, así que en flash no podés dejar
  activos más que esos.
- **Watchpoints:** frenar cuando el programa **lee o escribe una dirección**, por ejemplo, una
  variable que aparece pisada y no sabés quién la pisa. Hay 4 watchpoints de datos.
- **Paso a paso:** ejecutar una línea por vez (*step over* salta por encima de las funciones, *step
  into* entra en ellas).
- **Inspeccionar variables:** ver el valor de cualquier variable en el momento en que el programa está
  frenado.
- **Ver la memoria y los registros del periférico en vivo.** Esto es oro en esta materia: abrís la
  vista de "Peripherals" / "Registers" y ves el valor **real** de `PCONP`, `PINSEL0`, `ADCR`,
  `T0TCR`, etc.: lo que de verdad quedó configurado, no lo que vos creés.

> El SWD/JTAG es hardware del núcleo (Capítulos 33 y 34 del manual). Lee la memoria sin que tu
> programa tenga que hacer nada: por eso podés inspeccionar un micro incluso "colgado".

Dos detalles del Capítulo 33 que ahorran sorpresas:

- Con el CPU frenado, el **SysTick y el RIT se frenan automáticamente**; el resto de los periféricos
  **sigue corriendo** (un timer sigue contando, la UART sigue recibiendo). Por eso, al ir paso a
  paso, los tiempos relativos entre tu código y los periféricos no se conservan.
- Si activás la **protección de lectura de código** (CRP), el debug queda deshabilitado.

### Por qué "ver los registros en vivo" cambia todo

El bug más común en embebidos es **configuraste algo distinto de lo que pensás**. Imprimir no siempre
lo revela. El debugger sí: parás el programa después de tu `init()` y comparás registro por registro
lo que *querías* con lo que *quedó*. ¿`PCONP` tiene el bit del timer en 1? ¿`PINSEL` quedó con la
función correcta? En treinta segundos sabés si el problema está en la configuración o más adelante.

## Un método ordenado: el checklist del "no anda"

Cuando un periférico no responde, no toquetees al azar. Andá en este orden: el 90% de los casos está
en los primeros tres puntos:

1. **¿Está encendido?** Mirá su bit en `PCONP` con el debugger (módulo 3). Es la causa #1.
2. **¿Tiene clock?** Revisá `PCLKSEL` y **recalculá tus tiempos** con el `PCLK` real, no el asumido.
3. **¿Los pines están bien?** Revisá `PINSEL`/`PINMODE` (módulo 4). ¿La función es la correcta?
   ¿La resistencia (pull-up / tri-state / open-drain) es la que corresponde?
4. **¿Limpiás la bandera de interrupción?** Si no (módulo 7), reentrás al handler para siempre y el
   `main` parece congelado.
5. **¿La variable compartida con la ISR es `volatile`?** Si no (módulo 0, cap. 08), el `main` puede no
   "ver" los cambios que hace la interrupción.
6. **¿La cuenta de tiempo/baudrate da bien?** Rehacé el cálculo con el `PCLK` correcto.

Tener este checklist a mano evita horas de prueba y error. La mayoría de los "misterios" de la
materia son uno de estos seis puntos.

## Hard fault: cuando el micro "se muere"

Si el programa salta a `HardFault_Handler` (un handler por defecto que suele ser un `while(1)`),
algo grave pasó: típicamente un **puntero inválido** que tocó una dirección inexistente, un
*stack overflow*, o una instrucción ilegal. En rigor el Cortex-M3 distingue varios tipos de fault
(*bus fault*, *usage fault*, *memory management fault*), pero como sus handlers no vienen
habilitados, todos **escalan a hard fault** y caen en el mismo lugar (sección 34.3.4.2 del manual).

Un detalle que confunde: en este micro, **leer** a través de un puntero `NULL` **no falla**: en la
dirección 0 está la tabla de vectores, en flash, así que devuelve basura en silencio. Lo que sí
suele terminar en fault es **escribir** por ese puntero, o leer lejos de cero (`p->campo` con `p`
basura y el campo a cierto offset).

¿Cómo encontrás dónde fue? Al entrar a cualquier excepción, el micro **apila el PC** del programa
interrumpido, junto con R0–R3, R12, LR y el PSR (sección 34.3.3.7.1): por eso, frenado dentro del
`HardFault_Handler`, el *call stack* del debugger te muestra dónde estaba el programa (la
instrucción que falló, o la siguiente, según el tipo de fault). Y para el
diagnóstico fino, el núcleo tiene registros que dicen **qué** pasó y **dónde** (secciones 34.4.3.11
a 34.4.3.14):

- `HFSR`: si el bit FORCED está en 1, fue un fault de otro tipo escalado: mirá el `CFSR`.
- `CFSR`: la causa concreta (acceso a memoria inválido, instrucción indefinida, división por
  cero si la activaste, etc.).
- `BFAR` / `MMFAR`: la dirección que falló, cuando su bit de validez está en 1.

En MCUXpresso los ves en la vista de registros; las versiones recientes muestran además una vista
de "Faults" con todo esto ya decodificado. Causas frecuentes en la materia:

- Usar un periférico **sin encenderlo**: el manual solo garantiza lecturas y escrituras válidas con
  el periférico habilitado en `PCONP` (sección 4.8.9); en la práctica leés basura o directamente
  hard fault.
- Un arreglo accedido fuera de rango que pisó memoria (acá brillan los watchpoints).
- Un puntero mal calculado (volvé al módulo 1).

## Resumen de la caja de herramientas

| Herramienta | Cuándo | Costo |
|-------------|--------|-------|
| LED | primer diagnóstico, antes de tener UART | cero |
| UART / `printf` / debug_frmwrk | ver valores y flujo durante la ejecución | una UART |
| Debugger SWD/JTAG | parar, paso a paso, ver registros y memoria en vivo | el depurador (LPC-Link) |
| Checklist del "no anda" | siempre, antes de desesperar | cero |

Saber depurar no es un tema aparte: es lo que te permite **usar de verdad** todo lo de los módulos
anteriores. Cuando algo no sale, volvé acá.

---

**Anterior:** [01 - Imprimir para depurar](./01-imprimir-para-depurar.md) ·
**Volver al** [índice del curso](../README.md)
