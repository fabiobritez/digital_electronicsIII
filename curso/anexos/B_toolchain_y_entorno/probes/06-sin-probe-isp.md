# Sin debug probe: grabar por el puerto serie (ISP)

El LPC1769 se puede grabar **sin ningún debug probe**, con un adaptador USB-serial de dos
dólares. No es un truco ni un modo degradado: es una función de fábrica del chip, pensada
para actualizar equipos en el campo.

Lo que perdés es la depuración: por este camino solo grabás.

## Cómo funciona

Adentro del LPC1769, en `0x1FFF0000`, hay una **boot ROM** de 8 KB que NXP grabó en la
fábrica y que no se puede borrar. Al resetear, esa ROM corre **antes** que tu programa y
decide qué hacer:

```
        reset
          │
          ▼
    ¿P2.10 está en BAJO?
          │
    ┌─────┴─────┐
   sí           no
    │            │
    ▼            ▼
 modo ISP    ¿el checksum de los 8 vectores da 0?
 (espera          │
  por UART0)  ┌───┴───┐
              sí      no
              │        │
              ▼        ▼
        corre tu   modo ISP
        programa
```

Dos consecuencias que conviene tener claras:

1. **P2.10 en bajo durante el reset** fuerza el modo ISP. Ese es el "botón ISP" de las
   placas que lo tienen.
2. Aunque no fuerces nada, si el **checksum del vector 7** está mal, la boot ROM concluye
   que la FLASH está vacía y **se queda en ISP igual**. Es la causa del clásico "grabé y
   no hace nada". La [plantilla](../../../../plantilla/) lo inyecta en el build; verificalo
   con `make vectores`.

El modo ISP habla por **UART0** con un protocolo de texto documentado en el capítulo 32
del UM10360, y detecta la velocidad sola midiendo el primer carácter que le mandás
(*auto-baud*).

## Lo que necesitás

Un adaptador USB-serial de **3.3 V**. Los FTDI, CP2102, CH340 y PL2303 sirven todos.

> **Cuidado con los de 5 V.** Los pines del LPC1769 toleran 5 V cuando están como entrada
> digital, pero es mala idea de todos modos, y si el pin quedó configurado como salida o
> como entrada analógica, lo quemás. Verificá que tu adaptador tenga el jumper en 3.3 V.

### Cableado

| Adaptador | LPC1769 | Nota |
|-----------|---------|------|
| TX | **P0.3** (RXD0) | el TX del adaptador va al RX del micro |
| RX | **P0.2** (TXD0) | y viceversa. Es el error de cableado clásico |
| GND | GND | imprescindible |
| (no conectar) | 3V3 | alimentá la placa por su propio USB |

Y para entrar en modo ISP:

| Señal | Pin | Cuándo |
|-------|-----|--------|
| ISP | **P2.10** a GND | durante el reset y un instante después |
| RESET | pulsar el botón de reset | mientras P2.10 está en bajo |

La secuencia a mano: mantené P2.10 a masa, pulsá y soltá reset, esperá un segundo, soltá
P2.10. La boot ROM muestrea el pin hasta unos 3 ms después del reset.

## Grabar con lpc21isp

Es la herramienta abierta, disponible en todos los sistemas:

```bash
sudo apt install lpc21isp          # Ubuntu/Debian
```

```bash
lpc21isp -control build/firmware.hex /dev/ttyUSB0 115200 12000
```

Con la [plantilla](../../../../plantilla/):

```bash
make flash FLASHER=lpc21isp
make flash FLASHER=lpc21isp ISP_PORT=/dev/ttyUSB1
```

Los argumentos, uno por uno:

| Argumento | Qué es |
|-----------|--------|
| `-control` | usa las líneas RTS y DTR del adaptador para resetear y entrar a ISP **automáticamente**, sin tocar nada a mano. Solo funciona si la placa está cableada para eso; si no, sacalo y hacé la secuencia manual |
| `build/firmware.hex` | **Intel HEX**, no `.bin`. La plantilla lo genera solo |
| `/dev/ttyUSB0` | el puerto. En Windows sería `COM3` |
| `115200` | la velocidad. El bootloader la detecta sola, así que podés subirla |
| `12000` | la frecuencia del **cristal en kHz** (12 MHz en esta placa). La boot ROM la necesita para sus cuentas internas de escritura de FLASH. Si la ponés mal, la grabación puede salir corrupta |

## Grabar con FlashMagic (Windows, con ventanas)

[FlashMagic](https://www.flashmagictool.com/) es la versión gráfica de lo mismo. Elegís el
chip (LPC1769), el puerto COM, la velocidad, el cristal (12 MHz) y el archivo `.hex`, y
apretás Start. Tiene una opción para resetear la placa sola por DTR/RTS.

## Encontrar el puerto

```bash
# Linux: enchufá el adaptador y mirá qué apareció
ls /dev/ttyUSB* /dev/ttyACM*
dmesg | tail -5

# Windows: Administrador de dispositivos -> Puertos (COM y LPT)
```

En Linux, para usar el puerto sin `sudo` hay que estar en el grupo `dialout`:

```bash
sudo usermod -aG dialout $USER
```

Y **cerrar sesión y volver a entrar**: no alcanza con abrir otra terminal, los grupos se
leen al iniciar sesión.

## Depurar sin debugger

Es la limitación real de este camino. Las alternativas, en orden de utilidad:

1. **`printf` por la misma UART0.** Ya tenés el cable puesto: el adaptador que usás para
   grabar te sirve de consola. Está explicado en el
   [módulo 0, capítulo 16](../../../00_lenguaje_c/16-redirigir-printf-a-uart.md), y la
   plantilla ya deja el lugar preparado en
   [`src/syscalls.c`](../../../../plantilla/src/syscalls.c): alcanza con definir
   `__io_putchar()`.

   ```bash
   screen /dev/ttyUSB0 115200        # o picocom, minicom, cu
   ```

2. **LEDs.** Primitivo, pero para saber "por acá pasó" es instantáneo y no cuesta nada.

3. **Un analizador lógico barato.** Para I2C, SPI y UART resuelve la mayoría de los
   problemas, porque te muestra lo que salió de verdad por el cable. Ver el
   [módulo 17, página 03](../../../17_hardware_y_placa/03-instrumentos-de-medicion.md).

Todo esto está desarrollado en el [módulo 12 (debug)](../../../12_debug/).

## Problemas típicos

| Síntoma | Causa |
|---------|-------|
| `Can't synchronize with the target` | no entró en modo ISP: revisá P2.10 y la secuencia de reset |
| Lo mismo, con el cableado correcto | TX y RX cruzados al revés |
| Graba, pero la placa no arranca | el checksum del vector 7. `make vectores` |
| Funciona a veces sí y a veces no | ModemManager se está conectando al puerto. Las reglas de udev de la plantilla lo evitan |
| `Permission denied: /dev/ttyUSB0` | no estás en el grupo `dialout` |
| Grabación corrupta | pusiste mal la frecuencia del cristal (tiene que ser `12000`) |

## Cuándo elegir este camino

- Tu placa tiene el [probe LPC-Link viejo](./02-lpc-link-original.md) y no querés instalar
  software de NXP.
- Se rompió el probe de a bordo.
- Estás con un LPC1769 en una placa propia, sin conector de depuración.
- Querés entender cómo se actualiza el firmware de un equipo que ya está en el campo, que
  es exactamente este mecanismo.

---

**Probes:** [índice](./README.md) ·
**Anterior:** [05 - Otros probes](./05-otros-probes.md) ·
**Volver al** [anexo B](../README.md)
