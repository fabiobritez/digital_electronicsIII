# Electrónica Digital III: LPC1769 (curso)

Material del curso, organizado para aprenderse **en orden**. La filosofía es una sola, repetida en
cada periférico:

> **Primero entendés el hardware tocando los registros a mano. Cuando ya sabés qué hace cada bit,
> ves que el driver de CMSIS hace exactamente eso, empaquetado.**

Esa progresión registro → driver acompaña cómo se da la materia: **antes del primer parcial** se
trabaja a nivel de registros; **después**, con los drivers incluidos en el repositorio.

> **Todo lo que es herramienta y no materia** (compilar, grabar, depurar, las sondas, los
> protocolos JTAG y SWD, el toolchain) está aparte, en la unidad
> [**Herramientas**](../herramientas/). Se separó a propósito para que el curso quede solo con el
> LPC1769 y sus periféricos.

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
| 12 | [Debug](./12_debug/) | Se mudó a la unidad [Herramientas](../herramientas/06_depurar_en_serio/) |

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

### Aparte: la unidad [Herramientas](../herramientas/)

Está **fuera del curso**, en su propia carpeta, para que no se mezcle con los periféricos. Es todo
lo que rodea al chip: cómo se compila, cómo se graba, cómo se depura y con qué. Nada de esto entra
en los parciales y **no hace falta para cursar**: con MCUXpresso alcanza. Está para quien quiera
destapar la caja negra, y porque es lo único de todo el material que te va a servir igual el día
que te toque otro microcontrolador.

| # | Parte | De qué trata |
|---|-------|--------------|
| 01 | [Panorama](../herramientas/01_panorama/) | [El mapa completo de `main.c` al LED](../herramientas/01_panorama/01-el-mapa-completo.md), las dos formas de grabar un micro, y el vocabulario |
| 02 | [Protocolos y debug en el chip](../herramientas/02_protocolos_y_debug_en_el_chip/) | JTAG, SWD, y el hardware de depuración que el Cortex-M3 trae adentro del núcleo |
| 03 | [Debug probes](../herramientas/03_debug_probes/) | Que una sonda es **otro micro con su firmware**, qué es CMSIS-DAP y cómo se carga, y cuáles existen |
| 04 | [Toolchains](../herramientas/04_toolchains/) | Qué es `arm-none-eabi-gcc`, cómo se lee el **triplet**, y qué hay adentro de la carpeta del compilador |
| 05 | [Del código al binario](../herramientas/05_del_codigo_al_binario/) | Secciones, linker script, startup, y la secuencia de 0 V a `main()` (se puede leer junto al módulo 1) |
| 06 | [Depurar en serio](../herramientas/06_depurar_en_serio/) | **Acá está el ex módulo 12**: imprimir, el checklist del "no anda", hard faults y la consola por RTT |
| 07 | [LPC1769](../herramientas/07_lpc1769/) | Todo lo anterior aplicado: instalación en [Linux](../herramientas/07_lpc1769/03-instalacion-linux.md) y [Windows](../herramientas/07_lpc1769/04-instalacion-windows.md), el checksum de la boot ROM, una [guía por cada sonda](../herramientas/07_lpc1769/probes/), y cómo compila y graba MCUXpresso por dentro |

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
  [Linux](../herramientas/07_lpc1769/03-instalacion-linux.md) o
  [Windows](../herramientas/07_lpc1769/04-instalacion-windows.md), y una
  [guía por cada sonda](../herramientas/07_lpc1769/probes/) para el grabado
- **Cómo funciona todo eso por dentro:** la unidad [Herramientas](../herramientas/)
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
- **Si algo no anda:** el checklist de seis puntos está en
  [Herramientas 06-02](../herramientas/06_depurar_en_serio/02-el-metodo-del-no-anda.md).
- **Repaso final:** [REFERENCIA_RAPIDA.md](./REFERENCIA_RAPIDA.md), organizada por parcial.
- **Siempre:** ante una duda de hardware, abrí el capítulo correspondiente en
  [`../manual/INDEX.md`](../manual/INDEX.md).
