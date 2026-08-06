# De direcciones sueltas a structs estilo CMSIS

Ya podemos manejar el hardware con `*(volatile uint32_t *)0x2009C000`. Funciona, pero es ilegible y
peligroso. Vamos a mejorarlo en tres pasos, hasta llegar a la forma que usa CMSIS: `LPC_GPIO0->FIODIR`.
Entender este camino es lo que te va a permitir, en el [módulo 2](../02_arma_tu_propia_libreria/),
**escribir tu propia librería**.

## Paso 1: ponerle nombre a cada dirección con `#define`

Lo más simple: una macro por registro.

```c
#define FIO0DIR  (*(volatile uint32_t *) 0x2009C000)
#define FIO0SET  (*(volatile uint32_t *) 0x2009C018)
#define FIO0CLR  (*(volatile uint32_t *) 0x2009C01C)

FIO0DIR |= (1u << 22);
FIO0SET  = (1u << 22);
```

Mejor que antes. Pero tiene un problema: el LPC1769 tiene **5 puertos GPIO** (0 a 4), y cada uno
tiene los mismos registros (DIR, SET, CLR, PIN…) en direcciones distintas. Con `#define` tendrías que
escribir `FIO0DIR`, `FIO1DIR`, `FIO2DIR`… y así por cada registro de cada puerto. Decenas de macros,
y para "el puerto N" no podés usar una variable.

## Paso 2: la observación clave, los registros de un periférico son consecutivos

Mirá las direcciones de los registros del GPIO puerto 0:


| Registro   | Dirección    | Offset desde la base |
| ---------- | ------------ | -------------------- |
| `FIO0DIR`  | `0x2009C000` | +0x00                |
| `FIO0MASK` | `0x2009C010` | +0x10                |
| `FIO0PIN`  | `0x2009C014` | +0x14                |
| `FIO0SET`  | `0x2009C018` | +0x18                |
| `FIO0CLR`  | `0x2009C01C` | +0x1C                |


Están **pegados en memoria**, en offsets fijos desde la base `0x2009C000`. ¿Qué otra cosa en C es
"un montón de campos pegados en memoria con offsets fijos"? Una `struct`.

Y los puertos: GPIO0 está en `0x2009C000`, GPIO1 en `0x2009C020`, GPIO2 en `0x2009C040`… o sea, cada
puerto es **la misma struct** repetida cada 0x20 bytes.

## Paso 3: describir el periférico con una `struct` y "apoyarla" sobre la dirección

Definimos una `struct` cuyos campos calcen exactamente con los registros (respetando los offsets, con
relleno `RESERVED` donde haga falta):

```c
#include <stdint.h>

typedef struct {
    __IO uint32_t FIODIR;          // offset 0x00
         uint32_t RESERVED0[3];    // 0x04, 0x08, 0x0C  (huecos)
    __IO uint32_t FIOMASK;         // 0x10
    __IO uint32_t FIOPIN;          // 0x14
    __IO uint32_t FIOSET;          // 0x18
    __O  uint32_t FIOCLR;          // 0x1C
} LPC_GPIO_TypeDef;
```

(`__IO`, `__I`, `__O` son simplemente `volatile`, `volatile const` y `volatile`: etiquetan si el
registro es de lectura/escritura, solo lectura o solo escritura. Lo vemos en el módulo 2.)

Ahora "apoyamos" esa struct sobre la dirección base del puerto, haciendo un cast de la dirección a un
puntero a la struct:

```c
#define LPC_GPIO0  ((LPC_GPIO_TypeDef *) 0x2009C000)
#define LPC_GPIO1  ((LPC_GPIO_TypeDef *) 0x2009C020)
#define LPC_GPIO2  ((LPC_GPIO_TypeDef *) 0x2009C040)
```

Y usarlo queda **legible**:

```c
LPC_GPIO0->FIODIR |= (1u << 22);   // P0.22 como salida
LPC_GPIO0->FIOSET  = (1u << 22);   // prender
LPC_GPIO0->FIOCLR  = (1u << 22);   // apagar
```



### ¿Por qué funciona? El compilador hace la cuenta del offset por vos

Cuando escribís `LPC_GPIO0->FIOSET`, el compilador sabe que:

- `LPC_GPIO0` apunta a `0x2009C000`,
- dentro de la struct, `FIOSET` está en el offset `+0x18`,
- entonces `LPC_GPIO0->FIOSET` es exactamente `*(volatile uint32_t *)(0x2009C000 + 0x18)` =
`*(volatile uint32_t *)0x2009C018`.

¡Es lo mismo que escribíamos a mano! Solo que ahora es legible, reutilizable para cualquier puerto, y
el compilador calcula los offsets sin que te puedas equivocar.

> Por eso los campos `RESERVED` son **obligatorios**: si te saltás un hueco, todos los offsets de
> abajo quedan corridos y escribís en el registro equivocado. La struct tiene que ser un "molde"
> exacto del bloque de memoria del periférico.



## Esto es CMSIS

Abrí el header real del repo: `[library/CMSISv2p00_LPC17xx/inc/LPC17xx.h](../../library/CMSISv2p00_LPC17xx/inc/LPC17xx.h)`.
Vas a encontrar **la misma** `LPC_GPIO_TypeDef` (con más uniones para acceso por byte/half-word) y
los mismos `#define LPC_GPIO0 ((LPC_GPIO_TypeDef*) ...)`. Cuando incluís `"LPC17xx.h"`, todo eso ya
está hecho. CMSIS = "alguien ya escribió las structs de todos los periféricos del chip".

```c
// el FIO0DIR a mano:           *(volatile uint32_t *)0x2009C000
// con #define suelto:          FIO0DIR
// con struct (CMSIS):          LPC_GPIO0->FIODIR     <-- esto vas a usar en la materia
```



## La gran conclusión

CMSIS **no es una caja negra**. Es:

1. Un montón de `struct` que describen los bloques de registros de cada periférico.
2. Un montón de `#define` con las direcciones base.
3. (Más arriba) los *drivers* (`lpc17xx_gpio.c`, etc.), que son funciones que adentro hacen
  `LPC_GPIO0->FIODIR |= ...` por vos.

Si entendiste estas tres páginas, **ya podés escribir CMSIS vos mismo**. Eso es justo lo que vas a
hacer en el [módulo 2: Armá tu propia librería](../02_arma_tu_propia_libreria/), no para reemplazar
CMSIS, sino para que veas que el hardware es tuyo y lo podés adaptar a cualquier chip.

---

**Anterior:** [02 - Acceso a registros desde C](./02-acceso-a-registros-desde-c.md) ·
**Siguiente:** [04 - Bit-banding](./04-bit-banding.md)