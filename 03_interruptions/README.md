 

# Interrupciones en el LPC1769

## Características

* El controlador de interrupciones anidadas (NVIC), es parte integral del ARM Cortex-M3 (core), no es un periférico y al estar acoplado estrechamente al core, permite **baja latencia**.

* Controla excepciones del sistema (software) e interrupciones de periféricos (hardware).
* En el LPC176x/5x, el NVIC soporta **35 interrupciones vectorizadas** .
* **32 niveles de prioridad programables**, con enmascaramiento de prioridad por hardware. 
* **Tabla de vectores reubicable**.
* Soporta una interrupción no enmascarable (**NMI**), que no se puede desactivar por software.
* Permite **generación de interrupciones por software**.
  
## Fuentes de interrupción


Cada periférico puede tener uno o varios flags que disparan la misma línea de interrupción. Ejemplos:

* Timer0 (IRQ ID 1, excepción 17): interrupción puede ser por Match0, Match1, Capture0, Capture1.

* UART0 (IRQ ID 5, excepción 21): interrupción por datos recibidos (RDA), transmisión vacía (THRE), timeout, etc.

> Es importante aclarar que un mismo ID de interrupción puede tener varias causas, y dentro de la ISR hay que leer los flags del periférico para saber qué pasó.
 
 
## Interrupción no enmascarable (NMI)

* El NVIC además maneja la **interrupción no enmascarable (NMI)**.
* Para que la NMI opere desde una señal externa, debe conectarse al pin correspondiente (P2.10 / EINT0n / NMI).
* Al detectar un “1” lógico en este pin, se genera la interrupción NMI.

---

## Reubicación de la tabla de vectores

En memoria existe una tabla de vectores: cada entrada es la dirección de la rutina de servicio (ISR).

Por defecto, la tabla está en Flash, pero se puede remapear a RAM (usando el registro VTOR – Vector Table Offset Register).


* La tabla de vectores puede ubicarse en cualquier parte del primer **1 GB de espacio de direcciones** del Cortex-M3.
* Debe estar alineada en una frontera de **256 palabras (1024 bytes)**.
* En LPC176x/5x se recomienda respetar esta alineación.

El **bit 29 del VTOR (TBLOFF)** selecciona la región de memoria:

* `0` → espacio de código (Flash).
* `1` → espacio SRAM.

### Ejemplos:

* Para colocar la tabla al inicio de la RAM local (`0x1000 0000`):
  Escribir `0x1000 0000` en el registro VTOR.

* Para colocarla al inicio de la RAM AHB (`0x2007 C000`):
  Escribir `0x2007 C000` en el registro VTOR.


# Principales registros del NVIC

El NVIC en el LPC176x/5x dispone de varios registros que permiten **habilitar, deshabilitar, poner en pendiente, limpiar y leer el estado** de las interrupciones.

## Registros principales (Tabla 51)

* **ISER0 – ISER1 (Interrupt Set-Enable Registers)** - `0xE000E100` y `0xE000E104`

  Permiten habilitar interrupciones.

  * Escribir un `1` en el bit correspondiente **habilita** esa interrupción.
  * Leer el bit indica si la interrupción está habilitada (1) o no (0).

* **ICER0 – ICER1 (Interrupt Clear-Enable Registers)** - `0xE000E180` y `0xE000E184`

  Permiten deshabilitar interrupciones.

  * Escribir un `1` en el bit correspondiente **deshabilita** esa interrupción.
  * Leer indica el estado actual (0 = habilitada, 1 = deshabilitada).

* **ISPR0 – ISPR1 (Interrupt Set-Pending Registers)** - `0xE000E200` y `0xE000E204`

  Permiten **forzar** que una interrupción quede en estado pendiente.

  * Escribir un `1` pone la interrupción en “pendiente”.
  * Leer indica si está pendiente (1) o no (0).

* **ICPR0 – ICPR1 (Interrupt Clear-Pending Registers)** - `0xE000E280` y `0xE000E284`

  Permiten limpiar el estado de pendiente de una interrupción.

  * Escribir un `1` la borra del estado pendiente.
  * Leer indica si está pendiente (1) o no (0).

* **IABR0 – IABR1 (Interrupt Active Bit Registers)** - `0xE000E300` y `0xE000E304`

  Permiten leer si una interrupción está **activa en ese momento**.

  * Solo lectura.

* **IPR0 – IPR8 (Interrupt Priority Registers)** - `0xE000E400`, `0xE000E404`, ... , `0xE000E420`

  Nueve registros que permiten asignar prioridad a las interrupciones.

  * Cada registro controla 4 interrupciones.
  * Cada campo tiene 5 bits para definir la prioridad (0 = mayor prioridad, 31 = menor prioridad).

* **STIR (Software Trigger Interrupt Register)** - `0xE000 EF00`

  Permite **disparar una interrupción por software**, escribiendo el número de interrupción en este registro.
  

* Solo se aplica a interrupciones de periféricos (no a excepciones del sistema).
* Campo principal:

  * **INTID (bits 8:0)** → número de la interrupción a disparar (0 a 111 en LPC176x/5x).
* Bits 31:9 → reservados.

> Por defecto, solo el software privilegiado puede escribir en STIR.
> Se puede habilitar acceso desde software no privilegiado si se configura el bit correspondiente en el registro CCR (Control Configuration Register).  
 


#  NVIC y CMSIS

El **NVIC** puede manejar hasta 112 interrupciones, con **prioridades programables de 0 a 31** (0 = mayor prioridad).
Además soporta interrupciones **sensibles a nivel** y a **pulso**, interrupción no enmascarable (NMI), y permite **reorganizar prioridades dinámicamente**.

Cuando ocurre una excepción, el Cortex-M3 guarda automáticamente el contexto en la pila (stacking) y lo restaura al salir (unstacking), sin necesidad de código extra → lo que asegura baja latencia.

---

## Mapeo en CMSIS

Para simplificar el acceso, CMSIS presenta los registros del NVIC como **arreglos**:

* `ISER[x]` → Set Enable (habilitar).
* `ICER[x]` → Clear Enable (deshabilitar).
* `ISPR[x]` → Set Pending (forzar pendiente).
* `ICPR[x]` → Clear Pending (limpiar pendiente).
* `IABR[x]` → Active Bit (indica si está activa).
* `IP[n]` → Prioridad (un byte por interrupción, hasta 112).

 Ejemplo: `NVIC->ISER[0] = (1 << 5);` habilita la interrupción de UART0.

En el CMSIS tenemos arrays de 8 elementos de 32 bits, pero solo usamos 2 para el LPC1769.

## Registros básicos (en CMSIS)

* **ISER (Set-enable)**

  * Escribir `1` habilita la interrupción.
  * Leer devuelve 0 = deshabilitada, 1 = habilitada.

* **ICER (Clear-enable)**

  * Escribir `1` deshabilita la interrupción.

* **ISPR (Set-pending)**

  * Escribir `1` la marca como pendiente.
  * Leer devuelve 0 = no pendiente, 1 = pendiente.

* **ICPR (Clear-pending)**

  * Escribir `1` limpia el estado pendiente.

* **IABR (Active Bit)**

  * Solo lectura. 0 = no activa, 1 = activa.

* **IPR (Interrupt Priority Registers)**

  * 5 bits por interrupción.
  * 0 = mayor prioridad, 31 = menor prioridad.

 

## Para presta atención

* Una interrupción puede ponerse en pendiente aunque esté deshabilitada.
* Antes de reubicar la tabla de vectores (VTOR), asegurarse de que contiene correctamente los punteros a manejadores de fallas, NMI y excepciones habilitadas.

---

## Funciones de CMSIS

CMSIS provee funciones que encapsulan el acceso al NVIC.

### Intrinsics

```c
void __disable_irq(void);   // Deshabilita todas las IRQ
void __enable_irq(void);    // Habilita todas las IRQ
```

### Funciones NVIC

* `NVIC_EnableIRQ(IRQn_Type IRQn)` → Habilita una interrupción.
* `NVIC_DisableIRQ(IRQn_Type IRQn)` → Deshabilita una interrupción.
* `NVIC_GetPendingIRQ(IRQn_Type IRQn)` → Consulta si está pendiente.
* `NVIC_SetPendingIRQ(IRQn_Type IRQn)` → Fuerza a pendiente.
* `NVIC_ClearPendingIRQ(IRQn_Type IRQn)` → Limpia el estado pendiente.
* `NVIC_GetActive(IRQn_Type IRQn)` → Devuelve si está activa.
* `NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)` → Asigna prioridad.
* `NVIC_GetPriority(IRQn_Type IRQn)` → Consulta prioridad.
* `NVIC_SystemReset(void)` → Fuerza un reset del sistema. 

### Handlers

Luego el archivo `startup_LPC17xx.c` del proyecto, tiene handlers definidos debidamente para cada interrupción (apuntando a un loop infinito por defecto). 

Para controlar como se comporta cada interrupción, se debe definir el handler fuentemente en nuestro programa.


Ejemplos de handlers:

* `void TIMER0_IRQHandler(void)`
* `void UART0_IRQHandler(void)`
* `void EINT0_IRQHandler(void)`


##  Comportamiento de interrupciones
 
 Una interrupción puede quedar pendiente por:

  * Señal alta detectada.
  * Flanco de subida.
  * Escritura en ISPR o STIR.

  Se limpia cuando:

  * El procesador entra al ISR.
  * Se escribe en ICPR.

 
## Estados de una interrupción

Cuando decimos que una interrupción está **pendiente**, significa que el microcontrolador ya detectó que **alguien pidió atención** (por hardware o software) y la tiene en cola para ejecutarla.
No necesariamente entra al *handler* en ese mismo instante, porque puede estar atendiendo otra IRQ de mayor prioridad o porque está deshabilitada.

---

## Qué pasa con señales “por nivel”

Imaginemos un botón que al presionarlo mantiene un pin en nivel **alto (1 lógico)**:

1. El NVIC ve ese 1 lógico → la marca como **pendiente**.
2. El procesador entra al *handler* (ISR).
3. Apenas entra, el NVIC borra el estado *pendiente* y lo pasa a **activo** (está ejecutando).
4. Cuando el ISR termina, el NVIC vuelve a mirar la señal:

   * Si el pin **sigue en 1** (porque el botón sigue presionado, o el periférico no limpió el flag), entonces **otra vez la marca como pendiente** → y el procesador volverá a entrar.
   * Si el pin ya volvió a 0 (se liberó el botón o el periférico limpió el flag), entonces queda **inactiva** y el programa principal sigue normalmente.

 > O sea: con interrupciones por nivel, si no se “atiende” la causa en el periférico, el micro queda entrando una y otra vez al handler, casi sin ejecutar el programa principal.

---

## Qué pasa con señales “por pulso”

En este caso el periférico no mantiene el nivel, sino que genera un pulso corto (un flanco).

* El NVIC lo detecta y marca la interrupción como **pendiente**.
* El procesador atiende el *handler*.
* Como el pulso ya pasó, al salir del ISR la señal ya no está activa → la interrupción no vuelve a dispararse.

--- 

* **Pendiente** = “el micro se enteró que tiene que atender algo, lo tiene en lista de espera”.
* **Activo** = “está dentro del handler de esa interrupción”.
* **Inactivo** = “no hay nada que atender”. 
---



Ejemplo de configuracion con punteros:

En este ejemplo se configura la prioridad de la interrupción del WDT (Watchdog Timer) a 10.
Cada vez que se interrumpa el WDT, se incrementa el contador_irq.


```c

#include "LPC17xx.h"    
volatile uint32_t contador_irq = 0;

/* ISR con el nombre estándar */
void WDT_IRQHandler(void) {
    contador_irq++;          // trabajo mínimo
    /* Nada que limpiar en periférico porque usamos disparo por software */
}

int main(void) {
    /* === 1) PRIORIDAD ============================================
       WDT está en IPR0, campo bits [7:3] del primer byte (IP[?]).
       Con CMSIS de registros: NVIC->IP[...]     */

    /* Prioridad = 10 (0 = más alta, 31 = más baja) */
    /* Para WDT (IRQ 0), el byte de prioridad es IP[0]. 
       Según tu tabla, el HW usa los bits [7:3]; los [2:0] se leen en 0. */
    NVIC->IP[0] = (10 & 0x1F) << 3;

    /* === 2) LIMPIAR PENDING PREVIO ============================== */
    /* WDT está en el grupo 0 (bits 0..31) => ICPR[0] 
    con EL "u" lo decimos al compilador que es un unsigned int */
    NVIC->ICPR[0] = (1u << 0);

    /* === 3) HABILITAR LA IRQ ==================================== */
    NVIC->ISER[0] = (1u << 0);

    // A partir de aca, el programa principal ya puede interrumpirse por el WDT


    /* === 4) DISPARAR POR SOFTWARE =============================== */
    /* Opción A: usando STIR (INTID = 0 para WDT) */
    SCB->STIR = 0u;

    /* Opción B (equivalente): forzar pending con ISPR */
    // NVIC->ISPR[0] = (1u << 0);





    while (1) {
        __NOP();
    }
}

```


Ejemplo con GPIO:
