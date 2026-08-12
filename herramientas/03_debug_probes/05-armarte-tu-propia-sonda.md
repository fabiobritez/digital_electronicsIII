# Armarte tu propia sonda

Como muchas sondas son microcontroladores con firmware específico
([ver 03-01](./01-un-probe-es-otro-micro.md)), algunas placas pueden reutilizarse para esa
función. No alcanza con tener USB y dos GPIO: también hacen falta temporizado suficiente,
buffers, niveles eléctricos compatibles y una implementación preparada para ese hardware.

Esto sirve para tres cosas concretas:

- La sonda de a bordo de tu placa se rompió o no la podés usar.
- Querés una sonda CMSIS-DAP v2 con más caudal que una implementación HID antigua.
- Querés una segunda sonda para no tocar la que funciona.

---

## Una opción sencilla: Raspberry Pi Pico

El proyecto oficial `debugprobe` de Raspberry Pi permite reutilizar una Pico o Pico 2. El
resultado expone **CMSIS-DAP v2 para SWD** y un puerto serie virtual. No ofrece captura SWO.

### Cómo se graba

Es el ejemplo de libro de un bootloader por disco USB:

1. Apretá y mantené el botón **BOOTSEL** de la Pico.
2. Enchufala al USB sin soltarlo.
3. Aparece una unidad llamada `RPI-RP2`.
4. Arrastrá el archivo `debugprobe_on_pico.uf2` adentro.
5. La unidad desaparece sola y la Pico se reinicia ya convertida en sonda.

El firmware se baja de las *releases* de
[`raspberrypi/debugprobe`](https://github.com/raspberrypi/debugprobe). Para cargar el firmware no hace falta una utilidad de grabado; para usar la sonda sí
necesitás OpenOCD, pyOCD u otra herramienta compatible.

El modo BOOTSEL de la RP2040 vive en ROM y no se sobrescribe al copiar el UF2. Si cargás una
imagen incorrecta, normalmente podés volver a BOOTSEL y reemplazarla.

### Cómo se conecta al LPC1769

| Pin de la Pico | Señal | Al LPC1769 |
|---|---|---|
| GP2 | SWCLK | pin **TCK/SWDCLK** |
| GP3 | SWDIO | pin **TMS/SWDIO** |
| GND (cualquiera) | GND | GND |
| GP4 | TX de la UART | RXD0 (P0.3), opcional |
| GP5 | RX de la UART | TXD0 (P0.2), opcional |

En la placa de la cátedra esas señales salen al **conector Cortex de 10 pines**. Antes de
conectar una sonda externa, aislá la sonda de a bordo según el esquema de la placa. Alimentá
el target por su vía normal, uní primero GND y verificá que ambas interfaces trabajen a 3,3 V.
La Pico no usa VTref para adaptar sus niveles.

Con eso, el uso es idéntico al de cualquier otra CMSIS-DAP:

```bash
make flash
make debug
```

Y como bonus, los pines GP4/GP5 te dan la UART del target por el mismo cable USB, sin
comprar un conversor.

---

## Otras opciones

### DAPLink en una placa que tengas

**DAPLink** ([ver 03-02](./02-cmsis-dap.md)) soporta decenas de micros como plataforma. Si
tenés una placa con un STM32F103, un LPC11U35, un SAMD21 o similar dando vueltas, es
candidata.

El trabajo acá es más: hay que conseguir (o compilar) una imagen construida **para esa placa
en particular**, porque la asignación de pines cambia entre diseños. Una imagen de otra placa
con el mismo chip enumera bien pero puede no hablar con el target, que es el peor de los
mundos: parece que anda y no anda.

### Black Magic Probe en otra placa

El firmware de la [Black Magic Probe](https://black-magic.org/) es abierto y se puede cargar
en varias placas comunes (STM32F103 "Blue Pill", ST-Link V2 clones, y otras). Lo interesante
es que te deja **sin ninguna dependencia en la PC**: el gdbserver corre adentro de la sonda y
te conectás con gdb a un puerto serie.

### Una Raspberry Pi de las grandes, sin hardware extra

OpenOCD puede hacer *bit-banging* de SWD directamente sobre los GPIO de una Raspberry Pi, sin
ningún hardware en el medio:

```bash
openocd -f interface/raspberrypi-native.cfg -f target/lpc17xx.cfg
```

El rendimiento y los archivos disponibles dependen del modelo de Raspberry Pi, del sistema
operativo y de cómo fue compilado OpenOCD. Puede servir en un banco fijo, pero exige revisar
niveles eléctricos y asignación de GPIO.

### Un módulo FT2232

Los chips FTDI de doble canal tienen un modo (**MPSSE**) que hace JTAG y SWD por software.
Los módulos genéricos se consiguen baratísimos. La contra: hay que escribir el archivo de
configuración de OpenOCD con la asignación de pines de tu módulo, y eso ya es trabajo de
verdad.

---

## Comparación

| Opción | Costo | Trabajo | CMSIS-DAP | SWO | UART incluida |
|---|---|---|:---:|:---:|:---:|
| **Raspberry Pi Pico** | bajo | arrastrar un archivo y cablear | v2 | no | sí |
| DAPLink en una placa compatible | variable | conseguir o compilar la imagen exacta | v1 o v2 | según hardware | según firmware |
| Black Magic en una placa compatible | variable | conseguir o compilar la imagen exacta | no aplica | según plataforma | según plataforma |
| Raspberry Pi por GPIO | si ya la tenés | configurar OpenOCD y cablear | no aplica | no | no |
| Módulo FT2232 | bajo | cablear y escribir el `.cfg` | no | no habitual | puede usar el segundo canal |
| **MCU-Link** | comercial | instalar o actualizar el firmware adecuado | v2 | según modelo | según modelo |

---

## La recomendación para esta materia

Si necesitás una segunda sonda para SWD, una Pico con `debugprobe` es una alternativa
accesible si ya tenés la placa y podés resolver el cableado a 3,3 V. Si además necesitás
captura SWO, reset controlado o adaptación de niveles mediante VTref, elegí una sonda que
declare explícitamente esas funciones; CMSIS-DAP v2 por sí solo no las garantiza.

En una placa compartida conviene agregar una sonda externa antes que modificar la única sonda
que ya funciona. El detalle del caso de la cátedra está en
[07-01](../07_lpc1769/01-la-placa-y-su-sonda.md).

---

**Sondas:** [índice](./README.md) ·
**Anterior:** [04 - El software del lado de la PC](./04-el-software-del-host.md) ·
**Siguiente parte:** [04 - Toolchains](../04_toolchains/)
