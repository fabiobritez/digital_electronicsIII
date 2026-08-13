# Osciloscopio didáctico con DAC, ADC, DMA y UART

Este proyecto genera una senoide de 20 kHz con el DAC del LPC1769, la vuelve a medir con AD0.0 y
manda las muestras a la PC por UART0. La aplicación de escritorio muestra exactamente los valores
recibidos con sample and hold: mantiene cada muestra hasta que llega la siguiente. No interpola ni
suaviza la curva.

El proyecto está separado de los demás ejemplos y no modifica el Debug Framework.

## Qué hace el firmware

```text
tabla de 50 valores -> GPDMA canal 0 -> DAC a 1 MS/s -> P0.26/AOUT
                                                            |
                                                     puente físico
                                                            |
Timer0/MAT0.1 -> ADC a 189,394 kS/s <- P0.23/AD0.0           |
                         |
                   GPDMA canal 1
                         |
                    buffers en RAM -> protocolo binario -> UART0 -> CP2102 -> PC
```

La senoide tiene:

- 20 kHz de frecuencia.
- 50 actualizaciones del DAC por período.
- Una actualización del DAC cada 1 us, el máximo especificado para `BIAS = 0`.
- Un offset de 512 y una amplitud de 450 códigos sobre los 10 bits del DAC.
- Una salida aproximada de 0,20 a 3,10 V cuando la referencia es 3,3 V.

El ADC no usa `ADC_Init(..., 200000)`. El reloj se configura explícitamente en 12,5 MHz y MAT0.1
inicia una conversión cada 5,28 us. Esto da 189.393,94 muestras/s y deja un pequeño margen respecto
de los 5,20 us que requiere cada conversión.

## Conexiones

Con la placa apagada, colocá estos puentes:

```text
LPC1769 P0.26 / AOUT  ----------------  LPC1769 P0.23 / AD0.0

LPC1769 P0.2 / TXD0   ----------------  RXD del CP2102
LPC1769 GND            ----------------  GND del CP2102
```

Para el lazo DAC a ADC solamente hace falta el primer cable. Como ambos pines pertenecen a la misma
placa, ya comparten masa y referencia. La entrada del ADC es de alta impedancia y el DAC tiene salida
bufferizada, así que en esta prueba se pueden conectar directamente.

No conectes lo siguiente:

- No unas AOUT con AD0.3. AD0.3 también es P0.26: es el mismo pin y no puede ser DAC y ADC al mismo
  tiempo.
- No conectes 5 V ni 3,3 V del CP2102 a la placa. Alimentá la LPCXpresso con su propio USB. Para la
  UART solamente hacen falta RXD y GND.
- No conectes el TXD del CP2102: este firmware no recibe comandos.
- No agregues un capacitor en el puente si querés observar la respuesta real de este banco. El
  capacitor modificaría la señal y escondería parte del comportamiento del DAC.

Para ensayar una señal externa, la situación cambia. La entrada debe quedar siempre entre VREFN y
VREFP, no admite tensión negativa y necesita protección y filtro anti-alias adecuados.

## Dos modos de adquisición

### Modo 1: máxima fidelidad por ventanas

Es el modo predeterminado.

- Captura 1024 muestras de 12 bits a 189,394 kS/s.
- Cada ventana contiene aproximadamente 5,41 ms de señal.
- Detiene la adquisición mientras manda 2080 bytes por UART.
- La transmisión ocupa aproximadamente 22,6 ms.
- Produce alrededor de 35 ventanas por segundo.
- Conserva unas 9,47 muestras por período de la senoide de 20 kHz.

Hay pausas entre ventanas. La forma dentro de cada ventana conserva la velocidad completa del ADC.
Es la opción recomendada para mirar una señal de 20 kHz por UART.

### Modo 2: streaming continuo

Este modo usa dos buffers alternados y no detiene el ADC.

- Captura continuamente a 189,394 kS/s.
- Toma una de cada tres muestras.
- Reduce cada resultado a 8 bits.
- Transmite 63,131 kS/s.
- Conserva solamente unas 3,16 muestras por período a 20 kHz.

Sirve para comprobar que puede existir streaming sin huecos, pero la forma de una senoide de 20 kHz
queda bastante pobre. Es el costo de entrar en los aproximadamente 92.104 bytes/s reales de la UART
a 921600 baud.

## Compilar y grabar

Desde esta carpeta:

```bash
make
make flash
```

Eso compila y graba el modo 1.

Para el modo continuo:

```bash
make SCOPE_MODE=2
make SCOPE_MODE=2 flash
```

Cada modo usa su propia carpeta de build, por lo que se pueden alternar sin reutilizar objetos
compilados con la configuración anterior.

## Probar primero sin interfaz gráfica

La prueba automática valida sincronización, tamaños, CRC, secuencias y banderas de error:

```bash
python3 pc/capture_check.py --port /dev/ttyUSB0 --frames 100
```

Si hay un solo CP210x conectado, el puerto se puede detectar automáticamente:

```bash
python3 pc/capture_check.py --port auto --frames 100
```

Con el puente AOUT a AD0.0 colocado, el resultado esperado es aproximadamente:

```text
Frecuencia medida:      20.000 Hz
Código mínimo/máximo:   cerca de 250/3850 de 4095
Errores CRC/formato:    0/0
Saltos de secuencia:    0
Tramas con alerta:      0
Resultado:              OK
```

Los extremos dependen de VREF, del error del ADC y DAC, de la carga y del cableado. No deberían
tomarse como una calibración.

## Abrir el visualizador

La aplicación usa `tkinter` y `pyserial`. No necesita NumPy, Matplotlib, Qt ni un entorno virtual.

En Ubuntu o Debian:

```bash
sudo apt install python3-tk
python3 -m pip install pyserial
```

Después:

```bash
python3 pc/scope_viewer.py --port /dev/ttyUSB0
```

También admite autodetección y una ventana inicial distinta:

```bash
python3 pc/scope_viewer.py --port auto --window-us 500
```

La interfaz permite:

- Elegir cuánto tiempo mostrar.
- Activar o desactivar el trigger ascendente.
- Ajustar el nivel y la histéresis del trigger.
- Activar la alineación temporal con resolución menor que un período de muestreo.
- Ver la frecuencia estimada, mínimo, máximo, frecuencia de muestreo y cantidad de tramas.
- Detectar CRC incorrectos, overrun del ADC, error DMA y saltos de secuencia.

El trigger solamente elige desde qué muestra dibujar y corrige el origen del eje temporal. La
histéresis y la validación de permanencia rechazan pulsos aislados. La alineación submuestra estima
en qué fracción del intervalo ocurrió el cruce y desplaza la traza completa. Ninguna de estas
técnicas altera, interpola ni filtra los valores del ADC. La línea verde se construye con tramos
horizontales y saltos verticales entre muestras.

## Protocolo binario

Cada trama empieza con una cabecera de 32 bytes en little endian:

| Offset | Tamaño | Campo |
|---:|---:|---|
| 0 | 4 | Firma ASCII `LPCS` |
| 4 | 1 | Versión, actualmente 1 |
| 5 | 1 | Banderas |
| 6 | 2 | Tamaño de cabecera, 32 |
| 8 | 4 | Número de secuencia |
| 12 | 4 | Frecuencia real de captura del ADC |
| 16 | 4 | Frecuencia de las muestras transmitidas |
| 20 | 4 | Frecuencia configurada de la señal del DAC |
| 24 | 2 | Cantidad de muestras |
| 26 | 1 | Bits por muestra, 12 u 8 |
| 27 | 1 | Canal ADC, actualmente 0 |
| 28 | 2 | Bytes de payload |
| 30 | 2 | CRC-16/CCITT de la cabecera 0 a 29 y el payload |

Después viene el payload. Las muestras de 12 bits se mandan como `uint16_t` little endian. Las de 8
bits ocupan un byte. No se usa texto porque desperdiciaría caudal y complicaría la resincronización.

## Uso de memoria medido por el linker

| Modo | Flash | RAM principal | AHB SRAM0 | AHB SRAM1 |
|---|---:|---:|---:|---:|
| Ventanas de 12 bits | 2328 B | 4168 B | 4100 B | 216 B |
| Continuo de 8 bits | 2436 B | 4192 B | 7204 B | 216 B |

La cifra de RAM principal incluye los 2 KiB mínimos reservados por el linker para el stack. No se usa
`malloc` y los buffers grandes son estáticos.

La ubicación de los buffers es obligatoria, no una optimización menor. En este micro GPDMA no puede
acceder a Flash ni a la SRAM principal de `0x10000000`. Por eso:

- Los buffers y descriptores del ADC viven en `.ahbram0`.
- La tabla y el descriptor del DAC viven en `.ahbram1`.
- La CPU copia la tabla inicial desde Flash a AHB SRAM antes de habilitar el DAC.

Si una tabla o un descriptor circular se deja como variable global normal, el programa compila pero
DMA lee datos inválidos en la placa.

## Qué se verificó en la placa disponible

Banco usado: LPCXpresso LPC1769 OM13085 a 100 MHz, CP2102 y UART0 a 921600 baud.

Se verificaron ambos modos de punta a punta:

- Firmware compilado y chequeado antes de grabar.
- Grabado y verificación por OpenOCD.
- Modo 1 con el puente AOUT a AD0.0: 101 tramas consecutivas, sin errores CRC, formato, DMA, ADC ni
  saltos de secuencia. La frecuencia medida fue 19.994,4 Hz, con un error de 0,03 % respecto de los
  20 kHz configurados.
- Modo 2: 112 tramas consecutivas, sin errores CRC, formato, DMA, ADC ni saltos de secuencia.
- El visualizador abrió y procesó las tramas sin errores.

Con el DAC trabajando a su máximo de 1 MS/s aparecieron algunas muestras aisladas en 0 o 4095. No se
ocultan ni se reemplazan. El trigger de la aplicación exige histéresis y permanencia del cruce para
que esos picos no desplacen horizontalmente toda la señal. Si se necesita eliminarlos físicamente,
hay que reducir la tasa del DAC, sincronizar mejor el instante ADC respecto de cada actualización o
agregar un filtro analógico adecuado.

## Límites de esta prueba

- El DAC genera 20 kHz con 50 escalones por período, no una senoide analógica ideal.
- En modo 1 la adquisición tiene tiempo muerto mientras sale cada trama.
- En modo 2 la adquisición es continua, pero 8 bits y 63,131 kS/s dejan muy pocos puntos por período.
- El gráfico sample and hold muestra lo recibido, pero no convierte a la placa en un osciloscopio
  calibrado.
- Para señales desconocidas hace falta un front-end analógico y un filtro anti-alias.
- Para transmitir continuamente los 12 bits a la velocidad completa del ADC conviene usar USB, no
  UART.
