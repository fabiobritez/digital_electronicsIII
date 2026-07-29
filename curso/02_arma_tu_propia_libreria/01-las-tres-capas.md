# Las tres capas de CMSIS

Antes de construir nuestra librería, entendamos cómo está organizada la de verdad. CMSIS no es un
único archivo: es un sistema en **capas**, donde cada capa se apoya en la de abajo. Entender esta
separación es lo que te va a permitir reemplazar o adaptar cualquier pieza.

```
┌─────────────────────────────────────────────────────────┐
│  TU APLICACIÓN  (main.c)                                 │
│  "prendé el LED, mandá un dato por UART"                 │
└───────────────────────────┬─────────────────────────────┘
                            │ llama a funciones
┌───────────────────────────▼─────────────────────────────┐
│  CAPA 3: DRIVERS          (lpc17xx_gpio.c, _uart.c …)    │
│  Funciones cómodas: GPIO_SetDir(), UART_Send()           │
│  Adentro escriben registros por vos.                     │
└───────────────────────────┬─────────────────────────────┘
                            │ accede a
┌───────────────────────────▼─────────────────────────────┐
│  CAPA 2: DISPOSITIVO      (LPC17xx.h)                    │
│  Las structs y direcciones de CADA periférico del LPC.   │
│  LPC_GPIO0, LPC_UART0, LPC_ADC, PCONP…                   │
└───────────────────────────┬─────────────────────────────┘
                            │ se apoya en
┌───────────────────────────▼─────────────────────────────┐
│  CAPA 1: NÚCLEO           (core_cm3.h)                   │
│  Lo común a TODO Cortex-M3: NVIC, SysTick, tipos __IO…   │
│  No depende de NXP; lo define ARM.                       │
└─────────────────────────────────────────────────────────┘
```

## Capa 1: El núcleo (CMSIS-Core), `core_cm3.h`

Esta capa la define **ARM**, no NXP. Describe lo que es igual en **cualquier** chip que tenga un
núcleo Cortex-M3: el controlador de interrupciones (NVIC), el SysTick, el bloque de control del
sistema (SCB), y utilidades como `__enable_irq()`, `NVIC_EnableIRQ()`, `__WFI()`.

También define los famosos calificadores:
```c
#define __I   volatile const   // solo lectura  (input)
#define __O   volatile         // solo escritura (output)
#define __IO  volatile         // lectura/escritura
```
Son solo `volatile` (+ `const` para los de solo lectura). Sirven para **documentar** qué se puede
hacer con cada registro y para que el compilador te avise si escribís en uno de solo lectura.

> Como esta capa es de ARM, su documentación está en el **Capítulo 34** del manual
> ([`manual/ch34...`](../../manual/ch34_appendix-cortex-m3-user-guide.pdf)), no en los capítulos de
> NXP. Por eso un mismo SysTick se programa **igual** en un LPC1769, un STM32F1 o cualquier Cortex-M3.

## Capa 2: El dispositivo, `LPC17xx.h`

Esta capa la provee **NXP** (el fabricante del chip). Es la que vimos en el módulo 1: un montón de
`struct` que describen los registros de cada periférico **específico del LPC176x**, más los `#define`
con las direcciones base:

```c
typedef struct { __IO uint32_t FIODIR; ... } LPC_GPIO_TypeDef;
#define LPC_GPIO0  ((LPC_GPIO_TypeDef *) 0x2009C000)
```

Esta capa es **distinta para cada familia de chips**, porque cada uno tiene periféricos y direcciones
diferentes. Si pasás a un STM32, cambia esta capa (se llama `stm32f1xx.h`) pero la de abajo (núcleo)
es la misma.

**Con solo esta capa ya podés programar todo** (`LPC_GPIO0->FIODIR |= ...`). De hecho, mucha gente
trabaja "a registro pelado" usando únicamente `LPC17xx.h`, sin los drivers. Eso es lo que en la
materia hacés **antes del primer parcial**.

## Capa 3: Los drivers, `lpc17xx_gpio.c`, `lpc17xx_uart.c`…

Esta capa también la provee NXP, pero es **opcional**. Son funciones que envuelven la escritura de
registros para que sea más cómoda y legible:

```c
// En vez de:
LPC_GPIO0->FIODIR |= (1u << 22);
// escribís:
GPIO_SetDir(0, (1u << 22), 1);   // puerto 0, máscara, 1 = salida
```

Por dentro, `GPIO_SetDir()` hace **exactamente** `LPC_GPIO0->FIODIR |= ...`. Ni más ni menos. Es lo
que vas a usar **después del primer parcial**: la misma idea, empaquetada.

> Abrí [`library/CMSISv2p00_LPC17xx/src/lpc17xx_gpio.c`](../../library/CMSISv2p00_LPC17xx/src/)
> y vas a ver, adentro de cada función, los accesos a registros que ya sabés hacer.

## Por qué esta separación importa

La clave pedagógica: **cada capa se puede reemplazar sin tocar las otras.**

- ¿No te gusta el driver de NXP? Escribí el tuyo usando la capa 2. (Es lo que haremos.)
- ¿Cambiás de chip? Cambiás la capa 2, y tus drivers se adaptan.
- ¿Querés código portable? Diseñás tu capa 3 para que no dependa de los nombres de la capa 2.

En la [próxima página](./02-construyendo-mi-gpio.md) vamos a construir **nuestra propia capa 2 + capa
3 mínimas** para GPIO. Vas a ver que no es difícil: es todo lo que ya aprendiste, ordenado.

---

**Módulo:** [Armá tu propia librería](./README.md) ·
**Siguiente:** [02 - Construyendo mi GPIO](./02-construyendo-mi-gpio.md)
