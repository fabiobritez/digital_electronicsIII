# Electrónica Digital III: LPC1769 (curso)

Material del curso, organizado para aprenderse **en orden**. La filosofía es una sola, repetida en
cada periférico:

> **Primero entendés el hardware tocando los registros a mano. Cuando ya sabés qué hace cada bit,
> ves que el driver de CMSIS hace exactamente eso, empaquetado.**

Esa progresión registro → driver acompaña cómo se da la materia: **antes del primer parcial** se
trabaja a nivel de registros; **después**, con los drivers incluidos en el repositorio.

---

## Mapa del curso

### Bases (empezá acá)
| # | Módulo | De qué trata |
|---|--------|--------------|
| 0 | [Lenguaje C](./00_lenguaje_c/) | C para embebidos: tipos, punteros, structs, `volatile`, ancho fijo, [dónde vive cada variable](./00_lenguaje_c/10-donde-vive-cada-variable.md) (stack, heap y estáticos) y, para cerrar, [cómo se estructura un firmware entero](./00_lenguaje_c/17-superloop-y-codigo-no-bloqueante.md) |
| 1 | [Arquitectura y acceso a registros](./01_arquitectura_y_acceso_a_registros/) | **El módulo clave:** un registro es una dirección de memoria |
| 2 | [Armá tu propia librería](./02_arma_tu_propia_libreria/) | Construí tu mini-CMSIS desde cero, para entender que el hardware es tuyo |
| 3 | [Clock y Power](./03_clock_y_power/) | PCONP y PCLKSEL: encender y clockear periféricos (el paso que todos olvidan) |

### Periféricos (registro → driver en cada uno)
| # | Módulo | Periférico |
|---|--------|-----------|
| 4 | [PINSEL](./04_pinsel/) | Función de cada pin |
| 5 | [GPIO](./05_gpio/) | Entradas/salidas digitales |
| 6 | [SysTick](./06_systick/) | Base de tiempo del Cortex-M3 |
| 7 | [Interrupciones](./07_interrupciones/) | NVIC + EINT + interrupciones por GPIO |
| 8 | [Timers](./08_timers/) | Timers 0–3: match, capture, PWM |
| 9 | [UART](./09_uart/) | Comunicación serial |
| 10 | [ADC / DAC](./10_adc_dac/) | Conversión analógica ↔ digital |
| 11 | [DMA](./11_dma/) | Transferencias sin CPU (GPDMA) |
| 12 | [Debug](./12_debug/) | Framework de depuración |

### Periféricos de comunicación (plus)
| # | Módulo | Periférico |
|---|--------|-----------|
| 13 | [I2C](./13_i2c/) | Bus de 2 cables para sensores, EEPROM, displays |
| 14 | [SPI / SSP](./14_spi/) | Bus rápido full-duplex: flash, SD, displays |
| 15 | [USB](./15_usb/) | Device USB: puerto serie virtual (CDC), HID, almacenamiento |
| 16 | [PWM](./16_pwm/) | Brillo de LEDs, velocidad de motores, servos (se lee tras Timers, módulo 8) |

### Complementos (de consulta)
| # | Módulo | De qué trata |
|---|--------|--------------|
| 17 | [Hardware y placa](./17_hardware_y_placa/) | Leer el esquemático, electrónica mínima (3.3 V, corriente), e instrumentos de medición |
| 18 | [Periféricos adicionales](./18_perifericos_adicionales/) | RTC, Watchdog, QEI, CAN, I2S y Ethernet: para qué sirven y cómo se encaran (mismo molde registro → driver) |

### [Anexos](./anexos/) (opcionales: solo si armás tu propio entorno)
Van con **letra en vez de número**, para que se note que están fuera de la secuencia del curso. Nada
de esto entra en los parciales y **no hace falta para cursar**: con MCUXpresso alcanza. Están acá para
quien quiera compilar y grabar sin IDE, y para quien quiera destapar la caja negra.

| # | Anexo | De qué trata |
|---|-------|--------------|
| A | [Build, linker y startup](./anexos/A_build_linker_startup/) | Qué hace MCUXpresso por vos: secciones, linker script, código de arranque (se puede leer junto al módulo 1) |
| B | [Toolchain y entorno propio](./anexos/B_toolchain_y_entorno/) | [El camino completo de `main.c` al LED](./anexos/B_toolchain_y_entorno/00-el-camino-completo.md), qué hay adentro del compilador, setup en VSCode y en vim, instalación en [Linux](./anexos/B_toolchain_y_entorno/06-instalacion-linux.md) y [Windows](./anexos/B_toolchain_y_entorno/07-instalacion-windows.md), una [guía por cada debug probe](./anexos/B_toolchain_y_entorno/probes/), y cómo compila y graba MCUXpresso por dentro |

> **Notas:** varios módulos suman páginas extra de profundización: el [módulo 0 (C)](./00_lenguaje_c/)
> trae C embebido fino (`static`/`const`/`inline`/bitfields y punto fijo vs `float`) y cierra con
> **arquitectura de firmware** (superloop no bloqueante, máquinas de estado e intro a RTOS); el
> [módulo 1](./01_arquitectura_y_acceso_a_registros/), una nota avanzada sobre **bit-banding**; el
> [módulo 3](./03_clock_y_power/), **bajo consumo**; el [módulo 5 (GPIO)](./05_gpio/), **debounce y
> filtrado de entradas**; el [módulo 7](./07_interrupciones/), **secciones críticas y atomicidad**; y el
> [módulo 10 (ADC/DAC)](./10_adc_dac/), **muestreo, Nyquist y aliasing**.

### Práctica
- [`../plantilla/`](../plantilla/): **proyecto listo para usar**. Copialo, escribí tu código
  en `src/` y compilá con `make`. Graba y depura sin MCUXpresso
- [ejemplos/](./ejemplos/): código completo y funcional por periférico
- [ejercicios/](./ejercicios/): parciales resueltos (2022, 2023, 2025) con análisis de errores
- [REFERENCIA_RAPIDA.md](./REFERENCIA_RAPIDA.md): una página con el ritual, las fórmulas y los
  registros que más se usan (para tener al lado en el parcial)

### Referencia
- [`../manual/`](../manual/): el User Manual UM10360 **dividido en 35 PDFs por capítulo** +
  [`INDEX.md`](../manual/INDEX.md) que mapea cada periférico a su capítulo y a los registros clave.

---

## El "ritual de arranque" de todo periférico

Una vez que pasás las bases, cada periférico sigue el mismo guion. Tenelo siempre presente:

1. **Encenderlo** → `PCONP` (módulo 3)
2. **Clockearlo** → `PCLKSEL` (módulo 3)
3. **Conectar sus pines** → `PINSEL`/`PINMODE` (módulo 4)
4. **Configurar su comportamiento** → registros de control del periférico
5. **Usarlo** → registros de datos/estado (o interrupciones)

Si algo "no anda", repasá los pasos 1–3: el 90% de los problemas están ahí.

---

## Hardware y herramientas
- **Micro:** LPC1769 (ARM Cortex-M3, 100 MHz, 512 KB Flash, 64 KB RAM)
- **Placa:** LPCXpresso LPC1769 rev D (OM13085), con debug probe CMSIS-DAP a bordo
- **Entorno recomendado:** el toolchain abierto más la
  [plantilla](../plantilla/). Instalación en
  [Linux](./anexos/B_toolchain_y_entorno/06-instalacion-linux.md) o
  [Windows](./anexos/B_toolchain_y_entorno/07-instalacion-windows.md), y una
  [guía por cada probe](./anexos/B_toolchain_y_entorno/probes/) para el grabado
- **IDE alternativo:** MCUXpresso (gratuito), guía de instalación en
  [`05_gpio/_origen/0-ide.md`](./05_gpio/_origen/0-ide.md)
- **Librería:** CMSIS v2.00 para LPC17xx (en [`../library/`](../library/))

## Cómo estudiar
- **Parcial 1** (entra hasta Timers, módulos 0 → 8, lo básico de match): **todo a nivel registro**.
  Leé de cada periférico la **primera** página (la de registros) y hacé los ejemplos a registro.
- **Parcial 2** (Timers en todos sus modos, ADC/DAC y DMA; **se pueden usar los drivers**): las
  páginas de capture/counter de Timers, los módulos 10 y 11 completos, y la **segunda** página
  (driver) de cada periférico.
- El resto de los periféricos (UART, I2C, SPI, USB...) se ve en la materia pero no entra al
  parcial; la UART conviene manejarla igual porque es la herramienta de debug de todos los días.
- **Repaso final:** [REFERENCIA_RAPIDA.md](./REFERENCIA_RAPIDA.md), organizada por parcial.
- **Siempre:** ante una duda de hardware, abrí el capítulo correspondiente en
  [`../manual/INDEX.md`](../manual/INDEX.md).
