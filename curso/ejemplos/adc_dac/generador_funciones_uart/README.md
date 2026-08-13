# Generador de funciones por DAC y DMA

Este proyecto convierte la LPCXpresso LPC1769 en un generador de funciones controlado
desde la PC. Produce una señal continua por `P0.26/AOUT`, permite cambiar forma,
frecuencia, amplitud y offset por UART0, y puede verificar su propia salida con el ADC.

No contiene una colección de ondas precalculadas en Flash. Cuando llega una configuración,
la CPU calcula un período en una tabla de RAM. Después, GPDMA repite esa tabla y alimenta al
DAC sin ejecutar código por cada muestra. Ese reparto de tareas es la idea central del
ejemplo:

```text
aplicación Python -> UART0 RX -> calcular próxima tabla -> cambio breve de DMA
                                                    |
                                                    v
                                      GPDMA -> DAC -> P0.26
                                                       |
                                        loopback opcional
                                                       v
                          aplicación Python <- UART0 TX <- ADC <- P0.23
```

## Qué genera

- Senoidal.
- Cuadrada.
- Triangular.
- Serrucho ascendente.
- Serrucho descendente.

La senoide se calcula en punto fijo con la aproximación de Bhaskara I. No usa `float`,
`libm` ni una tabla de senos. Las otras formas se resuelven con aritmética entera.

## Conexiones

Con la placa apagada, conectá:

| LPCXpresso LPC1769 | Conexión |
|---|---|
| `P0.26/AOUT` | `P0.23/AD0.0`, solo para el monitor por loopback |
| `P0.2/TXD0` | `RXD` del CP2102 |
| `P0.3/RXD0` | `TXD` del CP2102 |
| `GND` | `GND` del CP2102 |

TX y RX se cruzan. No conectes el pin `VCC` del CP2102: la placa ya tiene su propia
alimentación. Para medir la salida con un osciloscopio externo, conectá la punta a
`P0.26/AOUT` y la masa a `GND`.

El puente directo entre DAC y ADC es válido en este proyecto porque ambos pertenecen a la
misma placa y la salida queda entre 0 V y la referencia analógica. No apliques una señal
externa negativa ni superior a `VDDA` sobre el ADC.

## Compilar, grabar y abrir la aplicación

Desde esta carpeta:

```bash
python3 -m pip install -r pc/requirements.txt
make flash
make app PORT=/dev/ttyUSB0
```

Si hay un solo CP2102 conectado, `make app` suele detectarlo sin indicar `PORT`. En Linux,
Tkinter puede venir en un paquete separado, por ejemplo `python3-tk`.

La aplicación permite elegir los cuatro parámetros del generador y muestra:

- Frecuencia pedida y frecuencia realmente programada.
- Cantidad de muestras por período.
- Tasa de actualización del DAC.
- Captura de AD0.0 en tiempo real.
- Frecuencia y niveles estimados por el ADC.
- Errores de DMA, UART o CRC.

El gráfico dibuja sample and hold. No interpola ni suaviza verticalmente las muestras. El
trigger solamente elige el origen temporal, igual que en un osciloscopio: usa nivel de
offset, histéresis, permanencia después del cruce y alineación temporal submuestra.

## Amplitud y offset

La aplicación usa amplitud pico a pico en milivolts:

```text
Vmin = offset - Vpp / 2
Vmax = offset + Vpp / 2
```

La configuración se acepta solamente si la onda completa entra entre 0 y 3297 mV. El DAC
tiene 10 bits, por lo que un paso ideal vale aproximadamente 3,22 mV con una referencia de
3,3 V. El código máximo es 1023 y representa `VREF * 1023 / 1024`, no `VREF` exacta.

El valor real también depende de `VREFP`, de la tolerancia de alimentación, del error de
ganancia y del offset del DAC. La referencia de esta placa no es un patrón de laboratorio.
Si necesitás amplitud exacta, medila y calibrá el sistema.

## Cómo se elige la frecuencia

El reloj del DAC queda en 25 MHz. Para cada pedido, el firmware busca una cantidad de
muestras `N` y un divisor entero:

```text
tasa_DAC = 25 MHz / divisor
frecuencia = tasa_DAC / N
```

En ondas senoidal, triangular y serrucho prioriza más muestras por período mientras el
error de frecuencia no supere 500 ppm. En la cuadrada prioriza dos muestras, una alta y
una baja, porque repetir el mismo código no mejora el flanco y aumenta trabajo del bus.

La tasa del DAC se limita por software a 1 MS/s. Estos son los rangos aceptados:

| Forma | Rango aceptado | Qué ocurre en el extremo superior |
|---|---:|---|
| Senoidal | 1 Hz a 100 kHz | 10 muestras por período |
| Cuadrada | 1 Hz a 250 kHz | 2 muestras por período, 500 kS/s |
| Triangular | 1 Hz a 100 kHz | 10 muestras por período |
| Serruchos | 1 Hz a 100 kHz | 10 muestras por período |

Que una frecuencia sea aceptada no significa que conserve una forma ideal. A 100 kHz, una
senoide o rampa tiene pocos escalones. La cuadrada de 250 kHz alterna correctamente los
códigos, pero `AOUT` sigue siendo un DAC analógico: no tiene los flancos de un GPIO y la
carga limita todavía más la transición.

Como criterio práctico:

- Hasta 20 kHz, las ondas suaves tienen una representación cómoda y se pueden revisar con
  el monitor integrado.
- Entre 20 y 50 kHz, observá la forma con un osciloscopio externo y decidí si la distorsión
  es aceptable para tu aplicación.
- Cerca de 100 kHz, consideralo un experimento de límite, no un generador senoidal preciso.
- Para una cuadrada rápida y con buenos flancos, usá un timer sobre un GPIO. El DAC sirve si
  además necesitás controlar los niveles alto y bajo.

## Qué limita la parte analógica

El DAC es una fuente de señal, no una fuente de potencia. El datasheet del LPC1769 indica
una carga resistiva mínima de 1 kohm y una capacitancia de carga típica de 200 pF. Una
carga más exigente cambia la amplitud y empeora los flancos. Para alimentar cables largos,
parlantes, filtros de baja impedancia u otra etapa que consuma corriente, agregá un buffer
con amplificador operacional.

La salida tampoco puede ser bipolar. Para obtener, por ejemplo, una senoide de -2 V a 2 V,
hace falta una etapa externa que elimine el offset, aplique ganancia y disponga de una
alimentación adecuada.

Toda salida por tabla contiene escalones y réplicas espectrales alrededor de la tasa de
actualización. Si importa la pureza de la senoide, colocá un filtro pasabajos de
reconstrucción después del DAC. El filtro debe dejar pasar la frecuencia útil y atenuar las
imágenes sin exigir demasiado a `AOUT`.

Valores eléctricos de referencia: [datasheet oficial del
LPC1769](https://www.nxp.com/docs/en/data-sheet/LPC1769_68_67_66_65_64_63.pdf),
tablas 19 y 22.

## Alcance del monitor ADC

AD0.0 captura bloques de 512 muestras a 189,394 kS/s mediante Timer 0 y GPDMA. Una captura
dura unos 2,70 ms. Después se envía por UART, así que hay huecos entre bloques. Es un monitor
didáctico, no una adquisición continua.

Por Nyquist, este ADC no puede interpretar de forma unívoca señales de 94,7 kHz o más. En la
práctica, una onda de 20 kHz tiene unas 9,5 muestras ADC por período. Eso alcanza para verla,
pero no para caracterizar con precisión los armónicos de una cuadrada. Para validar los
límites superiores usá un osciloscopio externo con ancho de banda y tasa de muestreo
suficientes.

Es posible que aparezca alguna muestra aislada en 0 o 4095 cuando DAC, ADC y DMA trabajan a
tasas altas. La aplicación no la oculta. El trigger exige histéresis y varias muestras del
lado correcto para que ese punto aislado no desplace la pantalla.

## Impacto sobre la CPU y la memoria

La compilación actual con `-O2` ocupa:

| Recurso | Uso |
|---|---:|
| Flash | 5608 bytes |
| SRAM principal | 2392 bytes estáticos |
| AHB SRAM0 | 3144 bytes para ADC y transmisión UART |
| AHB SRAM1 | 4112 bytes para dos tablas y el descriptor DMA |
| Stack | `main` usa 136 bytes y `wavegen_build` 80 bytes, según GCC |

Las dos tablas de 512 palabras permiten calcular la próxima onda sin modificar la que GPDMA
está leyendo. El LPC1769 no permite que GPDMA acceda a Flash ni a la SRAM principal, por eso
los buffers están en secciones AHB explícitas.

El linker reserva 2 KiB para el stack. Las cifras de la tabla son por función y no incluyen
el código de arranque ni una interrupción que ocurra en el peor momento, pero dejan margen
suficiente en esta aplicación.

Con el monitor desactivado, la CPU calcula solamente al recibir un comando, rearma el canal
DMA y vuelve a dormir. La generación continua sin bloquear la CPU. Con el monitor activado,
la llamada espera cada captura y cada envío, pero la CPU duerme mientras los tres canales DMA
trabajan: canal 0 para ADC, canal 1 para DAC y canal 2 para UART0 TX. El DAC sigue siendo
autónomo en ambos casos. Usá el monitor para verificar y desactivalo si integrás el generador
con otra tarea sensible al tiempo.

Al aplicar una configuración se detiene y rearma brevemente el canal del DAC. La fase vuelve
al inicio de la tabla y puede aparecer una discontinuidad corta. Este diseño no garantiza
cambios sin glitch ni continuidad de fase.

## Pruebas realizadas en la placa

Se probó el 8 de agosto de 2026 con una LPCXpresso LPC1769, loopback `AOUT` a `AD0.0` y un
CP2102 a 921600 baud. El CP2102 transporta comandos y capturas, pero no determina la
frecuencia del DAC.

| Forma | Programada | Estimada por AD0.0 |
|---|---:|---:|
| Senoidal | 20,000 kHz | 20,004 kHz |
| Cuadrada | 20,000 kHz | 19,996 kHz |
| Triangular | 10,000 kHz | 10,115 kHz |
| Serrucho ascendente | 5,000 kHz | 5,001 kHz |
| Serrucho descendente | 5,000 kHz | 5,005 kHz |

También se aceptaron y programaron correctamente seno de 1 Hz, seno de 100 kHz, triangular
de 100 kHz y cuadrada de 250 kHz. Los pedidos fuera del rango o con niveles inválidos fueron
rechazados sin alterar la señal que ya estaba activa. En la prueba de las cinco formas no
hubo errores de CRC, formato ni DMA. El verificador también ajustó fase, ganancia y offset de
cada captura y comparó su forma con la onda ideal.

La estimación de AD0.0 no reemplaza una medición calibrada. En especial, el error que muestra
la triangular incluye la resolución temporal del ADC y el método de detección de cruces.

Para repetir la prueba automática:

```bash
make check PORT=/dev/ttyUSB0
```

## Organización del código

| Archivo | Responsabilidad |
|---|---|
| `src/wavegen.c` | Valida parámetros, elige temporización y calcula la tabla |
| `src/hardware.c` | Configura DAC, ADC, Timer 0, GPDMA y UART0 |
| `src/protocol.c` | Encuadra comandos, respuestas y capturas con CRC-16 |
| `src/main.c` | Coordina doble buffer, cambios y monitor |
| `pc/protocol.py` | Implementa el mismo protocolo en la PC |
| `pc/generator_app.py` | Control gráfico y osciloscopio sample and hold |
| `pc/generator_check.py` | Prueba automática de las cinco formas |

El protocolo es binario y usa CRC-16/CCITT. UART0 recibe por interrupciones para que un
comando no se pierda mientras la CPU transmite una captura. La respuesta informa la
frecuencia real, el divisor y la cantidad de muestras elegida, en vez de suponer que el
pedido pudo representarse exactamente.

## Usarlo bien

- Empezá con 20 kHz o menos y verificá forma y amplitud.
- Elegí primero `Vpp` y offset de modo que toda la señal quede dentro del rango.
- Desactivá el monitor cuando solamente necesites generar.
- Usá un buffer y un filtro externos si conectás una carga real.
- Medí con un osciloscopio externo antes de confiar en los extremos de frecuencia.
- Si necesitás continuidad de fase, barridos suaves, modulación rápida o sincronismo entre
  canales, tomá este proyecto como base y rediseñá el mecanismo de cambio de tabla.
