# Referencia rápida: LPC1769

Resumen de una página con lo que más se usa (y más se olvida) al escribir firmware a registro.
Pensado para repasar antes del parcial. Cada fórmula está explicada en su módulo; esto es solo
el condensado.

**Qué entra en cada parcial:**

- **Parcial 1**: hasta Timers (módulos 0 a 8, lo básico de match). **Todo a nivel registro.**
- **Parcial 2**: Timers en todos sus modos (match, capture, counter, match externo), ADC/DAC
  y DMA. Se pueden usar los drivers CMSIS si querés.
- El resto (UART, I2C, SPI, USB...) se da en la materia pero no entra al parcial. La UART
  igual conviene tenerla fresca: es la herramienta de debug de todos los días.

---

## Parcial 1: a nivel registro

### El ritual de arranque (siempre, para todo periférico)

1. **Encender** → `LPC_SC->PCONP |= (1 << bit);`
2. **Clockear** → `PCLKSEL0/1` (por reset: CCLK/4 = **25 MHz** con la placa a 100 MHz)
3. **Pines** → `PINSEL` (función) y `PINMODE` (pull-up/down)
4. **Configurar** → registros de control del periférico
5. **Usar** → registros de datos/estado, o interrupciones (`NVIC_EnableIRQ`)

Si algo no anda, el 90% de las veces el problema está en los pasos 1–3.

### Bits de PCONP que más se usan

| Periférico | Bit | Periférico | Bit |
|------------|-----|------------|-----|
| Timer0 / Timer1 | 1 / 2 | Timer2 / Timer3 | 22 / 23 |
| UART0 / UART1 | 3 / 4 | ADC (`PCAD`) | 12 |
| PWM1 | 6 | GPDMA | 29 |
| GPIO | 15 | I2C0 / I2C1 / I2C2 | 7 / 19 / 26 |

**No tienen bit en PCONP:** SysTick (es del core), el DAC (se habilita por PINSEL) y EINT.
El ADC y los Timers 2/3 **arrancan apagados**: encendelos siempre. Detalle: [módulo 3](./03_clock_y_power/).

### PINSEL: de pin a registro

```
registro     = PINSEL(2*puerto + pin/16)        // división entera
corrimiento  = 2 * (pin % 16)
```

Ejemplo P0.22: `PINSEL1`, bits 13:12. Función `00` = GPIO (reset), `01`/`10`/`11` = alternativas.
`PINMODE` usa la misma cuenta: `00` pull-up (reset), `10` tri-state, `11` pull-down.
**Pines de ADC siempre en tri-state.** Detalle: [módulo 4](./04_pinsel/).

### GPIO (registros FIO)

```c
LPC_GPIO0->FIODIR |= (1 << n);    // 1 = salida, 0 = entrada (reset)
LPC_GPIO0->FIOSET  = (1 << n);    // pin en alto   (atómico: solo escribe los 1)
LPC_GPIO0->FIOCLR  = (1 << n);    // pin en bajo   (atómico)
estado = LPC_GPIO0->FIOPIN & (1 << n);   // leer
```

Nunca `FIOPIN = valor` para tocar un bit: pisás el puerto entero. Detalle: [módulo 5](./05_gpio/).

### SysTick (24 bits, clockeado por CCLK)

```
LOAD = (T × fclk) − 1        // T en segundos, fclk = 100 MHz -> 1 ms: LOAD = 99999
```

Máximo `LOAD` = 2²⁴−1 = 16.777.215 → a 100 MHz, T máximo ≈ **167 ms** (para más, contá ticks).
Con CMSIS: `SysTick_Config(SystemCoreClock / 1000)` = tick de 1 ms (el `−1` lo hace él).
Detalle: [módulo 6](./06_systick/).

### Interrupciones

```c
NVIC_EnableIRQ(TIMER0_IRQn);          // habilitar en el NVIC
NVIC_SetPriority(TIMER0_IRQn, 2);     // menor número = más prioridad (5 bits en LPC17xx)
```

Handlers: nombre exacto de la tabla de vectores (`TIMER0_IRQHandler`, `EINT0_IRQHandler`,
`ADC_IRQHandler`, `DMA_IRQHandler`...). **Las interrupciones de GPIO entran por `EINT3_IRQHandler`**
(se distinguen con `LPC_GPIOINT->IO0IntStatR`, etc.). Regla de oro: en el handler, **limpiar el flag
del periférico** (si no, reentra para siempre). EINT: `EXTMODE` (nivel/flanco), `EXTPOLAR`
(polaridad), limpiar con `EXTINT = (1 << n)`. Detalle: [módulo 7](./07_interrupciones/).

Variables compartidas ISR ↔ main: `volatile`, y si tienen más de una operación de acceso,
sección crítica (`__disable_irq()` / `__enable_irq()`).
Detalle: [módulo 7, pág. 3](./07_interrupciones/03-secciones-criticas-y-atomicidad.md).

### Timers 0–3: modo match (lo que entra en el parcial 1)

```
t_tick = (PR + 1) / PCLK                 // resolución del TC
T      = (MR0 + 1) × (PR + 1) / PCLK     // período con reset por match (MCR)
MR0    = T × PCLK / (PR + 1) − 1
```

Receta 1 ms con PCLK = 25 MHz: `PR = 24` (TC avanza cada 1 µs), `MR0 = 999`.
En `MCR`, por cada match: bit 0 = interrupción, bit 1 = reset del TC, bit 2 = stop.
Arrancar: `TCR = 1` (antes conviene `TCR = 2` para resetear). En el handler: **limpiar el flag**
con `LPC_TIM0->IR = (1 << 0);` (write-1-to-clear). Detalle: [módulo 8](./08_timers/).

---

## Parcial 2: timers completos, ADC/DAC y DMA (drivers permitidos)

### Timers: los otros modos

- **Capture** (`CCR` + `CR0`/`CR1`): en el flanco de un pin `CAPn.x`, el hardware copia el `TC`
  a `CRx`. Por canal, 3 bits en `CCR`: flanco de subida, de bajada, interrupción. Medís período
  o ancho de pulso restando dos capturas (la resta en `uint32_t` maneja sola el desborde).
- **Counter** (`CTCR`): el `TC` avanza con flancos de un pin `CAPn.x` en vez de con `PCLK`
  (bits 1:0 = modo, 3:2 = qué pin). Para contar eventos externos.
- **Match externo** (`EMR`): el match mueve un pin `MATn.x` por hardware (set/clear/toggle,
  2 bits por canal en `EMC`): señales sin CPU ni interrupción.

Driver: `TIM_Init`, `TIM_ConfigMatch`, `TIM_ConfigCapture`, `TIM_Cmd(LPC_TIM0, ENABLE)`.
Detalle: [módulo 8, pág. 3](./08_timers/03-capture-y-medicion.md).

### ADC (12 bits, máx 200 ksps)

```
f_ADC = PCLK_ADC / (CLKDIV + 1)   ≤ 13 MHz      // 65 ciclos por conversión
```

Con PCLK = 25 MHz: `CLKDIV = 1` → 12,5 MHz. Encender: `PDN` (`ADCR` bit 21) **y** PCONP bit 12.
Arrancar por software: `START = 001` (bits 26:24). Esperar `DONE` (bit 31 de `ADDRn`), resultado
en bits 15:4. En **burst**: `START = 000` y `ADGINTEN = 0`, no combinar burst con start.
`mV = cuentas × 3300 / 4096` (sin float).
Driver: `ADC_Init(LPC_ADC, rate)`, `ADC_ChannelCmd`, `ADC_StartCmd`, `ADC_ChannelGetData`.
Detalle: [módulo 10](./10_adc_dac/).

### DAC (10 bits, en P0.26 = AOUT, función 2)

```
Vout = VALUE × Vref / 1024        // VALUE en bits 15:6 de DACR (¡no /1023!)
```

No usa PCONP. `BIAS` (bit 16): 0 = rápido (1 µs, 700 µA), 1 = lento (2,5 µs, 350 µA).
Driver: `DAC_Init(LPC_DAC)`, `DAC_UpdateValue(LPC_DAC, valor)`.
Detalle: [módulo 10](./10_adc_dac/).

### DMA (GPDMA, 8 canales)

- Encender: PCONP bit 29 + `DMACConfig` bit `E = 1` (una sola vez, enciende el bloque entero).
- **Prioridad fija por número: canal 0 es el de mayor prioridad**, el 7 el de menor.
- Cada canal tiene su bloque de registros (`LPC_GPDMACH0`...): `SrcAddr`, `DestAddr`, `LLI`
  (listas enlazadas), `Control` (tamaño en bits 11:0, anchos, incrementos, bit I) y `Config`
  (tipo de transferencia: `00` M2M, `01` M2P, `10` P2M + periférico de origen/destino).
- En M2P/P2M el que marca el ritmo es la **request del periférico** (el ADC al terminar una
  conversión, el contador del DAC): sin request no se mueve nada.
- Flags: `DMACIntTCStat` / `DMACIntTCClear` (fin), `DMACIntErrStat` / `DMACIntErrClr` (error).
  Un bit por canal; en el handler limpiá con `(1 << canal)`.

Driver: `GPDMA_Init()` → armar `GPDMA_Channel_CFG_Type` → `GPDMA_Setup(&cfg)` →
`GPDMA_ChannelCmd(ch, ENABLE)`. Si el canal ya estaba habilitado, `GPDMA_Setup` devuelve
`ERROR`: frenalo primero. Detalle: [módulo 11](./11_dma/).

---

## No entra al parcial, pero la vas a usar igual: UART

```
baudrate = PCLK_uart / (16 × DL)         // DL = 256×DLM + DLL (con FDR neutro = 0x10)
DL       = PCLK_uart / (16 × baudrate)
```

9600 con PCLK = 25 MHz → `DL = 163`. Para 115200 el DL entero da >3% de error: hace falta el
**divisor fraccional** (`FDR`) o el driver (`UART_Init`), que lo calcula solo.
La trampa de siempre: `DLL`/`DLM` solo se ven con **DLAB=1** (`LCR` bit 7); bajalo antes de usar
`THR`/`RBR`. Transmitir: esperar `LSR` bit 5 (THRE). Recibir: esperar `LSR` bit 0 (RDR).
Detalle: [módulo 9](./09_uart/).

---

## Los cinco errores que más cuestan puntos

1. Periférico sin encender en `PCONP` (o pin sin `PINSEL`): registros que "no responden".
2. Cuenta de tiempo con el PCLK equivocado (es **25 MHz** por defecto, no 100 MHz).
3. Flag de interrupción sin limpiar: el programa "se cuelga" reentrando al handler.
4. `FIOPIN = ...` en vez de `FIOSET`/`FIOCLR`: pisás los demás pines del puerto.
5. Pin de ADC con pull-up (el reset la deja puesta): lecturas corridas. Tri-state siempre.
