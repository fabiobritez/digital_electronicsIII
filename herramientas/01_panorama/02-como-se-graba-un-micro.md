# Cómo se graba un micro

Tenés una imagen de firmware y una memoria FLASH dentro del chip. Falta transferirla,
escribirla y comprobar que quedó bien. Esta página explica ese proceso.

El proceso es representativo de muchos microcontroladores, aunque los tamaños de borrado, los
algoritmos y las interfaces cambian entre familias. Lo específico del LPC1769 está en
[07 - LPC1769](../07_lpc1769/).

---

## Primero: la FLASH no se escribe como la RAM

Esta es la razón de que grabar sea un tema y no una asignación.

En la RAM podés sobrescribir una palabra directamente. La FLASH interna típica de un
microcontrolador impone otras reglas:

1. **Solo se puede pasar de 1 a 0 escribiendo.** Para volver un bit a 1 hay que **borrar**.
2. **El borrado es por sectores enteros**, no por palabra. Un sector típico va de 1 KB a
   64 KB. Si querés cambiar un byte en el medio de un sector, hay que borrar el sector
   completo.
3. **Escribir y borrar llevan tiempo y necesitan una secuencia de comandos**, no una
   escritura común. Y mientras la FLASH se está borrando, muchas veces **no se puede leer**,
   lo cual es un problema si el programa que ordena el borrado vive ahí.

De la tercera propiedad se desprende una consecuencia importante:

> En muchos micros, quien ejecuta la secuencia de escritura es el propio CPU: corre desde
> RAM o llama a rutinas de una ROM interna mientras la sonda le entrega los datos.

Y hay una cuarta propiedad, que no afecta al grabado pero conviene saber: la FLASH tiene un
**número finito de ciclos de borrado**, típicamente 10 000 a 100 000 por sector. Grabar cien
veces por día durante un cuatrimestre no la gasta ni de cerca. Escribir un contador en la
FLASH una vez por segundo, sí.

---

## Las dos familias de métodos

En desarrollo vas a encontrar dos familias principales para grabar un micro ya soldado:
usar el puerto de depuración o hacer que el propio micro ejecute un bootloader. En producción
también existen programadores dedicados, que se presentan más adelante.

```
   A) POR EL PUERTO DE DEPURACION            B) POR UN BOOTLOADER
                                                RESIDENTE

   PC ──USB──► sonda ──SWD/JTAG──► micro     PC ──USB/serie──► micro
                                                                │
   la sonda entra "por atrás", por un        el micro colabora: corre un
   puerto de hardware que existe             programa suyo que recibe los
   aunque el micro no corra nada             bytes y se graba a si mismo
```

La diferencia de fondo es **quién manda**:

- En **A**, el micro puede estar colgado, sin programa, o directamente parado. El puerto de
  depuración no depende de que tu firmware colabore. Por eso suele funcionar con la
  aplicación detenida, dañada o ausente, y además permite **depurar**. Puede dejar de estar
  disponible por protección de lectura, estados de bajo consumo o problemas eléctricos.
- En **B**, el micro ejecuta un programa que recibe el firmware y controla la FLASH. Puede
  estar en ROM de fábrica o en una zona protegida de la propia FLASH. Sirve para grabar y
  actualizar, pero por sí solo **no ofrece depuración**.

---

## Camino A: por el puerto de depuración

Es el del día a día en desarrollo, y el que se usa en esta materia.

### Qué hace falta

| Pieza | Qué es |
|---|---|
| Un **puerto de depuración** en el chip | Por ejemplo, JTAG o SWD en muchos ARM Cortex-M ([parte 02](../02_protocolos_y_debug_en_el_chip/)) |
| Una **sonda de depuración** (*debug probe*) | el hardware que traduce el USB de tu PC a ese puerto ([parte 03](../03_debug_probes/)) |
| Un **grabador** en la PC | el programa que le dice a la sonda qué hacer: OpenOCD, pyOCD, LinkServer, J-Link |

### Cómo funciona por dentro

Un grabado por sonda suele seguir esta secuencia:

1. La sonda usa el puerto de depuración para **frenar el CPU** del micro.
2. Le **escribe en la RAM** un programita chiquito llamado *flash loader* (OpenOCD lo llama
   *work area algorithm*; ARM lo estandarizó como archivo `.FLM`; NXP usa `.cfx`). Ese
   programita sabe hablarle al controlador de FLASH de ese chip en particular.
3. Le escribe en la RAM, también, un **buffer con un pedazo de tu firmware**.
4. **Arranca el flash loader**, que corre en el micro y copia el buffer a la FLASH.
5. Repite 3 y 4 hasta terminar, y al final resetea.

En este esquema, la sonda proporciona acceso y datos, pero el CPU del target ejecuta la
secuencia específica de su FLASH. Por eso no alcanza con que el grabador sepa hablar SWD:
también necesita conocer el chip y disponer del algoritmo correcto.

> En algunos chips, el fabricante ya incluyó rutinas de grabado en una ROM interna. El
> algoritmo cargado en RAM puede llamarlas en vez de manejar directamente el controlador de
> FLASH. En el LPC1769 se denominan **IAP** (*In-Application Programming*).

### Ventajas y límites

| A favor | En contra |
|---|---|
| Graba con el micro colgado, en blanco o parado | necesitás una sonda (aunque en las placas de desarrollo viene soldada) |
| Es el mismo camino que usás para **depurar** | requiere que el puerto de depuración esté habilitado (se puede deshabilitar por seguridad) |
| Rápido, y verifica lo escrito | los pines SWD/JTAG tienen que estar accesibles |

---

## Camino B: por un bootloader residente

Un **bootloader** es un programa que corre antes de la aplicación y puede recibir una imagen
nueva, validarla y escribirla en la FLASH. Suele vivir en una ROM de fábrica o en una región
protegida de la propia FLASH.

### Cómo se entra

Un bootloader que arranque siempre sería inútil: nunca correría tu programa. Así que en el
arranque tiene que **decidir** entre esperar un firmware nuevo o saltar al tuyo. Los
criterios habituales:

| Criterio | Ejemplo |
|---|---|
| Un **pin** en un nivel determinado durante el reset | LPC17xx (P2.10 en bajo), STM32 (BOOT0 en alto) |
| Un **botón** apretado al encender | RP2040 (BOOTSEL), muchas placas con botón "ISP" o "DFU" |
| **No hay firmware válido** en la FLASH | LPC17xx (checksum), y casi todos los bootloaders de aplicación |
| Un **comando** desde el firmware anterior | actualizaciones OTA, `reset to bootloader` por USB |
| Un **doble reset** rápido | Arduino nuevos, placas con UF2 |

### Por dónde recibe

| Transporte | Cómo se ve | Se usa en |
|---|---|---|
| **UART** (serie) | un puerto `/dev/ttyUSB0` o `COM3` | muchas familias LPC, STM32 y AVR |
| **USB DFU** | un dispositivo USB especial, se graba con `dfu-util` | STM32, muchos ARM |
| **USB "pendrive"** (UF2, MSD) | aparece un disco y arrastrás un archivo | RP2040, placas con DAPLink |
| **USB CDC** | un puerto serie virtual | Arduino |
| **CAN / Ethernet / radio** | actualización remota | automotriz, IoT |

El caso del disco USB es especialmente simple para el usuario: enchufás la placa, aparece
una unidad, arrastrás el `.uf2` y el bootloader se ocupa del resto. En los sistemas que
reconocen almacenamiento USB de forma nativa no hace falta instalar una herramienta de
grabado.

### Ventajas y límites

| A favor | En contra |
|---|---|
| Puede requerir solo una conexión USB o un adaptador USB-serie | **no permite depurar**: solo graba |
| Es cómo se actualiza un equipo que ya está instalado en el campo | más lento |
| Si está en ROM, el bootloader no se borra al actualizar la aplicación | hay que poder entrar: si el pin o la interfaz no están accesibles, no hay camino |

---

## Cómo cambia el grabado en producción

En **producción**, grabar una placa por vez con una estación de desarrollo no escala. Se
mantienen los mismos principios eléctricos, pero se usan equipos y flujos preparados para
automatizar y verificar el proceso:

- **Programadores de gang**: graban 4, 8 o 16 placas a la vez, sin PC, desde una tarjeta SD.
- **Chips pre-programados de fábrica**: le mandás el binario al fabricante o al distribuidor
  y te llegan los chips ya grabados.
- **JTAG boundary scan en cadena**: varios chips de una misma placa conectados en serie por
  JTAG, programados en una sola pasada. Es una de las razones por las que JTAG sigue vivo
  ([ver 02-01](../02_protocolos_y_debug_en_el_chip/01-jtag.md)).
- **Grabado en la línea de test (ICT)**: una cama de pinchos que contacta los puntos de test
  de la placa, hace las pruebas eléctricas y graba, todo en la misma estación.

No los vas a usar en la materia. Están acá para que sepas que existen y no te sorprenda el
día que veas una placa que llega ya funcionando de fábrica.

---

## La comparación, de una

| | Sonda de depuración | Bootloader |
|---|---|---|
| Hardware extra | una sonda (o la de a bordo de tu placa) | nada, o un adaptador USB-serie |
| ¿Permite depurar? | **sí** | no |
| ¿Anda con el micro colgado? | **sí** | no |
| ¿Anda con la FLASH vacía? | **sí** | sí, si el bootloader está en ROM |
| Velocidad | suele ser alta | depende del transporte y del bootloader |
| ¿Verifica lo grabado? | sí | según la herramienta |
| Se usa en | **desarrollo** | actualizaciones en el campo, producción simple, rescate |

**Para la materia, conviene usar la sonda.** Permite grabar y depurar con la misma conexión,
y sigue siendo útil cuando la aplicación falla. El bootloader queda como camino alternativo
y como base para entender las actualizaciones de firmware en equipos terminados.

---

**Panorama:** [índice](./README.md) ·
**Anterior:** [01 - El mapa completo](./01-el-mapa-completo.md) ·
**Siguiente:** [03 - Glosario](./03-glosario.md)
