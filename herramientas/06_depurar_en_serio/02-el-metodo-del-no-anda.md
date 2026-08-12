# El método del "no anda"

Los mensajes muestran solamente lo que decidiste imprimir y, además, pueden alterar los tiempos.
El **debugger por hardware** permite detener el núcleo e inspeccionar su estado sin agregar
instrumentación al firmware. Tampoco es transparente: al frenarlo cambia la relación temporal entre
el CPU, los periféricos y el mundo exterior.

Esta página es sobre cómo usarlo bien. Cómo funciona por dentro está en
[02 - El debug adentro del Cortex-M3](../02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md);
acá damos por sabido que funciona.

---

## Qué te da el debugger

Con la sonda conectada y una sesión abierta (`make debug`, o F5 en VSCode, o el botón de
MCUXpresso) tenés cinco cosas:

| Herramienta | Qué hace | Cuántas hay |
|---|---|---|
| **Breakpoints** | detenerse al llegar a una instrucción | hasta **6** comparadores de hardware en el LPC1769 |
| **Watchpoints** | detenerse al leer o escribir una dirección | hasta **4** comparadores DWT en el LPC1769 |
| **Paso a paso** | avanzar una instrucción o una línea de código fuente | no consume comparadores adicionales |
| **Inspección de variables** | ver valores cuando el núcleo está detenido | limitada por la información de depuración y la optimización |
| **Memoria y registros** | leer el estado de RAM y de los periféricos | algunos registros cambian al leerlos |

Los límites dependen del núcleo y de cómo el IDE implemente cada punto. En FLASH suelen usarse
breakpoints de hardware; en RAM también pueden insertarse instrucciones de breakpoint. Una línea de C
puede corresponder a varias instrucciones, y con optimización el paso a paso puede no seguir el orden
que sugiere el código fuente.

### Por qué "ver los registros en vivo" cambia todo

El bug más común en embebidos es **configuraste algo distinto de lo que pensás**. Imprimir no
siempre lo revela: si el `printf` imprime la variable con la que **creés** que configuraste el
periférico, te va a confirmar tu error.

El debugger permite detenerse después de `init()` y comparar, registro por registro, lo que
esperabas con lo que quedó configurado. Por ejemplo: ¿`PCONP` tiene habilitado el timer?
¿`PINSEL` seleccionó la función correcta? Tené presente que ciertos registros se limpian al leerlos
o disparan acciones; el manual del micro indica cuáles se pueden observar sin efectos secundarios.

Esta inspección suele separar rápido un problema de configuración de uno que aparece más adelante.

---

## El checklist del "no anda"

Cuando un periférico no responde, evitá cambiar registros al azar. En el LPC1769 conviene revisar
este orden; en otro micro, buscá los registros equivalentes en su manual:

1. **¿Está alimentado y habilitado?**
   En el LPC1769, mirá su bit en `PCONP`
   ([módulo 3](../../curso/03_clock_y_power/)). En otros micros puede haber varios dominios de
   alimentación, clock o reset.

2. **¿Tiene clock?**
   Revisá `PCLKSEL` y **recalculá tus tiempos con el `PCLK` real**, no con el que asumiste.

3. **¿Los pines están bien?**
   Revisá `PINSEL` y `PINMODE` ([módulo 4](../../curso/04_pinsel/)). Comprobá la función
   alternativa, el modo eléctrico y la conexión física.

4. **¿Atendés y limpiás correctamente la interrupción?**
   La forma de reconocer y limpiar una bandera depende del periférico
   ([módulo 7](../../curso/07_interrupciones/)). Hacerlo mal puede provocar reingresos continuos o
   perder eventos.

5. **¿La comunicación entre la ISR y el `main` es correcta?**
   `volatile` evita ciertas optimizaciones, pero no vuelve atómico un acceso ni reemplaza un
   mecanismo de sincronización
   ([módulo 0, cap. 12](../../curso/00_lenguaje_c/12-volatile-y-tipos-para-hardware.md)).

6. **¿La cuenta de tiempo o de baudrate da bien?**
   Rehacé el cálculo con el `PCLK` correcto.

Este checklist cubre fallas frecuentes de configuración y ayuda a descartarlas de manera
sistemática.

---

## Un método, para cuando el checklist no alcanza

El checklist cubre los problemas de configuración. Para el resto, sirve trabajar como un
investigador y no como alguien que prueba cosas:

**1. Reproducilo.** Si el bug aparece "a veces", registrá qué entradas, tiempos y estado previo
lo acompañan. Una prueba repetible permite comprobar después si el arreglo funciona.

**2. Achicá el problema.** Construí un caso mínimo o deshabilitá módulos de a uno. Después
reincorporalos gradualmente. Cada cambio puede modificar los tiempos o esconder el defecto, así que
conservá también una forma de reproducir el caso original.

**3. Verificá tus supuestos, uno por uno.** "El timer está andando", "la interrupción entra",
"el valor llega bien". Cada afirmación se puede **comprobar** con una medición, un breakpoint o la lectura de un
registro. Cuando una no se cumple, ya acotaste el problema.

**4. Cambiá una cosa por vez.** Si cambiás tres y funciona, no sabés cuál era.

**5. Anotá lo que ya descartaste.** A la media hora ya no te acordás.

**6. Si hace media hora que no avanzás, cambiá de instrumento.** Si venís con el debugger,
poné un LED o un osciloscopio; si venís con prints, frená y mirá los registros. Un
instrumento distinto ve cosas distintas.

Y el punto que más ahorra: **si el software parece imposible, dudá del hardware.** Un cable
suelto, una masa que no es común, una fuente que cae. El
[módulo 17](../../curso/17_hardware_y_placa/03-instrumentos-de-medicion.md) es sobre eso.

---

## Detalles que ahorran sorpresas

Detener el núcleo **no implica que todo el chip se detenga**
([detalle completo](../02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md)):

- En el LPC1769, SysTick y RIT se detienen durante el halt, pero otros periféricos pueden seguir
  activos. Una UART puede recibir datos y desbordarse mientras avanzás paso a paso. Verificá el
  comportamiento de cada periférico en el manual del micro.
- Los modos Deep Sleep y Power-down del LPC1769 tienen restricciones durante una sesión de debug.
  Para diagnosticar bajo consumo, primero confirmá el flujo en un modo que mantenga accesible el
  puerto de depuración.
- La sonda y el estado de debug pueden cambiar el consumo y evitar ciertos modos de bajo consumo.
  Medí el equipo final sin la sesión conectada.
- La protección de lectura puede limitar o deshabilitar el acceso de debug según el dispositivo y
  el nivel seleccionado.

---

## La caja de herramientas completa

| Herramienta | Cuándo | Costo |
|---|---|---|
| **LED** | confirmar etapas de ejecución | un GPIO, tiempo de CPU y una señal visible |
| **UART / `printf`** | observar valores y flujo durante la ejecución | una UART, pines, memoria y tiempo |
| **[RTT](./04-consola-por-el-debugger-rtt.md)** | enviar texto por la interfaz de debug | RAM, tiempo de CPU y una sonda compatible |
| **Debugger** | detener el núcleo e inspeccionar estado | una sonda; altera el comportamiento temporal |
| **Watchpoints** | detectar quién accede a una variable o dirección | un comparador DWT |
| **[`DWT->CYCCNT`](../02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md)** | medir ciclos de CPU | configuración y lecturas de instrumentación |
| **Osciloscopio / analizador lógico** | observar señales y tiempos fuera del CPU | el instrumento y puntos de medición |
| **Checklist** | ordenar el diagnóstico | tiempo para comprobar cada supuesto |

Saber depurar no es un tema aparte: **es lo que te permite usar de verdad todo lo demás.**
Cuando algo no sale, volvé acá.

---

**Depurar en serio:** [índice](./README.md) ·
**Anterior:** [01 - Imprimir para depurar](./01-imprimir-para-depurar.md) ·
**Siguiente:** [03 - Hard faults](./03-hard-faults.md)
