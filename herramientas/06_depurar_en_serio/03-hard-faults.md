# Hard faults: cuando el micro "se muere"

El programa andaba y de golpe no hace nada. El LED quedó fijo, la UART no manda, y no
responde a nada. Si frenás con el debugger y mirás dónde está, aparece parado adentro de
`HardFault_Handler`, que suele ser un `while(1)` vacío.

Esta página es sobre cómo pasar de "se colgó" a "se colgó **en esta línea, por esto**".

---

## Qué es un fault

El Cortex-M3 tiene excepciones que se disparan cuando el CPU intenta algo que no puede hacer.
En rigor son cuatro tipos distintos:

| Excepción | Se dispara cuando |
|---|---|
| **MemManage** | violaste una regla de la MPU (si la usás) |
| **BusFault** | una transferencia por el bus informó un error |
| **UsageFault** | instrucción indefinida o estado ilegal; también ciertos accesos no alineados y divisiones por cero si están configurados para generar fault |
| **HardFault** | la excepción de último recurso |

Un detalle clave es que **los tres primeros vienen deshabilitados por defecto**. Si no los habilitaste explícitamente en el registro `SHCSR`, cualquiera de
ellos **escala a HardFault** y todos terminan en el mismo `while(1)`
(UM10360 §34.3.4.2).

Por eso `HardFault` no alcanza como diagnóstico: puede ser una excepción escalada o un fallo
generado directamente como HardFault.

---

## Causas frecuentes

**1. Acceder a un periférico que no está habilitado.** En el LPC1769, el manual solo garantiza
accesos válidos cuando el periférico correspondiente está habilitado en `PCONP` (§4.8.9). Según
el bloque y el micro, un acceso incorrecto puede devolver un valor inesperado, no tener efecto o
generar un fault. Es el primer punto del
[checklist del "no anda"](./02-el-metodo-del-no-anda.md).

**2. Un puntero mal calculado o sin inicializar.** Escribir por un puntero que apunta a una
dirección que no existe.

**3. Un arreglo accedido fuera de rango**, que pisó otra cosa. Acá brillan los watchpoints.

**4. Stack overflow.** El stack crece hacia abajo y se choca con las variables globales, o
directamente se sale de la RAM. Típico de una función recursiva, un arreglo grande declarado
adentro de una función, o un `printf` con `float` en un stack chico. La geometría de todo eso
está en
[05 - El linker script y el startup](../05_del_codigo_al_binario/02-linker-y-startup.md).

**5. Un salto a una dirección inválida:** un puntero a función corrupto, o una entrada de la
tabla de vectores mal armada.

### La trampa del puntero NULL

En C, desreferenciar un puntero `NULL` tiene comportamiento indefinido: el compilador puede
optimizar suponiendo que nunca ocurre. Además, el resultado físico depende del mapa de memoria.

En el LPC1769, la dirección 0 suele contener la tabla de vectores en FLASH. Una carga generada por el
compilador podría entonces devolver datos sin producir un fault inmediato. Eso no vuelve válido al
acceso: el programa ya está fuera de las reglas de C y el síntoma puede aparecer mucho después.
Una escritura en esa zona o un acceso a otra dirección inválida sí puede generar un BusFault.

---

## Cómo encontrar dónde fue

Acá está lo útil de la página. La clave es esta:

> Al entrar a **cualquier** excepción, el Cortex-M3 **apila** automáticamente el contexto del
> programa interrumpido: R0 a R3, R12, LR, **PC** y xPSR (UM10360 §34.3.3.7.1).

La dirección donde estaba el programa al producirse el fault **queda guardada en el stack**.

### La forma fácil: el call stack

Frenado dentro de `HardFault_Handler`, el debugger puede intentar reconstruir la pila. En VSCode y
MCUXpresso se consulta la vista **Call Stack**; en GDB:

```gdb
(gdb) bt
```

Si la información de depuración y la pila siguen íntegras, el resultado orienta hacia el sitio del
fallo. No siempre es exacto: algunos errores de bus son imprecisos y el PC apilado puede apuntar a
una instrucción posterior.

### Cuando el call stack no alcanza

La pila puede estar dañada, por ejemplo por un desborde, o no haber información suficiente para
reconstruirla. En ese caso conviene leer el marco apilado y los registros de diagnóstico.

| Registro | Qué dice |
|---|---|
| `HFSR` | si el bit `FORCED` está en 1, fue un fault de otro tipo que escaló: mirá el `CFSR` |
| `CFSR` | **la causa concreta**: acceso a memoria inválido, instrucción indefinida, no alineado, división por cero |
| `BFAR` | **la dirección** que causó el bus fault, si su bit de validez (`BFARVALID`) está en 1 |
| `MMFAR` | lo mismo para el memory manage fault |

Están en las secciones 34.4.3.11 a 34.4.3.14 del manual. En MCUXpresso se ven en la vista de
registros, y las versiones recientes traen una vista de "Faults" con todo decodificado. En gdb:

```gdb
(gdb) x/1xw 0xE000ED2C    # HFSR
(gdb) x/1xw 0xE000ED28    # CFSR
(gdb) x/1xw 0xE000ED38    # BFAR
```

### Un handler que te cuenta qué pasó

Lo más práctico es reemplazar el `while(1)` por algo que hable. La idea es tomar el marco
apilado y mirarlo:

```c
/* El handler real, que averigua de que stack venia y le pasa el marco al de C. */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4          \n"   /* el bit 2 del EXC_RETURN dice cual stack se uso */
        "ite eq              \n"
        "mrseq r0, msp       \n"
        "mrsne r0, psp       \n"
        "b hard_fault_report \n"
    );
}

/* Ahora si, en C, con el marco apilado como argumento. */
void hard_fault_report(uint32_t *marco)
{
    uint32_t pc  = marco[6];        /* donde estaba el programa */
    uint32_t lr  = marco[5];        /* quien lo habia llamado */
    uint32_t psr = marco[7];

    uint32_t cfsr = *(volatile uint32_t *)0xE000ED28;
    uint32_t bfar = *(volatile uint32_t *)0xE000ED38;

    printf("HARD FAULT\r\n");
    printf("  PC   = 0x%08lX\r\n", (unsigned long)pc);
    printf("  LR   = 0x%08lX\r\n", (unsigned long)lr);
    printf("  PSR  = 0x%08lX\r\n", (unsigned long)psr);
    printf("  CFSR = 0x%08lX\r\n", (unsigned long)cfsr);
    printf("  BFAR = 0x%08lX\r\n", (unsigned long)bfar);

    while (1) { }
}
```

Con el `PC` apilado, `addr2line` puede traducir la dirección a una ubicación del ELF usado para
esa compilación:

```bash
arm-none-eabi-addr2line -e build/firmware.elf 0x000004A2
# /home/.../src/main.c:87
```

Si no aparece una línea, verificá que el ELF corresponda exactamente al binario grabado y conservá
la información de depuración. Para faults imprecisos también conviene inspeccionar instrucciones
cercanas con `arm-none-eabi-objdump -dS`.

El handler anterior es un punto de partida didáctico. En un sistema real, `printf` puede depender de
estado ya dañado, bloquearse o provocar otro fault. Una alternativa robusta es guardar el marco y los
registros en una estructura fija para leerlos después. Antes de imprimir `BFAR`, comprobá
`BFARVALID` en `CFSR`.

> Si usás [RTT](./04-consola-por-el-debugger-rtt.md), `rtt_flush()` al entrar al handler da al
> host una oportunidad de consumir los mensajes que ya estaban en la cola. La función tiene un
> tiempo máximo de espera para no quedar bloqueada si no hay lector.

---

## Cómo evitarlos

| Práctica | Qué previene |
|---|---|
| Habilitar alimentación y clocks antes de acceder a un periférico | accesos inválidos durante la inicialización |
| Inicializar los punteros y validar su destino antes de usarlos | punteros indeterminados o fuera de rango |
| Evitar arreglos automáticos grandes y recursión sin límites | parte de los desbordes de stack |
| Revisar el `.map` y reservar margen para stack y heap | parte de los problemas de memoria antes de ejecutar |
| Compilar con `-Wall -Wextra` y revisar cada advertencia | varios errores de tipos, conversiones y uso de memoria |
| Rellenar el stack con un patrón y medir su marca de agua | detectar que el margen se está agotando |

---

## Lo que hay que recordar

- **HardFault es una categoría, no el diagnóstico completo.** Revisá si otra excepción escaló.
- **El marco de excepción contiene un PC.** El debugger puede usarlo, pero una pila corrupta o un
  fault impreciso requieren inspección adicional.
- **`CFSR` describe la causa y `BFAR` aporta una dirección solo cuando `BFARVALID` está activo.**
- **`addr2line` convierte una dirección en archivo y línea.**
- Desreferenciar `NULL` siempre es un error en C, aunque el mapa de memoria haga que una carga no
  produzca un fault inmediato.
- Antes de buscar casos raros, comprobá alimentación, clocks, resets y accesos a periféricos.

---

**Depurar en serio:** [índice](./README.md) ·
**Anterior:** [02 - El método del "no anda"](./02-el-metodo-del-no-anda.md) ·
**Siguiente:** [04 - La consola por el cable del debugger (RTT)](./04-consola-por-el-debugger-rtt.md)
