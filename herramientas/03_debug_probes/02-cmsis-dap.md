# CMSIS-DAP

CMSIS-DAP permite usar la sonda de la placa de la cátedra con herramientas abiertas como
OpenOCD y pyOCD. Para entender sus alcances conviene separar la especificación de comandos,
el transporte USB y el firmware concreto de cada sonda.

---

## Qué significa la sigla

**CMSIS-DAP** = **CMSIS** + **DAP**, y cada mitad viene de un lado distinto:

| Parte | Significa | Qué es |
|---|---|---|
| **CMSIS** | *Common Microcontroller Software Interface Standard* | el paraguas de estándares de software que publica **ARM** para todos los Cortex-M |
| **DAP** | *Debug Access Port* | el bloque de hardware adentro del chip por el que entra el depurador ([ver 02-03](../02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md)) |

Juntas, las siglas nombran una interfaz estandarizada por Arm para que un programa del host
controle una unidad de depuración y, a través de ella, acceda al DAP del target.

> **Cuidado con una confusión muy común en esta materia.** El curso usa "CMSIS" todo el
> tiempo para otra cosa: la biblioteca con `LPC17xx.h`, `NVIC_EnableIRQ()` y los drivers de
> periféricos. Esa es **CMSIS-Core** y corre en **tu** micro.
>
> Una implementación de **CMSIS-DAP corre en la sonda**. CMSIS-Core, en cambio, forma parte
> del software del target. Pertenecen a la misma familia de estándares de Arm, pero resuelven
> problemas distintos.

---

## Qué define exactamente

CMSIS-DAP define un **juego de comandos entre el host y la unidad de depuración** y ofrece
código de referencia para implementar el firmware. Esos comandos pueden transportarse por
USB HID en la versión 1.x o por endpoints bulk en la 2.x.

Los principales grupos de comandos son:

| Comando | Para qué |
|---|---|
| `DAP_Info` | consultar identificación, versión, capacidades y tamaño de paquete |
| `DAP_Connect` / `DAP_Disconnect` | seleccionar y liberar SWD o JTAG |
| `DAP_SWJ_Sequence` | generar secuencias sobre SWDIO/TMS y SWCLK/TCK |
| `DAP_SWJ_Clock` | solicitar la frecuencia del reloj de depuración |
| `DAP_Transfer` | leer o escribir registros del DP y de los AP |
| `DAP_TransferBlock` | repetir transferencias sobre un registro; con el autoincremento del AP permite mover bloques de memoria |
| `DAP_SWO_*` | configurar y recuperar traza SWO, si la sonda implementa esa capacidad |
| `DAP_ResetTarget` | solicitar una secuencia de reset del target |

La capa CMSIS-DAP trabaja con el sistema de depuración, no con sectores de FLASH ni con
periféricos específicos. OpenOCD o pyOCD conocen el target, cargan el algoritmo de FLASH y
traducen operaciones de alto nivel a accesos del DAP.

> Una misma sonda CMSIS-DAP puede usarse con muchos targets compatibles, pero la cadena
> completa también debe respetar tensión, conector, transporte y soporte del chip en la
> herramienta del host.

---

## v1.x y v2.x: cambia el transporte USB

La diferencia principal es cómo viajan los comandos:

| | **CMSIS-DAP v1.x** | **CMSIS-DAP v2.x** |
|---|---|---|
| Interfaz USB | HID | clase vendor con endpoints bulk |
| Estado | obsoleta para diseños nuevos, pero todavía soportada | recomendada para diseños nuevos |
| Paquete típico en USB full-speed | 64 bytes | 64 bytes, con varias transferencias en vuelo |
| USB high-speed | depende de la implementación HID | hasta 512 bytes por endpoint bulk |
| Driver en Windows | HID del sistema | WinUSB si los descriptores están bien configurados |
| SWO | disponible desde v1.1 como extensión opcional y leído por comandos | opcional; puede sumar un endpoint bulk para *streaming* |
| Caudal | limitado por HID y por la cantidad de paquetes | normalmente mayor, sobre todo por el *pipelining* |

Por eso **v2 no significa automáticamente "tiene SWO"**. La sonda debe incluir el circuito de
captura, habilitarlo en el firmware y anunciar la capacidad mediante `DAP_Info`.

### Cómo saber cuál tenés

En Linux, inspeccioná las interfaces USB:

```bash
lsusb -d <VID:PID> -v | grep -iE "bInterfaceClass|iInterface|iProduct|iSerial"
```

Una interfaz HID (`bInterfaceClass 3`) corresponde al transporte v1.x. El transporte v2.x
usa una interfaz vendor (`bInterfaceClass 255`) y su cadena de producto o interfaz suele
incluir `CMSIS-DAP v2`. El campo `FW Version` que muestra OpenOCD identifica el firmware
del producto; no alcanza por sí solo para deducir el transporte.

Las capacidades son una comprobación separada. Si la herramienta informa
`SWO-UART supported` o un modo de *streaming*, la sonda implementa SWO. La de la cátedra no
lo anuncia.

---

## Por qué importa que sea una interfaz pública

Una especificación pública permite que distintas herramientas implementen el mismo protocolo.
La compatibilidad concreta depende de la versión de la sonda y del soporte del programa:

| | CMSIS-DAP | Protocolo propietario |
|---|---|---|
| OpenOCD | sí | solo si el fabricante lo documentó o alguien lo hizo ingeniería inversa |
| pyOCD | sí | casi nunca |
| Keil, IAR | sí | solo con el suyo |
| Rust (probe-rs), Zephyr, Arduino | sí | según |
| ¿Exige el IDE del fabricante? | no; podés elegir una herramienta compatible | depende del protocolo y de las herramientas disponibles |
| Mantenimiento futuro | varias implementaciones pueden continuar | depende más del fabricante o de ingeniería inversa |

Ese último punto no es teórico. La primera generación de LPCXpresso traía una sonda con
un protocolo propietario. OpenOCD y pyOCD no lo implementan, por lo que depende de versiones
compatibles del software de NXP ([ver 07/probes/01](../07_lpc1769/probes/01-lpc-link-original.md)).
La rev. D, en cambio, puede usar CMSIS-DAP y conserva más alternativas de software.

---

## Cómo se carga CMSIS-DAP en una sonda

Como es firmware de un microcontrolador, se graba igual que cualquier otro firmware
([ver 03-01](./01-un-probe-es-otro-micro.md)). Lo que cambia es cómo se entra al bootloader
de ese micro, y eso depende de cada sonda:

| Sonda | Cómo se le carga el firmware |
|---|---|
| **LPC-Link2 / LPCXpresso V2, V3** | jumper de DFU puesto al enchufar, y correr `program_CMSIS` de **LPCScrypt** |
| **MCU-Link** | jumper ISP puesto al enchufar, y el `program_CMSIS` de su instalador |
| **Raspberry Pi Pico** | apretar **BOOTSEL** al enchufar, aparece un disco, y arrastrar el `.uf2` de `debugprobe` |
| **Placas con DAPLink** | aparece un disco `MAINTENANCE`, y arrastrar el `.hex` |
| **Sondas basadas en LPC11U3x** | puentear el pin de ISP y resetear: aparece un disco `CRP DISABLD` con un `firmware.bin` adentro |
| **Clones genéricos (STM32F103)** | por su propio SWD, o por el bootloader serie del STM32 |

Antes de actualizar una sonda, separá físicamente cuál chip vas a programar y comprobá el
método de recuperación. Una actualización correcta no debería modificar la FLASH del target,
pero una imagen o un cableado equivocados pueden dejar la sonda sin funcionar o manejar sus
pines de forma inesperada.

El paso seguro es entrar al bootloader **antes** de borrar o grabar nada y verificar que la
imagen corresponde exactamente al hardware. Si no podés confirmar ambas cosas, no sigas: una
recuperación podría requerir otra sonda.

> Esto se intentó de verdad con la placa de la cátedra y **no se pudo entrar al bootloader**.
> La historia completa, con lo que se aprendió del intento fallido, está en
> [07-01](../07_lpc1769/01-la-placa-y-su-sonda.md). Es un buen ejemplo de por qué el paso cero
> existe.

---

## DAPLink: una implementación con funciones extra

**DAPLink** es un proyecto abierto de firmware para sondas. Usa CMSIS-DAP para depuración y
puede agregar otras interfaces:

| Interfaz | Qué aporta |
|---|---|
| **HID o bulk** | comandos CMSIS-DAP |
| **Almacenamiento masivo** | grabado por arrastrar un archivo compatible con el target |
| **Puerto serie (CDC)** | puente entre el host y la UART del target |

La función de almacenamiento no forma parte de CMSIS-DAP: DAPLink agrega conocimiento de la
placa y algoritmos de FLASH. Tampoco toda sonda CMSIS-DAP ejecuta DAPLink ni toda compilación
de DAPLink habilita las tres interfaces. La micro:bit es un ejemplo conocido de placa que lo
utiliza.

---

## Lo que hay que recordar

- **CMSIS-DAP** define comandos y ofrece firmware de referencia para unidades de depuración.
- CMSIS-Core corre en el target; CMSIS-DAP se implementa en la sonda.
- El transporte v1.x usa HID. El v2.x usa endpoints bulk y permite mayor caudal.
- SWO es una capacidad opcional en ambas ramas; v2 puede transmitirlo por un endpoint
  dedicado.
- OpenOCD, pyOCD y otras herramientas pueden usar una sonda CMSIS-DAP, siempre que también
  soporten el target y el sistema operativo pueda acceder al dispositivo.
- DAPLink es un proyecto aparte que combina CMSIS-DAP con funciones opcionales como
  almacenamiento masivo y puerto serie.

---

**Sondas:** [índice](./README.md) ·
**Anterior:** [01 - Una sonda es otro micro](./01-un-probe-es-otro-micro.md) ·
**Siguiente:** [03 - Catálogo de sondas](./03-catalogo-de-probes.md)
