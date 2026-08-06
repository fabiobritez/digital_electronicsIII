# CAN: filtro de aceptación, recepción por interrupción y driver

Segunda parte de CAN. En la [primera](./04-can.md) vimos el protocolo, el arbitraje, el bit timing y los
registros del controlador. Acá vamos a lo que falta para usarlo de verdad: el **Acceptance Filter**
(la pieza más particular del CAN del LPC), la **recepción por interrupción**, el **transceiver** externo
y el **driver CMSIS** de punta a punta.

## Por qué hace falta un filtro de aceptación

Recordá que en CAN **todos** los nodos escuchan **todos** los mensajes. En un bus de auto puede haber
cientos de IDs distintos pasando miles de veces por segundo, y a tu nodo le importan tres o cuatro. Si
el CPU tuviera que mirar cada frame para decidir si lo tira, se la pasaría atendiendo basura.

El **Acceptance Filter (AF)** resuelve eso **por hardware**: es un bloque **compartido por CAN1 y CAN2**
con una **RAM de 512 × 32 bits (2 kB)** donde cargás una tabla de los IDs que te interesan. El hardware
hace una **búsqueda binaria** sobre esa tabla por cada frame que llega; si el ID no está, lo **descarta**
sin molestar al CPU. En la RAM entran hasta **1024 IDs estándar** o **512 extendidos** (o mezcla).

Es un bloque **global**: una sola tabla sirve a los dos controladores, y por eso cada entrada dice a
**qué controlador** (CAN1/CAN2) pertenece.

## Los modos del filtro (`AFMR`)

El registro **`AFMR`** (Acceptance Filter Mode, en `0x4003C000`) maneja el modo con tres bits:

| Modo | AccOff | AccBP | Qué hace | Acceso a la tabla |
|------|:------:|:-----:|----------|-------------------|
| **Off**       | 1 | 0 | **No acepta ningún mensaje**. Para inicializar. | CPU lee/escribe libre |
| **Bypass**    | X | 1 | **Acepta TODOS** los mensajes (filtro desactivado) | CPU lee/escribe libre |
| **Operating** | 0 | 0 | Filtra por hardware según la tabla | tabla **read-only** desde el CPU |
| **FullCAN**   | 0 | 0 + bit `eFCAN` | Operating + auto-recepción FullCAN | tabla read-only |

Reglas clave (que son fuente típica de bugs):

- Para **escribir** la tabla y los registros de secciones, el AF tiene que estar en **Off** o **Bypass**.
  En Operating, la RAM es de **solo lectura** (salvo para habilitar/deshabilitar mensajes individuales).
- **Bypass** es el "aceptar todo" cómodo para empezar a probar (`CAN_AccBP` en el driver). Pero en
  producción querés **Operating** con tu tabla cargada.
- El AF tiene su propio reloj `PCLK_ACF`, que **debe** tener el mismo divisor que `PCLK_CAN1/2`.

## Las cinco secciones de la tabla (AFLUT)

La RAM del AF se organiza en **cinco secciones**, en orden estricto. Cada sección tiene un registro de
*start address* que dice dónde empieza dentro de la RAM:

| Sección | Registro start | Qué guarda |
|---------|----------------|------------|
| **FullCAN** (IDs 11 bits con auto-recepción) | (antes de `SFF_sa`) | objetos FullCAN (ver nota sobre el límite de 64 más abajo) |
| **Standard Individual (SFF)** | `SFF_sa` | IDs estándar sueltos, exactos |
| **Standard Group (SFF_GRP)** | `SFF_GRP_sa` | rangos `[lowerID, upperID]` de IDs estándar |
| **Extended Individual (EFF)** | `EFF_sa` | IDs extendidos sueltos |
| **Extended Group (EFF_GRP)** | `EFF_GRP_sa` | rangos de IDs extendidos |
| (fin) | `ENDofTable` | dirección donde termina la tabla |

Dentro de cada sección, los IDs tienen que estar en **orden numérico ascendente**: si los cargás
desordenados, el hardware de búsqueda se confunde y el driver te devuelve `CAN_AF_ENTRY_ERROR`. Los
registros `LUTerrAd` y `LUTerr` reportan errores de la tabla.

Los "individual" aceptan **un** ID exacto; los "group" aceptan un **rango contiguo** y son mucho más
económicos en RAM si te interesan muchos IDs seguidos.

## FullCAN: recepción automática

La sección **FullCAN** es especial: para esos IDs (siempre **estándar**, 11 bits), el AF **no solo
acepta** el mensaje, sino que lo **guarda automáticamente** en un área de objetos dentro de la propia RAM
del AF, **sin** pasar por el buffer Rx del controlador ni necesariamente molestar al CPU. Con el bit
`eFCAN` activado, podés además recibir una interrupción cuando llega un objeto FullCAN, y leerlo con
`FCAN_ReadObj`.

> El límite de **64** que se suele citar es sobre los objetos que **participan del esquema de
> interrupción** FullCAN, no sobre la cantidad de objetos. El manual (sección 16.18) aclara que con la
> RAM entera dedicada a FullCAN entran hasta **146** objetos, pero **solo los primeros 64** levantan
> interrupción (`IntPndx`); los demás se reciben igual, pero sin aviso por interrupción. Es ideal para mensajes que llegan seguido y de los que siempre querés el **último valor**
(estilo "mailbox"): el hardware mantiene el dato fresco y vos lo leés cuando querés.

## El driver CMSIS, de menor a mayor

El driver `lpc17xx_can` esconde casi todo esto. Vamos en capas.

### 1. Inicialización y pines

```c
#include "lpc17xx_can.h"
#include "lpc17xx_pinsel.h"

CAN_MSG_Type tx, rx;

void can_pins(void)
{
    PINSEL_CFG_Type p;
    p.OpenDrain = 0; p.Pinmode = 0; p.Funcnum = 1;   // función CAN

    // CAN1: P0.0 = RD1, P0.1 = TD1
    p.Portnum = 0; p.Pinnum = 0; PINSEL_ConfigPin(&p);
    p.Pinnum = 1; PINSEL_ConfigPin(&p);
}

void can_setup(void)
{
    can_pins();
    CAN_Init(LPC_CAN1, 500000);                 // CAN1 a 500 kbit/s (calcula CANBTR)
    CAN_SetAFMode(LPC_CANAF, CAN_AccBP);        // para empezar: aceptar todo (bypass)
}
```

`CAN_Init` enciende el periférico por `PCONP`, fija `PCLK_CAN/ACF` a CCLK/2, entra a Reset Mode, calcula
y escribe `CANBTR` para la baudrate pedida, limpia la RAM del filtro y vuelve a normal.

### 2. Transmitir y recibir por *polling*

```c
void can_enviar(void)
{
    tx.id     = 0x100;                          // identificador del mensaje
    tx.len    = 2;                              // 2 bytes de datos (DLC)
    tx.dataA[0] = 0x12; tx.dataA[1] = 0x34;     // dataA[0..3], dataB[0..3] para bytes 5-8
    tx.format = STD_ID_FORMAT;                  // ID estándar de 11 bits (EXT_ID_FORMAT para 29)
    tx.type   = DATA_FRAME;                     // o REMOTE_FRAME para un pedido (RTR)
    CAN_SendMsg(LPC_CAN1, &tx);                 // busca un buffer Tx libre y dispara TR
}

void can_recibir(void)
{
    if (CAN_ReceiveMsg(LPC_CAN1, &rx) == SUCCESS) {
        // rx.id, rx.len, rx.dataA[...], rx.dataB[...] tienen el mensaje
    }
}
```

`CAN_SendMsg` usa los 3 buffers Tx internos (TFI/TID/TDA/TDB) y maneja `CMR`. `CAN_ReceiveMsg` lee
RFS/RID/RDA/RDB y libera el buffer con `RRB`.

### 3. Cargar el filtro de aceptación (Operating)

Para producción, en vez de bypass, armás la **tabla**. Hay dos caminos:

**A) Cargar entradas sueltas** (cómodo para pocas):

```c
CAN_SetAFMode(LPC_CANAF, CAN_AccOff);                 // apagar para poder escribir
CAN_LoadExplicitEntry(LPC_CAN1, 0x100, STD_ID_FORMAT);// aceptar el ID 0x100
CAN_LoadGroupEntry(LPC_CAN1, 0x200, 0x2FF, STD_ID_FORMAT); // aceptar el rango 0x200..0x2FF
CAN_LoadFullCANEntry(LPC_CAN1, 0x080);                // 0x080 como objeto FullCAN
CAN_SetAFMode(LPC_CANAF, CAN_Normal);                 // volver a filtrar (Operating)
```

**B) Armar la tabla completa de una** con `AF_SectionDef` (lo que hace el ejemplo `CAN_test_aflut`):

```c
FullCAN_Entry  fc[2];
SFF_Entry      sff[2];
SFF_GPR_Entry  grp[1];
AF_SectionDef  tabla;

void can_filtro(void)
{
    fc[0].controller = CAN1_CTRL; fc[0].disable = MSG_ENABLE; fc[0].id_11 = 0x01;
    fc[1].controller = CAN1_CTRL; fc[1].disable = MSG_ENABLE; fc[1].id_11 = 0x02;

    sff[0].controller = CAN1_CTRL; sff[0].disable = MSG_ENABLE; sff[0].id_11 = 0x08;
    sff[1].controller = CAN1_CTRL; sff[1].disable = MSG_ENABLE; sff[1].id_11 = 0x09;

    grp[0].controller1 = grp[0].controller2 = CAN1_CTRL;
    grp[0].disable1 = grp[0].disable2 = MSG_ENABLE;
    grp[0].lowerID = 0x10; grp[0].upperID = 0x20;

    tabla.FullCAN_Sec    = fc;  tabla.FC_NumEntry      = 2;
    tabla.SFF_Sec        = sff; tabla.SFF_NumEntry     = 2;
    tabla.SFF_GPR_Sec    = grp; tabla.SFF_GPR_NumEntry = 1;
    tabla.EFF_Sec        = NULL; tabla.EFF_NumEntry    = 0;
    tabla.EFF_GPR_Sec    = NULL; tabla.EFF_GPR_NumEntry= 0;

    if (CAN_SetupAFLUT(LPC_CANAF, &tabla) != CAN_OK) {
        // tabla mal armada (IDs desordenados, demasiados objetos, etc.)
    }
}
```

Acordate: dentro de cada sección los IDs van **ascendentes**, o `CAN_SetupAFLUT` devuelve
`CAN_AF_ENTRY_ERROR`.

### 4. Recepción por interrupción

En un sistema real no hacés *polling*: habilitás la interrupción de recepción y dejás que el hardware te
avise.

```c
void can_irq_setup(void)
{
    CAN_IRQCmd(LPC_CAN1, CANINT_RIE, ENABLE);   // habilitar Receive Interrupt
    CAN_IRQCmd(LPC_CAN1, CANINT_FCE, ENABLE);   // (opcional) interrupción FullCAN
    NVIC_EnableIRQ(CAN_IRQn);                    // CAN1 y CAN2 comparten el mismo vector NVIC
}

void CAN_IRQHandler(void)
{
    // FullCAN primero (si lo usás)
    if (CAN_FullCANIntGetStatus(LPC_CANAF) == SET) {
        if (CAN_FullCANPendGetStatus(LPC_CANAF, FULLCAN_IC0) ||
            CAN_FullCANPendGetStatus(LPC_CANAF, FULLCAN_IC1)) {
            FCAN_ReadObj(LPC_CANAF, &rx);
        }
    }
    // CANICR se LIMPIA al leerlo: una sola lectura por ISR
    uint32_t st = CAN_IntGetStatus(LPC_CAN1);
    if (st & CAN_ICR_RI) {                       // bit de Receive Interrupt
        CAN_ReceiveMsg(LPC_CAN1, &rx);
        // procesar rx...
    }
    // st también te dice ALI (arbitration lost), BEI (bus error), EPI (error passive)...
}
```

Detalle fino que se paga caro si se ignora: **`CANICR` se limpia al leerse**, por eso `CAN_IntGetStatus`
se llama **una sola vez** por ISR y guardás el resultado. Y como CAN1 y CAN2 comparten el **mismo vector**
`CAN_IRQn`, en la ISR tenés que chequear **ambos** controladores si usás los dos.

## El hardware: transceiver y bus físico

Esto el datasheet lo da por sabido, pero es donde más placas "no andan":

- El LPC1769 trae el **controlador** CAN, pero **no el transceiver**. Los pines `RD`/`TD` manejan niveles
  lógicos de 3,3 V, no el par diferencial. Necesitás un chip externo tipo **MCP2551** o **TJA1050**, que
  convierte `TD`/`RD` al par **`CAN_H` / `CAN_L`**. Sin transceiver, **no hay bus**.
- El bus físico es un **par diferencial**: en reposo (recesivo) `CAN_H ≈ CAN_L ≈ 2,5 V`; en dominante,
  `CAN_H` sube (~3,5 V) y `CAN_L` baja (~1,5 V). El receptor mira la **diferencia**, no la tensión
  absoluta: por eso el ruido común (que afecta igual a ambos cables) se cancela.
- **Terminación: 120 Ω en cada extremo** del bus (no en cada nodo: solo en las dos puntas). Falta de
  terminación, o ponerla en el medio, genera reflexiones y errores intermitentes que aparecen "a veces".

## Casos de borde y errores típicos

- **Un solo nodo en el bus -> "no transmite".** Como nadie manda el bit de **ACK**, el transmisor lo
  toma como error y **retransmite indefinidamente**, subiendo `TXERR` hasta error passive. Es el síntoma
  clásico de probar CAN con un solo nodo. Soluciones: poner **un segundo nodo** que acuse, o usar
  **Self Test Mode** (`STM`) con *Self Reception Request*, que recibe el propio frame **sin** necesitar
  ACK ajeno (lo hace el ejemplo `CAN_self_test`).
- **Baudrate distinta entre nodos.** Si dos nodos no comparten **exactamente** la misma baudrate (mismo
  `PCLK_CAN` y `CANBTR` equivalente), no se entienden: errores de bit/stuffing en cascada. Verificá que
  todos usen la misma velocidad **y** un sample point sano.
- **`CAN_Init` se cuelga en un `while(1)`.** El cálculo de bit timing del driver exige que
  `PCLK_CAN / baudrate` sea múltiplo de algún `NT` válido. Si tu combinación de CCLK/divisor y baudrate
  no da un `BRP` entero, el driver **no encuentra** solución y se queda en un loop infinito. Solución:
  ajustá el divisor de `PCLK_CAN` (o la baudrate) para que la división dé un número "amigable".
- **Bus Off y no vuelve.** De Bus Off (`TXERR > 255`) el controlador **no sale solo**: tu firmware debe
  bajar el Reset Mode (`CAN_ModeConfig(..., CAN_RESET_MODE, DISABLE)` o limpiando `MOD.RM`). Sin eso, el
  nodo queda mudo para siempre. Conviene manejarlo desde la *Bus Error Interrupt* (`BEI`).
- **Filtro mal cargado -> no llega nada.** Si dejaste el AF en **Off** (en vez de Bypass u Operating con
  tabla), **descarta todo** y parece que el CAN "no recibe". Y si cargaste IDs **desordenados** en una
  sección, `CAN_SetupAFLUT` falla con `CAN_AF_ENTRY_ERROR`.
- **Dos nodos con el mismo ID.** Como vimos en la página 1: gana el arbitraje los dos y se corrompen en
  el campo de datos. Un ID = un único emisor.

## Lo que te llevás (parte 2)

- El **Acceptance Filter** es un bloque **global** (CAN1+CAN2) que descarta por hardware los IDs que no
  te interesan, usando una tabla en RAM ordenada en 5 secciones (FullCAN, SFF/SFF_GRP, EFF/EFF_GRP).
- `AFMR` elige el modo: **Off** (no acepta nada, para escribir la tabla), **Bypass** (acepta todo, para
  probar), **Operating** (filtra), **FullCAN** (auto-recepción de mensajes frecuentes).
- Con el driver: `CAN_Init` + `CAN_SetAFMode` para arrancar; `CAN_LoadExplicit/Group/FullCANEntry` o
  `CAN_SetupAFLUT` para el filtro; recepción por interrupción con `CAN_IRQCmd` + `CAN_IRQHandler`
  (acordate de leer `CANICR` una sola vez).
- En hardware: **transceiver externo** (MCP2551/TJA1050), par diferencial `CAN_H/CAN_L` y **120 Ω en
  cada extremo**.
- Los errores más comunes son de **bus físico** (terminación), de **ACK** (un solo nodo), de **baudrate**
  y de **filtro** (Off, o IDs desordenados).

## Referencias
- Manual, Cap. 16: [`../../manual/ch16_can1-2.pdf`](../../manual/ch16_can1-2.pdf)
- Header CMSIS: [`lpc17xx_can.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_can.h)
- Ejemplos: [`../../library/examples/CAN/`](../../library/examples/CAN/):
  `CAN_self_test`, `CAN_test_bypass_mode`, `CAN_test_aflut`, `CAN_LedControl`, `CAN_test_two_kit`.

---

**Anterior:** [04 - CAN (protocolo y bit timing)](./04-can.md) ·
**Módulo:** [Periféricos adicionales](./README.md) ·
**Siguiente:** [05 - I2S](./05-i2s.md)
