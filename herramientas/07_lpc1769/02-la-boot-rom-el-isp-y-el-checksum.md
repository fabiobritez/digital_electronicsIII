# La boot ROM, el ISP y el checksum

Una grabación verificada confirma que los bytes se escribieron, pero no que la boot ROM los acepte
como una aplicación válida. En el LPC1769, el checksum de la tabla de vectores es una de las primeras
condiciones que conviene comprobar.

---

## Lo primero: tu programa no es lo primero que corre

Cuando el LPC1769 sale del reset **no salta a tu código**. Antes corre la **boot ROM**: 8 KB
que NXP grabó en fábrica en `0x1FFF0000` y que **no se pueden borrar por ningún medio**.

Entre sus funciones están:

1. decidir si ejecuta la aplicación o entra al manejador **ISP**;
2. implementar el protocolo ISP sobre UART0;
3. ofrecer rutinas **IAP** para borrar y programar la FLASH desde una aplicación.

Al programar mediante una sonda, la PC no escribe la FLASH como si fuera una memoria USB. El servidor
carga y ejecuta un algoritmo en el target; según la herramienta, ese algoritmo puede apoyarse en las
rutinas IAP de la ROM.

---

## El árbol de decisión del arranque

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
 modo ISP    ¿la suma de las 8 primeras palabras
 (espera      de la tabla de vectores da CERO?
  por UART0)       │
              ┌────┴────┐
             sí         no
              │          │
              ▼          ▼
         corre tu    modo ISP
         programa    (¡y parece que no hace nada!)
```

Dos consecuencias, y la segunda es la que arruina tardes:

1. **P2.10 en bajo durante el reset solicita el modo ISP.** La boot ROM puede muestrearlo hasta unos
   3 ms después del flanco de reset. El manual documenta una excepción cuando está activo el flag de
   desborde del watchdog. Como el pin queda en alta impedancia después del reset, la placa debe
   mantenerlo en un nivel definido.
2. **Si el checksum no identifica código válido, la ROM inicia la rutina de autobaud de UART0.**
   Desde afuera puede parecer que el programa quedó detenido, aunque el núcleo esté esperando la
   sincronización ISP.

---

## El checksum del vector 7

### Qué exige el chip

La boot ROM suma las **primeras 8 palabras de 32 bits** de la tabla de vectores (o sea, los
primeros 32 bytes de la FLASH) y exige que el resultado, en aritmética de 32 bits con
desborde, **dé cero**.

Las primeras 7 palabras son cosas que ya están definidas:

| Palabra | Offset | Contenido |
|---|---|---|
| 0 | `0x00` | valor inicial del stack pointer |
| 1 | `0x04` | `Reset_Handler` |
| 2 | `0x08` | `NMI_Handler` |
| 3 | `0x0C` | `HardFault_Handler` |
| 4 | `0x10` | `MemManage_Handler` |
| 5 | `0x14` | `BusFault_Handler` |
| 6 | `0x18` | `UsageFault_Handler` |
| **7** | **`0x1C`** | **reservado por ARM: acá va el checksum** |

O sea que la única forma de que la suma dé cero es poner en la palabra 7 el **complemento a
dos** de la suma de las otras siete. ARM declara esa posición como "reservada", y NXP la usó
para esto.

### Por qué es tan traicionero

Porque el comportamiento depende de la herramienta y de cómo se la invoque. OpenOCD, lpc21isp,
J-Link o un proyecto generado por MCUXpresso pueden calcular o parchear el vector en determinados
flujos; una escritura genérica de bytes no tiene por qué hacerlo. Ese comportamiento también puede
cambiar entre versiones y formatos de archivo.

Si el build ya genera una imagen válida, el resultado no depende de que el programador la modifique
durante la grabación.

### Cómo lo resuelve la plantilla

Inyectándolo **en tiempo de compilación**, con
[`tools/lpc_checksum.py`](../../plantilla/tools/lpc_checksum.py). Así, el ELF que sale del build ya contiene el valor correcto y cualquier herramienta que respete la
imagen recibe los mismos bytes.

```bash
cd plantilla
make vectores      # muestra la tabla de vectores y verifica que la suma dé cero
```

También evita diferencias durante la verificación: si una herramienta modifica el checksum al
programar, el contenido leído ya no coincide byte por byte con el archivo original.

---

## Los chequeos previos

Varias condiciones estructurales se pueden verificar en el archivo antes de conectar la placa:

```bash
make preflight
```

```
Chequeos previos al grabado: build/firmware.elf [ELF]

  [ OK ]   checksum de la boot ROM      la suma de las 8 palabras da 0 (vector 7 = 0xEFFF75EE)
  [ OK ]   stack pointer inicial        0x10008000 (tope de la RAM: 0x10008000)
  [ OK ]   Reset_Handler                0x000001B1 (bit Thumb en 1, dentro de la FLASH)
  [ OK ]   CRP en 0x2FC                 la imagen termina en 0x260, no llega a esa palabra
  [ OK ]   tamano                       608 bytes de 524288 (0.12% de la FLASH)

  Todo en orden. Listo para grabar.
```

En la plantilla, `make flash` ejecuta este control antes de programar y se detiene si encuentra un
error.

Qué mira cada uno:

1. **El checksum**, que es todo lo anterior.
2. **El stack pointer inicial.** El Cortex-M3 lo carga antes de ejecutar una sola instrucción.
   Si no apunta a RAM válida (entre `0x10000000` y `0x10008000`), el primer `push` escribe en
   el aire.
3. **El bit Thumb del `Reset_Handler`.** El Cortex-M3 **solo** ejecuta Thumb-2, y lo señaliza
   con el bit 0 de la dirección de salto en 1. Por eso el vector 1 vale `0x000001B1` y no
   `0x000001B0`: ese `1` final no es parte de la dirección, es el bit de modo. Si queda par,
   el chip toma un UsageFault en la primera instrucción.
4. **La palabra de CRP.** Ver la sección que sigue.
5. **Que entre en la FLASH.** Complementa las aserciones del linker y detecta una imagen que excede
   la capacidad declarada.

---

## CRP: protección de lectura de código

El LPC1769 interpreta ciertos patrones en `0x000002FC` como niveles de *Code Read Protection*.
Todos deshabilitan el acceso de debug por JTAG/SWD y restringen el ISP en distinta medida:

| Valor en `0x2FC` | Nivel | Efecto principal |
|---|---|---|
| `0x12345678` | CRP1 | impide debug y lectura de memoria; conserva una actualización ISP limitada |
| `0x87654321` | CRP2 | agrega restricciones: no permite escribir RAM ni copiar RAM a FLASH; solo admite borrar toda la FLASH |
| `0x43218765` | CRP3 | si hay código válido, deshabilita la entrada ISP forzada por P2.10 |

Con CRP3, el producto depende de que la propia aplicación ofrezca una actualización mediante IAP o
invoque el ISP por software. Si ese camino no existe o falla, puede no quedar un método práctico para
reprogramar la placa. Por eso no se debe grabar ninguno de estos patrones por accidente.

El patrón `0x4E697370`, llamado `NO_ISP` en otras familias LPC, **no figura como opción CRP del
LPC1769 en UM10360**. No hay que trasladar tablas de otro micro aunque la dirección se parezca.

El chequeo previo revisa la palabra solamente si la imagen alcanza `0x2FC`. Una FLASH borrada
contiene `0xFFFFFFFF`, que no activa CRP. Antes de habilitar protección de forma intencional, leé
la tabla de interacción entre CRP, validez del código y P2.10 en el manual, y probá el mecanismo de
actualización en hardware reemplazable.

---

## Cómo saber si el chip está corriendo tu programa

`Verified OK` significa "los bytes quedaron escritos". **No** significa "el programa corre".
Son cosas distintas, y esta es la forma directa de distinguirlas: mirar dónde está el PC.

```bash
cd plantilla
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"
```

| Lo que ves | Qué significa |
|---|---|
| PC dentro de `0x1FFF0000` a `0x1FFF1FFF` | está ejecutando la boot ROM; investigá si es ISP, IAP o una etapa transitoria |
| PC dentro de la FLASH de usuario | salió de la boot ROM y ejecuta código grabado |

La lectura del PC es una evidencia útil, pero representa un instante. Combinála con el estado de
reset, el pin ISP y un breakpoint en `Reset_Handler` o `main`. Hay una sesión real en la
[página 06](./06-primer-grabado-verificado.md).

---

## Grabar por el ISP, ya que estamos

El mismo bootloader permite grabar el chip sin una sonda, mediante un adaptador USB-serie con niveles
lógicos compatibles con la placa.

Los pasos, en resumen:

1. Conectar la UART0: TX del adaptador a **P0.3** (RXD0), RX a **P0.2** (TXD0), GND común.
2. Mantener **P2.10 en bajo** durante el reset.
3. Grabar con `lpc21isp` o con FlashMagic.

```bash
lpc21isp -control build/firmware.hex /dev/ttyUSB0 115200 12000
```

El último argumento es la frecuencia del oscilador en kHz usada por esta placa. Confirmala en el
esquemático: un valor incorrecto puede hacer fallar las operaciones de programación.

La guía completa, con el cableado, los problemas típicos y qué hacer para depurar sin
debugger, está en [la guía 05 de sondas](./probes/05-sin-probe-isp.md).

---

## Lo que hay que recordar

- Antes de tu código corre la **boot ROM**, que no se puede borrar, y **ella decide** si
  arrancás.
- Se queda en ISP si **P2.10 está en bajo** o si **el checksum de los 8 vectores no da cero**.
- El tratamiento automático del checksum depende de la herramienta; generar una imagen válida evita
  depender de ese comportamiento.
- La plantilla lo inyecta en el build y `make preflight` verifica el vector, el stack inicial, el
  reset, CRP y el tamaño antes de grabar.
- Los patrones CRP en `0x2FC` deshabilitan debug y pueden dejarte sin una ruta práctica de
  actualización. No los uses sin estudiar antes el flujo de recuperación.
- `Verified OK` **no** es "anda". Para saberlo, mirá dónde quedó el PC.

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [01 - La placa y su sonda](./01-la-placa-y-su-sonda.md) ·
**Siguiente:** [03 - Instalación en Linux](./03-instalacion-linux.md)

**Ver también:** [05 - El linker script y el startup](../05_del_codigo_al_binario/02-linker-y-startup.md) ·
[05 - El arranque paso a paso](../05_del_codigo_al_binario/03-el-arranque-paso-a-paso.md) ·
**Manual:** capítulo [32](../../manual/ch32_flash-memory-interface-and-programming.pdf)
