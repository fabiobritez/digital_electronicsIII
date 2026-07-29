# Configurar pines con CMSIS y el driver PINSEL

En la página anterior configuramos pines escribiendo PINSEL/PINMODE a mano. Funciona, pero hay que
calcular para cada pin **qué registro** y **qué corrimiento** usar: fácil equivocarse. CMSIS provee
un **driver** que te deja describir el pin con una estructura y se encarga de toda esa cuenta.

## El driver: `lpc17xx_pinsel`

La idea: en vez de pensar en bits y corrimientos, llenás una estructura que dice *"puerto X, pin Y,
función Z, este modo de resistencia, este modo de salida"*, y una función la aplica.

### La estructura `PINSEL_CFG_Type`

```c
typedef struct {
    uint8_t Portnum;     // número de puerto (0-4)
    uint8_t Pinnum;      // número de pin (0-31)
    uint8_t Funcnum;     // función (0-3)        -> lo que iba a PINSEL
    uint8_t Pinmode;     // modo de resistencia  -> lo que iba a PINMODE
    uint8_t OpenDrain;   // open-drain o normal  -> lo que iba a PINMODE_OD
} PINSEL_CFG_Type;
```

Fijate que los cinco campos son **exactamente** las tres decisiones de la página anterior (función,
resistencia, open-drain) más la identificación del pin. El driver no hace nada nuevo: traduce esta
struct a las escrituras de registros que ya sabés hacer.

### La función `PINSEL_ConfigPin()`

```c
void PINSEL_ConfigPin(PINSEL_CFG_Type *PinCfg);
```

Recibe un puntero a la struct y configura el pin. Adentro hace el cálculo de registro/corrimiento y
los `PINSEL &= ~...; PINSEL |= ...;` por vos.

### Constantes útiles del driver

```c
// Funciones
PINSEL_FUNC_0, PINSEL_FUNC_1, PINSEL_FUNC_2, PINSEL_FUNC_3
// Modos de resistencia (PINMODE)
PINSEL_PINMODE_PULLUP, PINSEL_PINMODE_TRISTATE, PINSEL_PINMODE_PULLDOWN
// Open drain
PINSEL_PINMODE_NORMAL, PINSEL_PINMODE_OPENDRAIN
```

> Ojo: el driver **no** define una constante para el modo repeater (`01`); si lo necesitás, va el
> valor crudo. Lo retomamos en la [página 04](./04-pinmode-opendrain-tolerancia.md).

## El mismo ejemplo de antes, ahora con el driver

Recordá el ejemplo de la página anterior (P0.0 como TXD3, sin resistencias, push-pull). A registro
eran 5 líneas con corrimientos. Con el driver:

```c
#include "lpc17xx_pinsel.h"

void config_p00_txd3(void) {
    PINSEL_CFG_Type pin;
    pin.Portnum   = 0;
    pin.Pinnum    = 0;
    pin.Funcnum   = PINSEL_FUNC_2;          // TXD3
    pin.Pinmode   = PINSEL_PINMODE_TRISTATE;// sin pull-up/down
    pin.OpenDrain = PINSEL_PINMODE_NORMAL;  // push-pull
    PINSEL_ConfigPin(&pin);
}
```

Más legible y sin riesgo de errar el corrimiento. **Hace lo mismo que el código a registro**: de
hecho, si abrís [`library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_pinsel.c`](../../library/CMSISv2p00_LPC17xx/Drivers/src/),
vas a ver adentro los mismos `PINSELx &= ~...; |= ...;`.

## Recetas frecuentes

### UART0 (TXD0 = P0.2 func 1, RXD0 = P0.3 func 1)
```c
PINSEL_CFG_Type pin;
pin.Funcnum = PINSEL_FUNC_1;
pin.Pinmode = PINSEL_PINMODE_TRISTATE;
pin.OpenDrain = PINSEL_PINMODE_NORMAL;
pin.Portnum = 0;
pin.Pinnum  = 2;  PINSEL_ConfigPin(&pin);   // TXD0
pin.Pinnum  = 3;  PINSEL_ConfigPin(&pin);   // RXD0
```

### Canal de ADC (ej. AD0.0 = P0.23 func 1): ¡tri-state!
```c
PINSEL_CFG_Type pin;
pin.Portnum = 0;  pin.Pinnum = 23;
pin.Funcnum = PINSEL_FUNC_1;
pin.Pinmode = PINSEL_PINMODE_TRISTATE;   // analógico: sin resistencias
pin.OpenDrain = PINSEL_PINMODE_NORMAL;
PINSEL_ConfigPin(&pin);
```

### I²C0 (SDA0 = P0.27, SCL0 = P0.28, func 1)
```c
PINSEL_CFG_Type pin;
pin.Funcnum = PINSEL_FUNC_1;               // lo único que importa en estos pines
pin.Pinmode = PINSEL_PINMODE_TRISTATE;     // se ignora: P0.27/P0.28 no tienen resistencias
pin.OpenDrain = PINSEL_PINMODE_NORMAL;     // se ignora: ya son open-drain por hardware
pin.Portnum = 0;
pin.Pinnum = 27; PINSEL_ConfigPin(&pin);
pin.Pinnum = 28; PINSEL_ConfigPin(&pin);
```

> P0.27/P0.28 son pines I²C **dedicados**: open-drain de hardware, sin resistencias internas. Acá
> solo importa el `Funcnum`; los otros dos campos no tienen efecto en estos pines. Para **I²C1/I²C2**
> sobre pines comunes (por ejemplo SDA1 en P0.0) es al revés: ahí el open-drain **sí** lo tenés que
> pedir vos con `OpenDrain = PINSEL_PINMODE_OPENDRAIN`. Detalle en la
> [página 04](./04-pinmode-opendrain-tolerancia.md).

## Errores comunes (valen para registro y para driver)

| Error | Correcto |
|---------|-----------|
| Configurar el periférico antes que el PINSEL | PINSEL primero, periférico después |
| Función equivocada (P0.2 con func 0 no es TXD0) | Verificar la función en el Cap. 7 |
| Pull-up en un pin de ADC | `TRISTATE` para señales analógicas |
| Olvidar open-drain en I²C1/I²C2 sobre pines comunes | `OPENDRAIN` en esos SDA/SCL (I²C0 no lo necesita) |
| Usar `=` en lugar de `&= ~ / \|=` al hacerlo a mano | "Limpiar y poner" para no pisar otros pines |

## Cuándo usar cada nivel

- **A registro (`LPC_PINCON->PINSELx`):** cuando estás aprendiendo (pre-parcial 1) y querés ver
  exactamente qué bit tocás. También cuando necesitás control fino o ahorrar el peso del driver.
- **Con el driver (`PINSEL_ConfigPin`):** cuando ya entendés qué hace por dentro y querés código
  claro y rápido de escribir (post-parcial 1). Es lo habitual en proyectos.

Ya sabés conectar pines con el driver. Si querés el detalle fino (el mapa completo de registros y la
fórmula exacta para cualquier pin) seguí en la [página 03](./03-mapa-de-registros.md). Y para los
modos de entrada, open-drain, I²C y tolerancia a 5 V, la
[página 04](./04-pinmode-opendrain-tolerancia.md). Con el módulo 3 (encender + clockear) y este, ya
tenés el "ritual de arranque" de cualquier periférico.

---

**Anterior:** [01 - La función de los pines](./01-funcion-de-los-pines.md) ·
**Módulo:** [PINSEL](./README.md) ·
**Siguiente:** [03 - El mapa completo de registros](./03-mapa-de-registros.md)
