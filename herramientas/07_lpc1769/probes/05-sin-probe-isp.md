# Sin debug probe: grabar por el puerto serie (ISP)

El LPC1769 puede programarse sin sonda mediante un adaptador USB-serie y el ISP de su boot ROM. Es
una vía de fábrica útil para producción, servicio o recuperación, siempre que el hardware exponga
UART0, reset y P2.10.

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
   no hace nada". La [plantilla](../../../plantilla/) lo inyecta en el build; verificalo
   con `make vectores`.

El modo ISP habla por **UART0** con un protocolo de texto documentado en el capítulo 32
del UM10360, y detecta la velocidad sola midiendo el primer carácter que le mandás
(*auto-baud*).

## Lo que necesitás

Un adaptador USB-serie cuya **tensión lógica** sea compatible con 3,3 V. El nombre del chip
(FTDI, CP2102, CH340 o PL2303) no alcanza: distintos módulos cablean de manera diferente la
alimentación y los niveles.

> No conectes señales TX de 5 V por asumir tolerancia del pin. Seleccioná 3,3 V y medí el nivel si el
> módulo no está documentado. El jumper de alimentación no siempre cambia la tensión lógica.

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

lpc21isp es una herramienta abierta disponible en varias distribuciones y con builds para distintos
sistemas:

```bash
sudo apt install lpc21isp          # Ubuntu/Debian
```

```bash
lpc21isp -control build/firmware.hex /dev/ttyUSB0 115200 12000
```

Con la [plantilla](../../../plantilla/):

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
| `115200` | velocidad solicitada; el autobaud la detecta dentro de los rangos admitidos |
| `12000` | frecuencia del oscilador en kHz usada por la herramienta; confirmala en el esquemático de tu placa |

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

1. **`printf` por la misma UART0.** Cerrá lpc21isp, liberá P2.10 y reseteá para ejecutar la
   aplicación. Después el mismo adaptador puede funcionar como consola. Está explicado en el
   [módulo 0, capítulo 16](../../../curso/00_lenguaje_c/16-redirigir-printf-a-uart.md), y la
   plantilla ya deja el lugar preparado en
   [`src/syscalls.c`](../../../plantilla/src/syscalls.c): alcanza con definir
   `__io_putchar()`.

   ```bash
   screen /dev/ttyUSB0 115200        # o picocom, minicom, cu
   ```

2. **LEDs o GPIO.** Permiten marcar etapas, pero consumen un pin, instrucciones y tiempo.

3. **Un analizador lógico u osciloscopio.** Muestra señales y tiempos del bus; no revela por sí solo
   qué camino tomó el firmware. Ver el
   [módulo 17, página 03](../../../curso/17_hardware_y_placa/03-instrumentos-de-medicion.md).

Todo esto está desarrollado en [06 - Depurar en serio](../../06_depurar_en_serio/).

## Problemas típicos

| Síntoma | Causa |
|---------|-------|
| `Can't synchronize with the target` | no entró en modo ISP: revisá P2.10 y la secuencia de reset |
| No sincroniza con P2.10 correcto | verificá GND común y que TX vaya a RX, no TX a TX |
| Graba, pero la placa no arranca | ejecutá `make preflight` y revisá reset, checksum, clocks y aplicación |
| El puerto se abre solo o cambia de estado | comprobá servicios como ModemManager y las reglas udev |
| `Permission denied: /dev/ttyUSB0` | revisá grupo `dialout`, ACL y la sesión actual |
| La programación falla durante el borrado/escritura | verificá frecuencia, alimentación, baudrate y señal serie |

## Cuándo elegir este camino

- Tu placa tiene el [LPC-Link vieja](./01-lpc-link-original.md) y no querés instalar
  software de NXP.
- Se rompió el probe de a bordo.
- Estás con un LPC1769 en una placa propia, sin conector de depuración.
- Querés estudiar el ISP usado en fabricación o servicio. Para actualización en campo suelen
  necesitarse además controles de integridad, recuperación y seguridad.

---

**Guía por sonda:** [índice](./README.md) ·
**Anterior:** [04 - Otras sondas](./04-otros-probes.md) ·
**Volver a** [LPC1769](../README.md)
