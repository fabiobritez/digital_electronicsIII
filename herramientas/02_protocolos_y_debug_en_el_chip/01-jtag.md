# JTAG

JTAG es uno de los estándares de test y depuración más antiguos que todavía vas a encontrar.
Su nombre no nació del debugging: significa *Joint Test Action Group*, el grupo que dio
origen al estándar **IEEE 1149.1**.

Entender de dónde viene explica todas sus rarezas.

---

## Por qué se inventó: no era para depurar

A fines de los 80 apareció un problema de fabricación. Hasta entonces, para verificar que una
placa recién soldada estaba bien, se apoyaba una **cama de pinchos** sobre las patas de los
chips y se medía. Con los encapsulados nuevos eso dejó de ser posible: las patas quedaron
demasiado juntas, y con los BGA directamente quedaron **debajo del chip**, sin acceso físico.

La idea de JTAG fue: si no podemos llegar a las patas desde afuera, que el propio chip nos
deje verlas desde adentro.

Para eso, los pines digitales compatibles incorporan celdas entre la lógica interna y el
encapsulado. Esas celdas forman un registro serie alrededor del integrado llamado
**boundary scan** (barrido de frontera):

```
        ┌─────────────────────────────────┐
   pin ─┤□ ← celda                        │
   pin ─┤□                    NUCLEO      │
   pin ─┤□                    del chip    │
   pin ─┤□                                │
        └─────────────────────────────────┘
          └── todas las celdas encadenadas ──┘
```

Con eso podés hacer dos cosas sin tocar el chip:

- **Mirar** qué hay en cada pin (`SAMPLE`).
- **Forzar** un valor en una salida (`EXTEST`) y observarlo en el chip vecino. Así se
  detectan circuitos abiertos, cortocircuitos y varios defectos de montaje sin acceder
  físicamente a cada pista.

Esa es la función original de IEEE 1149.1. La depuración llegó después, cuando los fabricantes
aprovecharon el mismo puerto serie para acceder a lógica adicional dentro del chip.

---

## Las señales

| Señal | Dirección | Qué hace |
|---|---|---|
| **TCK** | entrada | *Test Clock*: el reloj. Todo se mueve con sus flancos |
| **TMS** | entrada | *Test Mode Select*: la señal que **navega la máquina de estados** |
| **TDI** | entrada | *Test Data In*: los bits entran al chip por acá |
| **TDO** | salida | *Test Data Out*: los bits salen del chip por acá |
| **TRST** | entrada | *Test Reset*: opcional, resetea la lógica de test |

Son cuatro señales obligatorias y una de reset opcional. En el conector también hacen falta
masa y, normalmente, una referencia de tensión. Para un micro con pocos pines, ese costo
motivó la aparición de SWD.

> En el LPC1769 estas señales están en **pines dedicados** (los pines 1 a 5 del encapsulado
> LQFP100: TDO/SWO, TDI, TMS/SWDIO, TRST, TCK/SWDCLK). No son pines de propósito general que
> se puedan reasignar: están ahí siempre. También trae **RTCK**, que solo tenía sentido para
> los ARM7 viejos y en Cortex-M no se usa.

---

## La máquina de estados TAP

Esta es la parte que hace que JTAG parezca complicado, y en realidad es simple: **hay un solo
cable de control (TMS) y con él hay que hacer todo**, así que en vez de comandos hay una
máquina de estados de 16 estados por la que se navega poniendo TMS en 0 o en 1 en cada flanco
de TCK.

El bloque que la implementa se llama **TAP controller** (*Test Access Port*).

```
                     TMS=1,1,1,1,1  desde cualquier lado
                            │
                            ▼
                     ┌─────────────┐
                     │ Test-Logic- │
                     │   Reset     │
                     └──────┬──────┘
                            │ TMS=0
                            ▼
                     ┌─────────────┐
             ┌──────►│  Run-Test/  │◄──────┐
             │       │    Idle     │       │
             │       └──────┬──────┘       │
             │              │              │
        ┌────┴─────┐   ┌────▼─────┐   ┌────┴─────┐
        │ rama DR  │   │  TMS=1   │   │ rama IR  │
        │ (datos)  │◄──┴──────────┴──►│ (instr.) │
        └──────────┘                  └──────────┘
```

Lo único que hay que retener son las **dos ramas**:

- **La rama IR** carga el *Instruction Register*: le decís al chip **qué** vas a hacer.
- **La rama DR** carga el *Data Register*: le pasás **los datos** de esa operación.

Y en cada rama el recorrido es siempre el mismo: `Capture` (el registro toma el valor
actual), `Shift` (entran y salen bits en serie, uno por flanco de TCK), `Update` (el valor
nuevo se aplica).

Las tres instrucciones básicas obligatorias son `BYPASS`, `SAMPLE/PRELOAD` y `EXTEST`.
Muchos dispositivos también implementan `IDCODE`:

| Instrucción | Qué hace |
|---|---|
| `BYPASS` | conecta TDI a TDO con un solo flip-flop: "ignorame, dejá pasar" |
| `SAMPLE / PRELOAD` | leer el estado de los pines sin perturbar y precargar el registro |
| `EXTEST` | forzar valores en los pines |
| `IDCODE` (opcional) | devolver un identificador de 32 bits |

Al iniciar una conexión JTAG, OpenOCD suele leer uno o más identificadores de este tipo para
comprobar la cadena. Después puede usar registros de identificación propios del sistema de
depuración para reconocer el núcleo.

---

## La cadena: por qué JTAG sobrevive

Los chips con JTAG se pueden conectar **en serie**: el TDO de uno va al TDI del siguiente, y
TCK y TMS van en paralelo a todos.

```
       ┌──────┐    ┌──────┐    ┌──────┐
 TDI──►│ CPU  │───►│ FPGA │───►│ CPLD │───► TDO
       └──▲─▲─┘    └──▲─▲─┘    └──▲─▲─┘
          │ │        │ │        │ │
 TCK ─────┴─┼────────┴─┼────────┴─┤
 TMS ───────┴──────────┴──────────┘
```

Con las cuatro señales JTAG podés formar una cadena con varios dispositivos. Los que no
participan de una operación se ponen en `BYPASS` y aportan un único bit de retardo.

El encadenamiento y el boundary scan son dos capacidades que SWD no reemplaza y explican
por qué JTAG sigue vigente:

- Placas con varios dispositivos programables (FPGA + micro + CPLD).
- Test de fabricación por boundary scan.
- Procesadores grandes, multinúcleo, o de fabricantes que no son ARM (RISC-V, MIPS, Xtensa,
  y los DSP).

Para un microcontrolador solo en una placa, en cambio, cuatro pines es un precio caro por
algo que se puede hacer con dos.

---

## Cómo se pasó de test a depuración

El estándar deja lugar para **instrucciones propias del fabricante**. Así que los fabricantes
colgaron del mismo TAP registros que no tienen nada que ver con test:

```
                  TAP controller
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
   boundary scan    IDCODE       registros de DEPURACION
   (el estandar)               (agregado del fabricante)
```

En los sistemas Arm CoreSight, el acceso externo desemboca en un **DAP** (*Debug Access
Port*), que se explica en
[03 - El debug adentro del Cortex-M3](./03-adentro-del-cortex-m3.md). Una vez dentro del DAP,
JTAG actúa como transporte: las operaciones de depuración se expresan como accesos a sus
registros y, a través de ellos, al sistema. SWD ofrece otro transporte hacia ese mismo DAP
con menos señales.

---

## Los conectores

Vas a encontrar, entre otros, estos tres conectores de Arm. Conviene reconocerlos porque
cambian el paso, el tamaño y las señales disponibles:

| Conector | Pines | Paso | Dónde aparece |
|---|---|---|---|
| **ARM JTAG de 20** | 20 | 2.54 mm | el clásico de los ARM7/ARM9 y los J-Link viejos. Grandote |
| **Cortex Debug** | 10 | 1.27 mm | el estándar moderno. SWD + SWO, y opcionalmente JTAG. **Es el que trae la placa de la cátedra** |
| **Cortex Debug + ETM** | 20 | 1.27 mm | igual que el anterior más los 5 pines de traza paralela |

El de 10 pines es muy común en placas Cortex-M. Su pin 1 es **VTref**, la referencia que la
sonda usa para conocer los niveles lógicos del target. En la mayoría de las sondas es una
entrada y no alimenta la placa. Si falta, muchas sondas no habilitan sus señales.

---

## Lo que hay que recordar

- JTAG nació para probar interconexiones de una placa; la depuración se agregó después.
- Usa cuatro señales principales, TRST opcional, masa y una referencia de tensión.
- La máquina TAP selecciona instrucciones y desplaza datos mediante TMS y TCK.
- Permite hacer boundary scan y **encadenar** varios dispositivos.
- En muchos Cortex-M, JTAG y SWD son transportes alternativos hacia el mismo DAP. Para una
  placa con un solo micro suele elegirse SWD porque requiere menos señales.

---

**Protocolos:** [índice](./README.md) ·
**Siguiente:** [02 - SWD](./02-swd.md)
