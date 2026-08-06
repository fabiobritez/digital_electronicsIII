# ADC y DAC a nivel registro

El mundo real es analógico; el micro es digital. Estos dos periféricos son el puente:

- **ADC** (Analog-to-Digital): mide una **tensión** en un pin y la convierte en un **número**. El del
  LPC1769 es de **12 bits** (0–4095), con **8 canales** multiplexados y arquitectura **SAR**
  (aproximaciones sucesivas).
- **DAC** (Digital-to-Analog): convierte un **número** en una **tensión** de salida. El del LPC1769
  es de **10 bits** (0–1023) en el pin **P0.26 (AOUT)**, con arquitectura de **cadena de resistencias**
  (resistor string) y salida **bufferada**.

La referencia de tensión es Vref ≈ **3.3 V** (en rigor, `VREFP` a `VREFN`; en la mayoría de las placas
`VREFP = VDDA = 3.3 V` y `VREFN = VSSA = 0 V`).

> Esta página es densa a propósito: es la referencia "campo por campo" del módulo. La
> [página 02](./02-adc-dac-con-driver.md) hace lo mismo con el driver CMSIS, y la
> [página 03](./03-muestreo-nyquist-y-aliasing.md) explica *cada cuánto* conviene leer (Nyquist).

---

## ADC: la idea física

El ADC tiene **un solo conversor** SAR compartido entre 8 canales por un multiplexor: convierte **un
canal a la vez**. "Aproximaciones sucesivas" significa que internamente hace una búsqueda binaria: en
cada ciclo de su reloj decide un bit del resultado, del más significativo al menos significativo. Por
eso una conversión de 12 bits no es instantánea: **necesita varios ciclos de reloj**.

Dos relojes distintos conviven, y conviene no confundirlos:

- **`PCLK_ADC`**: el reloj del periférico, que sale del árbol de clock del micro (módulo 3). Típicamente
  `CCLK/4` (con `CCLK = 100 MHz`, `PCLK_ADC = 25 MHz`).
- **Reloj de conversión** (`f_ADC`): se obtiene dividiendo `PCLK_ADC` por `CLKDIV+1`. Es el que marca el
  ritmo del SAR, y el manual exige que **no supere 13 MHz**.

### El cálculo del clock y del tiempo de conversión

```
f_ADC = PCLK_ADC / (CLKDIV + 1)   ≤ 13 MHz
```

Una conversión **completa** (12 bits) tarda **65 ciclos** de `f_ADC` (en modo software). En modo burst
tarda **64 ciclos**. De ahí sale la tasa de muestreo:

```
f_muestreo = f_ADC / 65
```

Para llegar a la tasa máxima publicada de **200 ksps**, hace falta `f_ADC = 13 MHz` aprox
(`13 MHz / 65 = 200 kHz`). Ejemplos numéricos con `PCLK_ADC = 25 MHz`:

| `CLKDIV` | `f_ADC = 25/(CLKDIV+1)` | ¿≤ 13 MHz? | `f_muestreo = f_ADC/65` |
|----------|-------------------------|------------|--------------------------|
| 0 | 25 MHz | NO (inválido) | - |
| 1 | 12.5 MHz | sí | ~192 ksps |
| 2 | 8.33 MHz | sí | ~128 ksps |
| 4 | 5 MHz | sí | ~77 ksps |
| 24 | 1 MHz | sí | ~15 ksps |

La regla de oro del manual: programá el **`CLKDIV` más chico que deje `f_ADC` en 13 MHz o un poco menos**.
La excepción que el datasheet menciona y que conviene tener presente: si tu fuente analógica es de
**alta impedancia**, a veces conviene un reloj **más lento** para darle tiempo al capacitor de muestreo
a cargarse (más sobre esto en "Consideraciones analógicas").

### Los registros (`LPC_ADC`)

| Registro | Acceso | Función |
|----------|--------|---------|
| `ADCR` | R/W | Control: canal(es), `CLKDIV`, `BURST`, `PDN`, `START`, `EDGE` |
| `ADGDR` | R/W | Global Data: último resultado + `CHN` + `OVERRUN` + `DONE` |
| `ADINTEN` | R/W | Habilitar interrupción por canal (`ADINTEN0..7`) o global (`ADGINTEN`) |
| `ADDR0..ADDR7` | RO | Resultado **por canal** + `OVERRUN`/`DONE` de ese canal |
| `ADSTAT` | RO | Espejo de `DONE`/`OVERRUN` de los 8 canales + flag de interrupción |
| `ADTRM` | R/W | Trim de offset (lo carga el bootcode; rara vez se toca) |

> Los nombres son **exactos** según `LPC17xx.h` (struct `LPC_ADC_TypeDef`). Notá que entre `ADGDR` y
> `ADINTEN` hay un `RESERVED0` en el struct: el offset de `ADINTEN` es `0x0C`, no `0x08`.

### `ADCR` campo por campo

| Bits | Símbolo | Qué hace |
|------|---------|----------|
| 7:0 | `SEL` | Un bit por canal. En modo software, **solo uno** en 1. En burst, cualquier combinación de 1 a 8 unos. `SEL = 0` equivale a `0x01` (canal 0). |
| 15:8 | `CLKDIV` | Divisor: `f_ADC = PCLK_ADC/(CLKDIV+1)`, debe quedar ≤ 13 MHz. |
| 16 | `BURST` | 1 = conversiones repetidas automáticas sobre los canales de `SEL`. 0 = controladas por `START`, 65 ciclos cada una. |
| 21 | `PDN` | 1 = ADC operativo. 0 = power-down (no convierte). |
| 26:24 | `START` | Cuándo arranca una conversión (ver tabla abajo). Solo aplica con `BURST = 0`. |
| 27 | `EDGE` | Con `START = 010..111`: 0 = flanco **ascendente**, 1 = flanco **descendente** del trigger. |

Los bits 20:17, 23:22 y 31:28 son **reservados**: no escribas unos ahí.

#### El campo `START` (bits 26:24) y los hardware triggers

| `START` | Significado |
|---------|-------------|
| `000` | No arrancar. El manual pide usar este valor **al apagar el ADC** (al poner `PDN = 0`). |
| `001` | Arrancar una conversión **ya** (software). |
| `010` | Arrancar en el flanco (según `EDGE`) de **P2.10 / EINT0** (el pin debe estar en función EINT0 por `PINSEL4`). |
| `011` | Arrancar en el flanco de **P1.27 / CAP0.1** (el pin debe estar en función CAP0.1 por `PINSEL3`). |
| `100` | Arrancar en el flanco de **MAT0.1** (no necesita que MAT0.1 salga a un pin). |
| `101` | Arrancar en el flanco de **MAT0.3**. |
| `110` | Arrancar en el flanco de **MAT1.0**. |
| `111` | Arrancar en el flanco de **MAT1.1**. |

Esto es clave para **muestreo uniforme**: en vez de disparar el ADC "a ojo" desde el CPU, configurás un
Timer (módulo 8) para que su Match (`MAT0.1`, etc.) dispare la conversión con período **exacto**. Eso es
justo lo que asume Nyquist (ver [página 03](./03-muestreo-nyquist-y-aliasing.md)). El detalle fino: el
pin/señal seleccionado se hace **XOR con `EDGE`** antes de la lógica de detección de flanco.

Otro dato del manual que importa acá: una conversión **en curso no se puede interrumpir**. Un nuevo
`START = 001` o un nuevo flanco de trigger que llegue mientras el ADC está convirtiendo **se ignora**.
Moraleja: el período del trigger tiene que ser mayor que el tiempo de conversión (los 65 ciclos).

> **Regla que se viola seguido:** `START` y `BURST` **no se combinan**. Si `BURST = 1`, `START` debe ser
> `000` o las conversiones **no arrancan**. Y si `BURST = 1`, el bit `ADGINTEN` de `ADINTEN` debe ser 0.

#### Qué pin es cada canal

| Canal | Pin | Función en `PINSEL` |
|-------|-----|----------------------|
| AD0.0 | P0.23 | `01` (PINSEL1 15:14) |
| AD0.1 | P0.24 | `01` (PINSEL1 17:16) |
| AD0.2 | P0.25 | `01` (PINSEL1 19:18) |
| AD0.3 | P0.26 | `01` (PINSEL1 21:20), el mismo pin del AOUT del DAC |
| AD0.4 | P1.30 | `11` (PINSEL3 29:28) |
| AD0.5 | P1.31 | `11` (PINSEL3 31:30) |
| AD0.6 | P0.3 | `10` (PINSEL0 7:6), compartido con RXD0 |
| AD0.7 | P0.2 | `10` (PINSEL0 5:4), compartido con TXD0 |

Ojo con el número de función: **no** es siempre `01`. Y en todos los casos, el `PINMODE`
correspondiente va en `10` (sin pull-up ni pull-down).

### Conversión por software, paso a paso (canal AD0.0 = P0.23)

```c
#include <LPC17xx.h>

void adc_init(void) {
    // 1) Encender el ADC (PCONP bit 12 = PCADC). En reset el ADC viene apagado.
    LPC_SC->PCONP |= (1u << 12);

    // 2) Pin P0.23 como AD0.0 (función 1) y SIN resistencias (tri-state).
    //    P0.23 -> PINSEL1 bits 15:14 ; PINMODE1 bits 15:14
    LPC_PINCON->PINSEL1  &= ~(0x3u << 14);
    LPC_PINCON->PINSEL1  |=  (0x1u << 14);    // funcion 1 = AD0.0
    LPC_PINCON->PINMODE1 &= ~(0x3u << 14);
    LPC_PINCON->PINMODE1 |=  (0x2u << 14);    // 10 = ni pull-up ni pull-down (tri-state)

    // 3) ADCR: canal 0, CLKDIV para quedar <= 13 MHz, PDN=1, START=000.
    //    Con PCLK_ADC = 25 MHz: 25/(CLKDIV+1) <= 13  ->  CLKDIV=1  ->  12.5 MHz.
    LPC_ADC->ADCR = (1u << 0)        // SEL: canal 0
                  | (1u << 8)        // CLKDIV = 1
                  | (1u << 21);      // PDN = 1 (encendido). START queda en 000.
}

uint16_t adc_read_ch0(void) {
    // Lanzar una conversión: START = 001 (bit 24). No tocar el resto de ADCR.
    LPC_ADC->ADCR &= ~(0x7u << 24);
    LPC_ADC->ADCR |=  (0x1u << 24);

    while (!(LPC_ADC->ADDR0 & (1u << 31))) { }   // esperar DONE del canal 0
    return (LPC_ADC->ADDR0 >> 4) & 0xFFF;        // RESULT en bits 15:4 -> 12 bits
}
```

### `ADGDR` vs `ADDRn`: cuándo usar cada uno

Hay **dos formas** de leer el resultado, y mezclarlas trae problemas:

- **`ADGDR` (Global Data Register):** un único registro con el resultado de la **última conversión que
  terminó**, sin importar el canal. Campos:

  | Bits | Campo | Significado |
  |------|-------|-------------|
  | 15:4 | `RESULT` | Los 12 bits (fracción binaria entre `VREFN` y `VREFP`). |
  | 26:24 | `CHN` | De qué canal salió este resultado (000 = canal 0, ...). |
  | 30 | `OVERRUN` | 1 si en burst se perdió un resultado antes de leerlo. Se limpia al **leer** el registro. |
  | 31 | `DONE` | 1 cuando terminó una conversión. Se limpia al leer `ADGDR` **y** al escribir `ADCR`. |

  Conviene `ADGDR` cuando convertís **un solo canal** o cuando, en burst multicanal, te alcanza con leer
  "lo último que salió" mirando `CHN` para saber de quién es.

- **`ADDR0..ADDR7` (uno por canal):** cada uno guarda el último resultado **de su canal**, con su propio
  `DONE` (bit 31) y `OVERRUN` (bit 30). El layout de `RESULT` es idéntico (bits 15:4). Conviene cuando
  convertís **varios canales** y querés el valor más reciente de **cada uno** por separado.

> **El error sutil:** elegí **un método y quedate con él**. El manual avisa que los flags `DONE`/`OVERRUN`
> de `ADGDR` y de los `ADDRn` se pueden **desincronizar** entre sí, generando interrupciones o pedidos de
> DMA espurios. No leas `ADGDR` y `ADDRn` de forma intercalada esperando que los flags concuerden.

### Interrupción: `ADINTEN` y `ADSTAT`

- `ADINTEN` bits 7:0 (`ADINTEN0..7`): habilitan que el `DONE` de **ese canal** genere interrupción.
- `ADINTEN` bit 8 (`ADGINTEN`): elige la fuente. Con `1`, **solo** el `DONE` global de `ADGDR` genera
  interrupción; con `0`, **solo** los canales habilitados por `ADINTEN0..7`. Reset value de `ADINTEN`
  es `0x100`, o sea `ADGINTEN = 1` por defecto.
- La IRQ es `ADC_IRQn`; el handler es `ADC_IRQHandler`. El `DONE` se **niega al leer** el `ADDR`/`ADGDR`
  correspondiente (así se "reconoce" la interrupción leyendo el dato).

`ADSTAT` es de solo lectura y junta todo: bits 7:0 espejan los `DONE` de cada canal, bits 15:8 espejan
los `OVERRUN`, y el bit 16 (`ADINT`) es el OR de todos los `DONE` habilitados. Útil para saber de un
vistazo "qué terminó" sin leer ocho registros.

### Modo BURST: conversión continua

Con `BURST = 1`, el ADC recorre **solo** los canales marcados en `SEL`, sin que arranques cada conversión,
y vuelve a empezar al terminar. La primera conversión corresponde al `1` menos significativo de `SEL`,
después los de número mayor. Cada conversión en burst tarda **64 ciclos** (uno menos que en software).
Para **frenar** el burst, limpiá el bit `BURST`: la conversión que esté en curso en ese momento **se
completa** igual.

Para leer sin perder datos: leé el `ADDRn` del canal que te interesa cuando su `DONE` esté en 1. Si el
ADC convirtió de nuevo **antes** de que leyeras, se prende `OVERRUN` (perdiste una muestra, pero el dato
en el registro sigue siendo el más reciente). En burst **no** uses un `while(!DONE)` pensado para
software: el dato ya está, vos solo lo cosechás.

```c
// Burst sobre canales 0 y 1
LPC_ADC->ADCR &= ~(0x7u << 24);          // START = 000 (obligatorio en burst)
LPC_ADC->ADINTEN &= ~(1u << 8);          // ADGINTEN = 0 (obligatorio en burst)
LPC_ADC->ADCR |= (1u << 0) | (1u << 1);  // SEL canales 0 y 1
LPC_ADC->ADCR |= (1u << 16);             // BURST = 1 -> arranca solo
// luego: leer LPC_ADC->ADDR0 y LPC_ADC->ADDR1 cuando tengan DONE
```

### El ADC y el DMA

El ADC genera el **request de DMA desde su misma línea de interrupción**: para que haya transferencia DMA
tienen que cumplirse las mismas condiciones que para una interrupción. Dos puntos importantes del manual:

- Si usás DMA, **deshabilitá la IRQ del ADC en el NVIC** (`NVIC_DisableIRQ(ADC_IRQn)`); si no, competirían
  por la misma línea.
- El DMA del ADC solo soporta **burst requests**; los tamaños de burst útiles son 1, 4 u 8. Si convertís
  una cantidad de canales que no encaja, poné burst size = 1.

El combo Timer → ADC → DMA es la forma profesional de capturar a frecuencia fija sin CPU. Se arma del
todo en el [módulo 11 (DMA)](../11_dma/).

### Consideraciones analógicas (lo que el datasheet no recalca)

- **Tri-state obligatorio en el pin.** Una resistencia de pull interna forma un divisor con tu fuente y
  falsea la lectura. Por eso `PINMODE = 10` (ni pull-up ni pull-down). Además, al seleccionar la función
  ADC en `PINSEL`, un circuito interno **desconecta la parte digital** del pin: no podés tener función
  digital y lectura ADC válida a la vez en el mismo pin.
- **Vref y el rango.** La conversión es una **fracción** entre `VREFN` y `VREFP`: 0x000 ≈ `VREFN`,
  0xFFF ≈ `VREFP`. La calidad de tu medición depende de qué tan **limpia y estable** sea esa referencia.
- **Nunca superes `VDDA` en el pin.** El manual es explícito: niveles por encima de `VDDA` en una entrada
  analógica dan **lecturas inválidas** (y pueden dañar el pin). Si tu señal puede pasarse, poné un divisor
  y/o un clamp.
- **Impedancia de fuente.** El SAR carga un capacitor de muestreo; si tu fuente es de alta impedancia
  (megaohms), el cap no llega a cargarse en 65 ciclos rápidos. Solución: bajar la impedancia (un buffer
  op-amp) o **bajar `f_ADC`** para alargar el tiempo de muestreo.
- **Ruido y promediado.** El ADC del LPC1769 no es de instrumentación; tiene ruido de unos pocos LSB.
  Promediar N muestras reduce el ruido aleatorio y mejora la resolución efectiva (no reemplaza al filtro
  antialiasing de la página 03, pero es gratis y ayuda).

### El ritual de arranque del ADC

1. **PCONP bit 12 (`PCADC`)** = 1: alimentar el periférico. En reset viene apagado.
2. **`PCLKSEL0`**: elegir `PCLK_ADC` (módulo 3) y calcular `CLKDIV` para `f_ADC ≤ 13 MHz`.
3. **`PINSEL`** en función ADC + **`PINMODE` tri-state**.
4. **`ADCR`**: `SEL`, `CLKDIV`, `PDN = 1`, `START = 000`.
5. Arrancar (`START = 001`, hardware trigger o `BURST`) y leer `ADDRn`/`ADGDR`.

---

## DAC

El DAC es más simple, pero tiene una rareza importante: **no tiene bit en `PCONP`**. Está siempre
conectado a `VDDA`; se "habilita" configurando su pin (P0.26) en función DAC con `PINSEL`. El manual es
tajante: **esto hay que hacerlo antes de tocar cualquier registro del DAC**.

Arquitectura: **cadena de resistencias** de 10 bits con **salida bufferada** (un seguidor interno). Tasa
máxima de actualización: **1 MHz** (con `BIAS = 0`).

### Registros (`LPC_DAC`)

| Registro | Acceso | Función |
|----------|--------|---------|
| `DACR` | R/W | Valor de 10 bits a convertir + bit `BIAS` |
| `DACCTRL` | R/W | Control del modo DMA/timer (doble buffer, contador, DMA) |
| `DACCNTVAL` | R/W | Valor de recarga (16 bits) del timer de DMA/interrupción |

> En `LPC17xx.h`, `DACCNTVAL` es `uint16_t`: son 16 bits, no 32.

### `DACR` campo por campo

| Bits | Símbolo | Significado |
|------|---------|-------------|
| 5:0 | - | Reservados (pensados para futuros DAC de mayor resolución). Escribir 0. |
| 15:6 | `VALUE` | Los 10 bits del valor. La salida queda en `Vout = VALUE × (VREFP − VREFN)/1024 + VREFN`. |
| 16 | `BIAS` | Compromiso velocidad/consumo (ver abajo). |

> **Ojo con el divisor:** el manual usa **/1024**, no /1023. Es decir, `VALUE = 1023` (todo en uno) da
> `1023/1024 × Vref ≈ 0.999 × Vref`, **no** exactamente `Vref`. Para cuentas didácticas `/1023` se ve por
> ahí, pero el valor correcto del hardware es `/1024`.

#### El bit `BIAS` (settling time vs corriente)

| `BIAS` | Settling time | Corriente máx | Update rate máx |
|--------|---------------|---------------|------------------|
| 0 | **1 µs** | **700 µA** | **1 MHz** |
| 1 | **2.5 µs** | **350 µA** | **400 kHz** |

El "settling time" es lo que tarda la salida en **estabilizarse** tras escribir un valor nuevo. Si vas a
generar ondas rápidas, necesitás `BIAS = 0` (rápido, pero consume más). Si la salida cambia lento o
querés ahorrar, `BIAS = 1`. Estos tiempos valen para una **carga capacitiva ≤ 100 pF** en AOUT: una
carga mayor alarga el settling.

### Generar una tensión fija

```c
void dac_init(void) {
    // P0.26 como AOUT (funcion 2). El DAC NO usa PCONP.
    LPC_PINCON->PINSEL1 &= ~(0x3u << 20);
    LPC_PINCON->PINSEL1 |=  (0x2u << 20);    // funcion 2 = AOUT
    // (opcional) tri-state en el pin de salida analogica:
    LPC_PINCON->PINMODE1 &= ~(0x3u << 20);
    LPC_PINCON->PINMODE1 |=  (0x2u << 20);
}

void dac_write(uint16_t valor10bits) {
    // VALUE en bits 15:6. Preservar BIAS si ya estaba seteado.
    uint32_t reg = LPC_DAC->DACR & (1u << 16);    // conservar BIAS
    reg |= (uint32_t)(valor10bits & 0x3FF) << 6;
    LPC_DAC->DACR = reg;
}

// Ejemplo: sacar ~1.65 V (mitad de 3.3 V)
// dac_write(512);
```

### Modo DMA/timer: `DACCTRL` y `DACCNTVAL`

El DAC trae un **timer propio de 16 bits** para soltar valores a ritmo fijo sin CPU. Esto es lo que
permite generar ondas continuas con DMA. Bits de `DACCTRL`:

| Bit | Símbolo | Significado |
|-----|---------|-------------|
| 0 | `INT_DMA_REQ` | Se pone en 1 por hardware cuando el contador llega a 0; se limpia al escribir `DACR`. |
| 1 | `DBLBUF_ENA` | Habilita doble buffer en `DACR` (requiere `CNT_ENA`). |
| 2 | `CNT_ENA` | Habilita el contador de time-out. |
| 3 | `DMA_ENA` | Rutea el request del DAC al GPDMA (DMA burst request 7). |

Cómo funciona (figura 134 del manual): con `CNT_ENA = 1`, un contador de 16 bits **cuenta para abajo**
desde `DACCNTVAL` al ritmo de `PCLK_DAC`. Cuando llega a 0: (a) se recarga con `DACCNTVAL`, (b) se setea
el request de DMA/interrupción. Si `DMA_ENA = 1`, ese request va al GPDMA, que escribe el próximo valor
de la tabla en `DACR`. El **período de actualización** es entonces:

```
T_update = DACCNTVAL / PCLK_DAC
f_onda = PCLK_DAC / (DACCNTVAL × N_muestras_por_ciclo)
```

**Doble buffer** (`DBLBUF_ENA = 1`, requiere `CNT_ENA = 1`): cualquier escritura a `DACR` cae primero en
un **pre-buffer**; el `DACR` real se carga desde ahí recién cuando el contador llega a 0. Esto evita
glitches: el valor cambia **sincronizado con el tick**, no en cualquier momento. Leer `DACR` devuelve el
`DACR` real, no el pre-buffer. (El timer interno **no** es accesible para lectura/escritura; sí lo son
`DACCTRL` y `DACCNTVAL`.)

### Generar una señal (anticipo de DMA)

Para una onda (seno, triangular), se escribe el DAC repetidamente con los valores de una **tabla**, a
intervalos fijos. Hacerlo con el CPU funciona, pero lo elegante es que el **timer del DAC dispare al
DMA** para que copie la tabla a `DACR` sin intervención del CPU: señal continua y sin jitter. Eso se ve
en el [módulo 11 (DMA)](../11_dma/) y hay un ejemplo en
[`../ejemplos/dma/dac_dma_sin.c`](../ejemplos/dma/).

> **El DAC no maneja cargas pesadas.** El buffer de salida entrega como mucho 700 µA (`BIAS = 0`). No le
> cuelgues un parlante de 8 Ω ni un LED directo: necesitás una etapa de potencia (un op-amp o un
> transistor) después de AOUT. Además, la salida del DAC se **deshabilita** en modos deep-sleep,
> power-down y deep power-down.

### El ritual de arranque del DAC

1. **`PCLKSEL0`**: elegir `PCLK_DAC` (módulo 3): importa para el ritmo del timer de DMA.
2. **`PINSEL1`**: P0.26 en función 2 (AOUT). **Esto habilita el DAC** (no hay PCONP).
3. (Opcional) `BIAS` según la velocidad que necesites.
4. Escribir `VALUE` en `DACR` (bits 15:6). Para ondas: configurar `DACCTRL`/`DACCNTVAL` + DMA.

---

En la [próxima página](./02-adc-dac-con-driver.md) hacemos todo esto con los drivers CMSIS: cómo el
`ADC_Init` calcula el `CLKDIV` solo, burst, hardware trigger, interrupción, y el DAC con su modo DMA.

---

**Módulo:** [ADC/DAC](./README.md) · **Siguiente:** [02 - ADC/DAC con el driver](./02-adc-dac-con-driver.md)
