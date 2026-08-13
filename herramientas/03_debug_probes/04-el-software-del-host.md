# El software del lado de la PC

La sonda sola no hace nada: espera comandos. Del lado de la PC hace falta un programa que
sepa **qué comandos mandar**, y ahí es donde entra el conocimiento del chip.

---

## La arquitectura común

La mayoría de los flujos de depuración separan GDB, un servidor y la sonda. Esa separación
permite reemplazar una capa cuando ambas caras hablan el mismo protocolo:

```
   ┌──────────┐   GDB Remote     ┌──────────────┐   USB    ┌───────┐  SWD  ┌────────┐
   │   gdb    │◄───Serial────────►│  gdbserver   │◄────────►│ sonda │◄─────►│ target │
   │ (o el IDE)│    Protocol      │              │          └───────┘       └────────┘
   └──────────┘   por TCP :3333   │ openocd      │
                                  │ pyocd        │
                                  │ LinkServer   │
                                  │ JLinkGDBServer│
                                  └──────────────┘
```

Tres capas, con una separación muy limpia:

| Capa | Qué sabe | Qué **no** sabe |
|---|---|---|
| **gdb** | tu programa: los símbolos, los tipos, qué línea de C es cada instrucción | nada de USB, de SWD ni de sondas |
| **gdbserver** | el núcleo, la sonda, los resets y los algoritmos de FLASH del target | interpreta direcciones y registros; recibe los símbolos principales desde GDB |
| **sonda** | cómo ejecutar SWD/JTAG y controlar sus señales | normalmente no conoce tu programa; algunas sondas integradas agregan funciones específicas de una placa |

Si el servidor soporta la nueva sonda, el firmware no necesita cambiar. Del mismo modo, GDB
puede trabajar con arquitecturas distintas usando la versión adecuada. Ante una falla, ubicar
la capa involucrada (GDB, servidor, USB, sonda o target) acota la búsqueda.

> El **GDB Remote Serial Protocol** usa paquetes simples que suelen viajar por TCP, aunque
> también puede usar un puerto serie. Para leer cuatro bytes en `0x10000000`, GDB envía un
> paquete cuyo contenido comienza con `m10000000,4`; el sufijo lleva un checksum.

---

## OpenOCD

OpenOCD es un proyecto libre con soporte para una amplia variedad de sondas y targets. Es el
servidor que usa este repositorio.

Se configura con comandos Tcl, normalmente guardados en archivos `.cfg`. Es habitual separar
la configuración de la **sonda** y la del **chip**, aunque también pueden combinarse o incluirse
desde otro archivo.

```bash
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg
```

Para grabar y salir, en un solo comando:

```bash
openocd -f interface/cmsis-dap.cfg -f target/lpc17xx.cfg \
        -c "program build/firmware.elf verify reset exit"
```

La estructura de un archivo de configuración propio, que es lo que hay en
[`plantilla/openocd/lpc1769.cfg`](../../plantilla/openocd/lpc1769.cfg):

```tcl
source [find interface/cmsis-dap.cfg]   # la sonda
transport select swd                    # 2 pines, no JTAG
set WORKAREASIZE 0x2000                 # RAM que le presto para el flash loader
source [find target/lpc17xx.cfg]        # el chip
adapter speed 1000                      # kHz
```

Al cambiar de sonda, normalmente se reemplaza el archivo de interfaz. También pueden cambiar
el transporte, la velocidad, el manejo de reset o alguna opción del driver, así que conviene
partir del archivo `.cfg` correspondiente en lugar de cambiar solo el nombre.

Además de grabar y de ser gdbserver, tiene una consola por telnet en el puerto 4444 y un
montón de comandos útiles sueltos:

```bash
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"        # ¿dónde está parado el PC?
openocd -f openocd/lpc1769.cfg -c "init; mdw 0x2009C000; exit"  # leer una dirección
```

También incorpora soporte para **RTT**, que es lo que usa `make rtt`
([ver 06-04](../06_depurar_en_serio/04-consola-por-el-debugger-rtt.md)).

**Cuándo elegirlo:** cuando soporta la combinación de sonda y target y querés un flujo
abierto, configurable y fácil de automatizar.

---

## pyOCD

Lo mismo, en Python, especializado en Cortex-M. Se instala con `pip` y es más simple de
arrancar:

```bash
python -m pip install pyocd
pyocd list
pyocd flash -W -t lpc1768 build/firmware.hex
pyocd gdbserver -t lpc1768
```

A favor:

- Instalación trivial, sin compilar nada.
- Puede instalar **CMSIS-Packs** para incorporar descripciones y algoritmos de FLASH sin
  escribir un `.cfg`: por ejemplo, `pyocd pack install LPC1769`.
- Muy cómodo para scripts, porque es una biblioteca de Python además de un comando.

En contra:

- Está orientado a sistemas Arm y su soporte depende del tipo de núcleo y del pack.
- Para sondas CMSIS-DAP por HID necesita un backend HID funcional. Según el sistema y el modo
  de instalación, puede hacer falta instalar `hidapi` o permisos adicionales; una falla en
  esa capa suele verse como "no se encontró ninguna sonda".
- **No parchea el checksum de la boot ROM de los LPC**, que es una diferencia con OpenOCD que
  cuesta caro si no la sabés
  ([ver 07-02](../07_lpc1769/02-la-boot-rom-el-isp-y-el-checksum.md)).

El `-W` de `pyocd flash` evita esperar indefinidamente a que aparezca una sonda. Es útil
en scripts y en la verificación inicial de una instalación.

---

## LinkServer (NXP)

El grabador nativo de NXP. Se instala **junto con MCUXpresso pero como paquete separado**, así
que se puede usar desde la terminal sin abrir el IDE:

```bash
LinkServer probes                              # ver las sondas conectadas
LinkServer devices | grep 1769                 # ¿está soportado el chip?
LinkServer flash LPC1769 load build/app.axf    # grabar
LinkServer flash LPC1769 verify build/app.axf
LinkServer gdbserver LPC1769                   # servidor gdb
```

Soporta las sondas NXP previstas por la herramienta y varios dispositivos CMSIS-DAP. La
compatibilidad exacta depende de la versión de LinkServer y de los descriptores USB que
reporte la sonda.

> **Con la placa de la cátedra no funciona.** Su sonda declara el descriptor USB `iSerial`
> vacío y LinkServer le pasa ese serial vacío a su motor de grabado, que corta con
> `Ee(E1). Probe serial number not found`. Está probado y documentado en
> [07-06](../07_lpc1769/06-primer-grabado-verificado.md). Para esa placa, usá OpenOCD.

Por dentro son tres programas encadenados (`redlinkserv` habla con la sonda,
`crt_emu_cm_redlink` hace de gdbserver, y unos drivers de FLASH `.cfx` que se cargan **en la
RAM del target**). El detalle está en
[07-07](../07_lpc1769/07-mcuxpresso-por-dentro.md), y muestra que, aun con una implementación cerrada, se repiten las capas de servidor, acceso
a la sonda y algoritmo de FLASH.

---

## Las herramientas de SEGGER

Para J-Link. Se bajan del sitio de SEGGER, hay `.deb` para Ubuntu.

```bash
# grabar, interactivo
JLinkExe -device LPC1769 -if SWD -speed 4000
J-Link> loadfile build/firmware.hex
J-Link> r
J-Link> g

# servidor gdb
JLinkGDBServer -device LPC1769 -if SWD -speed 4000
```

Dos detalles:

- `JLinkGDBServer` escucha en el puerto **2331**, no en el 3333 como OpenOCD.
- El `-device LPC1769` no es decorativo: con ese nombre SEGGER sabe cómo es la FLASH del chip
  y **calcula solo el checksum del vector 7**.

El paquete también incluye `JLinkRTTClient` para el mecanismo RTT de SEGGER.

---

## Otras que vale la pena conocer

| Herramienta | Qué es |
|---|---|
| **probe-rs** | el equivalente moderno escrito en Rust. Soporta CMSIS-DAP, J-Link y ST-Link, y trae `cargo-flash` y `cargo-embed`. Muy usado en el mundo Rust embebido |
| **Black Magic Probe** | no necesita nada en la PC: el gdbserver corre adentro de la sonda |
| **lpc21isp / FlashMagic / dfu-util / stm32flash** | grabadores por **bootloader**, no por sonda. Otra familia entera ([ver 01-02](../01_panorama/02-como-se-graba-un-micro.md)) |

---

## Y arriba de todo esto, la interfaz gráfica

Un IDE agrega configuración, generación de archivos y una interfaz gráfica, pero en general
termina coordinando las mismas capas: build, servidor y GDB.

En una configuración típica de VSCode con Cortex-Debug, al apretar **F5** ocurre una secuencia
parecida a esta:

1. corre la tarea de build (compila),
2. lanza OpenOCD o pyOCD como gdbserver,
3. arranca gdb y lo conecta al puerto 3333,
4. le manda `load` para grabar el firmware,
5. pone un breakpoint en `main` y hace `continue`.

Es literalmente lo que harías a mano:

```bash
# terminal 1
openocd -f openocd/lpc1769.cfg

# terminal 2
gdb-multiarch build/firmware.elf
(gdb) target extended-remote :3333
(gdb) load
(gdb) break main
(gdb) continue
```

MCUXpresso agrega pasos y archivos propios, pero mantiene la misma separación general y usa
`crt_emu_cm_redlink` como parte de su cadena de depuración.

El punto no es memorizar cinco comandos, sino poder reconocer qué herramienta lanzó el IDE,
con qué argumentos y en qué capa apareció el error.

---

## Cómo elegir

| Tu situación | Usá |
|---|---|
| Flujo preparado por la plantilla de esta materia | **OpenOCD** |
| Preferís una herramienta instalable desde Python y tu target está soportado | pyOCD |
| Tenés MCUXpresso y una sonda con número de serie | LinkServer, si te resulta cómodo |
| Tenés un J-Link y querés su máximo rendimiento | las de SEGGER |
| No tenés sonda | un grabador por bootloader (`lpc21isp`) |

---

**Sondas:** [índice](./README.md) ·
**Anterior:** [03 - Catálogo de sondas](./03-catalogo-de-probes.md) ·
**Siguiente:** [05 - Armarte tu propia sonda](./05-armarte-tu-propia-sonda.md)
