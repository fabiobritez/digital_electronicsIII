# Tu placa por dentro

En el código escribís `P0.22`. Pero, ¿dónde está **físicamente** ese pin? ¿Cómo sé a qué patita del
chip corresponde, y qué hay conectado ahí? Esa traducción (del nombre lógico al cable real) es lo que
falta entre el software y el mundo.

## Tres niveles: nombre lógico → pin del chip → pin de la placa

Hay tres "capas" entre tu código y el cable:

1. **El nombre lógico** (`P0.22`): puerto 0, bit 22. Es lo que usás en el software.
2. **El pin del chip** (LPC1769): el LPC1769 viene en un encapsulado **LQFP100** (100 patitas). `P0.22`
   sale por **una** de esas 100 patitas. El **datasheet del LPC1769** tiene la tabla "pin → función"
   que dice qué patita es cada `Px.y` (también está en el User Manual UM10360, capítulo 7). Para
   cerrar el ejemplo: `P0.22` es la **patita 56** del LQFP100.
3. **El pin de la placa** (conector / header): la placa conecta esa patita del chip a un pin de un
   **header** (las tiras de pines donde enchufás cables) o a un componente de la placa (un LED, un
   botón). El **esquemático de la placa** es el que dice esto.

> Para llegar del `P0.22` del código al cable, necesitás **dos documentos**: el **datasheet** del chip
> (P0.22 → patita N) y el **esquemático** de tu placa (patita N → dónde va en la placa). El primero es
> de NXP; el segundo, del fabricante de tu placa.

## Cómo leer un esquemático (lo básico)

Un esquemático es un dibujo de las conexiones. Para encontrar dónde está tu pin:

- Buscá el **símbolo del LPC1769** (un rectángulo con todos sus pines etiquetados `P0.0`, `P0.1`, …).
- Cada pin tiene una **línea** (un *net*, una conexión) que va a algún lado. Las líneas con el **mismo
  nombre de net** están conectadas aunque no se toquen en el dibujo (es la convención para no llenar
  todo de cables cruzados).
- Seguí la línea de `P0.22`: te va a llevar a un header, a un LED, a un botón, etc.

Símbolos que vas a ver siempre:
- **VCC / 3V3 / VDD**: alimentación positiva (3.3 V en el LPC1769). Alrededor del micro vas a ver
  varios pines de alimentación con nombre propio: **VDD(3V3)** (×4, alimenta los puertos de E/S),
  **VDD(REG)(3V3)** (×2, el regulador interno), **VDDA** y **VREFP** (alimentación y referencia
  positiva del ADC/DAC, separadas para que el ruido digital no ensucie las mediciones) y **VBAT**
  (batería del RTC). Todos esos van a 3.3 V en una placa típica; **VREFN** (referencia negativa del
  ADC) va a masa.
- **GND** (un triángulo o líneas decrecientes): masa, el 0 V de referencia. En el LPC1769 son los
  pines **VSS** (×6) y **VSSA** (la masa analógica).
- **Resistencias** (rectángulo o zigzag), **capacitores** (dos líneas paralelas), **LEDs** (un
  triángulo con flechas), **transistores**, etc.

## Cómo está cableado un LED (los dos casos)

Un LED en una placa se conecta de una de dos formas, y **cambia tu código**:

**Activo en alto** (ánodo al pin, cátodo a GND vía resistencia):
```
P0.22 ──[ resistencia ]──▶|── GND        pin en ALTO  -> LED prende
                          LED
```
```c
GPIO_SetValue(0, LED);    // alto -> prende
```

**Activo en bajo** (ánodo a VCC, cátodo al pin):
```
3V3 ──[ resistencia ]──▶|── P0.22        pin en BAJO -> LED prende
                       LED
```
```c
GPIO_ClearValue(0, LED);  // bajo -> prende  (¡al revés!)
```

> Por eso a veces "el LED hace lo contrario de lo que pienso": está cableado activo en bajo. El
> esquemático te lo dice. No es un bug del código.

## Cómo está cableado un botón (y por qué importa el pull)

Un botón conecta el pin a GND o a VCC cuando lo apretás. Las dos formas:

**Botón a GND con pull-up** (el más común):
```
3V3 ──[ pull-up ]── P2.10 ──[ botón ]── GND
```
- Suelto: el pull-up mantiene el pin en **alto** (lee 1).
- Apretado: el botón lo lleva a **bajo** (lee 0).
- El pull-up puede ser **interno** (PINMODE, módulo 4) o externo (en la placa).

**Botón a VCC con pull-down:**
```
GND ──[ pull-down ]── P2.10 ──[ botón ]── 3V3
```
- Suelto: lee 0. Apretado: lee 1.

> Si leés un botón y "flota" (lee valores al azar cuando está suelto), te falta el **pull**: el pin
> quedó sin conectar a nada y capta ruido. Configurá el PINMODE correcto (módulo 4) o agregá una
> resistencia externa.

## Encontrar el pin físico para enchufar un cable

Cuando querés conectar algo a `P0.22`:
1. En el **silkscreen** de la placa (las letras impresas), buscá la etiqueta del header. Muchas placas
   rotulan los pines con su `Px.y` directamente.
2. Si no, cruzá el **esquemático** (P0.22 → qué pin del header) con la **disposición física**
   (qué header, qué fila).
3. Verificá siempre **GND**: tu cable de señal necesita una **referencia común** de masa con lo que
   conectes. Sin GND común, nada funciona.

## Lo que te llevás
- `P0.22` es un nombre lógico; el **datasheet** lo mapea a una patita del chip y el **esquemático** a
  un punto de la placa.
- Aprendé a **seguir un net** en el esquemático: es la habilidad clave.
- El **cómo está cableado** un LED o botón (activo alto/bajo, pull-up/down) **determina tu código**.
  Antes de culpar al software, mirá el esquemático.

En la [próxima página](./02-electronica-minima.md): las reglas eléctricas para no quemar el micro.

---

**Módulo:** [Hardware y placa](./README.md) · **Siguiente:** [02 - Electrónica mínima](./02-electronica-minima.md)
