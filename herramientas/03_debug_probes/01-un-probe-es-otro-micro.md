# Una sonda suele ser otro micro

Enchufás un cable USB y podés grabar y depurar. Parece una conexión directa con el target,
pero en muchas placas hay hardware activo haciendo de puente. Entenderlo ayuda a separar los
problemas de la aplicación, la sonda y el host.

---

## El LPC1769 no habla USB para grabarse

El chip tiene un periférico USB, sí, pero es para **tu aplicación**: si querés hacer un
teclado o un puerto serie virtual, ahí está. Ese periférico no sabe nada de grabar la FLASH,
y de hecho no arranca hasta que tu programa lo configura, con lo cual sería inútil para
grabar un chip vacío.

El LPC1769 sí incorpora un puerto SWD con las señales **SWDIO** y **SWCLK**. Mientras el
chip esté alimentado, fuera de reset y sin protecciones que bloqueen el acceso, por ahí se
llega al sistema de depuración y al bus
([parte 02](../02_protocolos_y_debug_en_el_chip/)).

El problema es que tu PC no tiene pines SWD. Tiene USB.

Hace falta un puente activo. En esta placa y en muchas sondas, ese puente es otro
microcontrolador con firmware propio.

```
   PC ──USB──►  ┌──────────────┐  ──SWD (2 pines)──►  ┌──────────┐
                │  el micro de │                      │ tu micro │
                │  la SONDA    │                      │ (target) │
                └──────────────┘                      └──────────┘
                       ▲
              corre un firmware cuyo unico
              trabajo es hacer esa traduccion
```

---

## En muchas placas de desarrollo hay dos micros

Cuando una placa incluye una sonda, suele estar soldada cerca del conector USB. A veces una
línea de corte, resistencias o jumpers permiten separarla del micro principal.

```
        ┌─────────────────── placa de desarrollo ─────────────────┐
        │                          ┊                              │
 USB ───┤   MICRO A     ── SWD ──► ┊    MICRO B                   │
        │  (la sonda)   SWDIO/SWCLK┊   (tu micro, el "target")    │
        │  su firmware  su alim.   ┊   tu firmware                │
        │                          ┊                              │
        └──────────────────────────┴──────────────────────────────┘
                       linea de corte / jumpers
```

Esa sonda integrada es una de las comodidades que puede aportar una placa de desarrollo.
Otras placas no la incluyen y dependen de un bootloader USB, de un conversor serie o de una
sonda externa.

En la placa de la cátedra, el micro A es un **LPC11U35** corriendo el firmware CMSIS-DAP de
ARM, y el micro B es el LPC1769. La sonda controla varias señales del target, como SWD, reset y, según la placa, UART, pero
no ejecuta la aplicación ni participa del build.

### No es una excentricidad de NXP

Es un diseño común, aunque no universal. Algunos ejemplos:

| Placa | El micro de la sonda | El firmware que corre |
|---|---|---|
| LPCXpresso LPC1769 rev D (la de la cátedra) | LPC11U35 | CMSIS-DAP |
| LPC-Link2 / LPCXpresso V2, V3 | LPC4322 | CMSIS-DAP, J-Link o redlink, según cuál le carguen |
| MCU-Link | LPC55S69 | CMSIS-DAP v2 |
| ST Nucleo, Discovery | STM32F103 o STM32F723 | ST-Link |
| Arduino Zero, muchas Atmel | SAM3U o SAMD21 | EDBG / CMSIS-DAP |
| Muchas placas NXP Kinetis | Kinetis K20 | OpenSDA (CMSIS-DAP, J-Link o P&E) |
| Raspberry Pi Debug Probe | RP2040 | `debugprobe` (CMSIS-DAP v2) |
| Micro:bit | Nordic o NXP, según versión | DAPLink |

Las sondas comerciales también contienen lógica activa: suele ser un microcontrolador,
aunque existen soluciones basadas en puentes USB como FTDI o en FPGA. En ningún caso es un
cable pasivo.

---

## Qué hace ese firmware

Es un programa embebido normal, con dos mitades:

**Del lado del USB:** una pila USB completa. Enumera, declara su clase (HID, o vendor
específico), declara su fabricante, su producto y su número de serie, y recibe paquetes de
comandos.

**Del lado del SWD:** una implementación del protocolo, muchas veces a puro *bit-banging*
sobre dos GPIO (mover los pines a mano en un lazo apretado), a veces apoyada en un SPI para
ir más rápido.

Y en el medio, un intérprete: *"llegó un comando `DAP_Transfer` pidiendo leer el registro
tal, hago la secuencia SWD, junto la respuesta, la mando de vuelta por USB"*.

Muchas sondas modernas agregan, con el mismo chip, funciones extra que no tienen nada que ver
con depurar:

| Función extra | Qué es |
|---|---|
| **VCOM** | un puerto serie virtual conectado a la UART del target. Con un solo cable USB tenés grabado, depuración **y** consola |
| **Disco USB** | aparece una unidad y grabás arrastrando el archivo (DAPLink, UF2) |
| **Medición de consumo** | un shunt y un ADC en la línea de alimentación del target (MCU-Link) |
| **Web** | una página que se abre desde el disco USB, con documentación de la placa |

---

## Las consecuencias prácticas

Acá está el motivo por el que vale la pena saber todo esto. Cada una de estas rarezas se
explica sola una vez que asumís que del otro lado hay una computadora.

### 1. Tiene firmware, y la versión define lo que podés hacer

Para usar SWO deben soportarlo el chip, el cableado, el hardware y el firmware de la sonda,
y el programa del host. La especificación CMSIS-DAP incorporó comandos SWO desde la versión
1.1, pero su implementación es opcional. La sonda de la cátedra no anuncia esa capacidad
([ver 07-01](../07_lpc1769/01-la-placa-y-su-sonda.md)).

### 2. Se puede colgar, y hay que reenchufarla

Un firmware embebido que se queda esperando el final de una transacción que nunca llegó se
cuelga, como cualquier programa. El síntoma es "funcionaba y de golpe dejó de aparecer".

El primer intento razonable es cerrar las herramientas que puedan estar usándola y
desenchufar y volver a enchufar el USB. Eso reinicia y vuelve a enumerar el micro de la
sonda. Si el problema se repite, hay que revisar drivers, permisos, cable y alimentación.

### 3. Se puede reprogramar, con su propio bootloader

Si la sonda usa un micro reprogramable, necesita algún mecanismo para cargar o actualizar su
firmware. Puede ser un bootloader en ROM, un bootloader protegido en FLASH o un segundo
puerto de depuración.

Antes de actualizarla hay que comprobar cuál es su vía de recuperación. Si el bootloader es
accesible, una imagen incorrecta suele poder reemplazarse. Si no lo es, recuperar la sonda
puede exigir otra sonda o equipamiento adicional.

Y una consecuencia divertida: **la sonda es, a su vez, un target.** Cuando le grabás firmware
a una sonda estás haciendo exactamente lo mismo que hacés con el LPC1769, un nivel más
arriba.

### 4. El límite de velocidad puede ser suyo, no del chip

En una implementación CMSIS-DAP v1 por USB HID de velocidad completa, los paquetes suelen
ser de 64 bytes y el intervalo USB puede dominar el caudal. En ese caso, subir `SWCLK` no
mejora una transferencia ya limitada por el host o la sonda.

Es medible: con la sonda de la cátedra, subir el reloj de SWD de 4 a 15 MHz **no cambia el
caudal de RTT ni un byte**. El cuello de botella está del lado del USB de la sonda, no en el
cable SWD ni en el LPC1769.

### 5. Tiene identidad USB, y a veces está mal

Como cualquier dispositivo USB, declara VID, PID, nombre y número de serie. Y como el
firmware lo escribió alguien, puede estar incompleto: la sonda de la cátedra declara su
**número de serie vacío**, y eso alcanza para que LinkServer se niegue a usarla, aunque la
detecte correctamente.

### 6. Se le pueden pisar los drivers

En Linux, el acceso depende de la clase USB, del backend usado por la herramienta y de los
permisos de usuario. Una sonda puede aparecer en `lsusb` y aun así no estar disponible para
OpenOCD. Antes de culpar al firmware, revisá las reglas de udev y cerrá otros procesos que
puedan haber reclamado la interfaz.

### 7. Solo la puede usar una herramienta a la vez

En general, una interfaz USB de la sonda solo puede ser reclamada por un proceso a la vez.
Si MCUXpresso mantiene una sesión abierta, OpenOCD puede no acceder. Algunos servidores
admiten varios clientes, pero siguen coordinando una única conexión física al target.

---

## Y si no hay sonda: el bootloader del propio chip

Queda la pregunta obvia: si para grabar hace falta un segundo micro, **¿quién grabó al
primero de todos?**

Muchos microcontroladores incluyen un **bootloader de fábrica** en ROM y otros se programan
por un puerto dedicado durante fabricación. El bootloader permite cargar firmware por UART
o USB sin una sonda de depuración. Es el camino B de
[cómo se graba un micro](../01_panorama/02-como-se-graba-un-micro.md), y es la red de
recuperación del LPC1769 y de otras familias que ofrecen una ROM similar.

---

## Lo que hay que recordar

- Una sonda es hardware activo entre el host y el target; muchas usan otro microcontrolador.
- Su firmware traduce comandos del host a SWD o JTAG y puede sumar UART, almacenamiento o
  medición.
- La versión de firmware, la interfaz USB y el hardware determinan funciones y caudal.
- Antes de actualizar una sonda, verificá cómo entrar a su modo de recuperación.
- **CMSIS-DAP** estandariza la comunicación entre el host y muchas sondas; es el tema del
  [capítulo que sigue](./02-cmsis-dap.md).

---

**Sondas:** [índice](./README.md) ·
**Siguiente:** [02 - CMSIS-DAP](./02-cmsis-dap.md)
