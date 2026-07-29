# Cómo se accede a un registro desde C

Ya sabemos que un registro es una dirección de memoria. Ahora: **¿cómo escribo una dirección
concreta desde C?** La respuesta es un puntero. Y como el contenido lo cambia el hardware, ese
puntero tiene que ser `volatile` (ver [módulo 0, cap. 08](../00_lenguaje_c/08-tipos-de-ancho-fijo-y-volatile.md)).

## El patrón fundamental

Para acceder al registro que está en la dirección `0xDIRECCION` como un valor de 32 bits:

```c
#include <stdint.h>

// "Tratá la dirección 0xDIRECCION como un uint32_t volátil, y dame ese uint32_t"
*(volatile uint32_t *) 0xDIRECCION
```

Desglosado:
- `(volatile uint32_t *) 0xDIRECCION` → convierte el número `0xDIRECCION` en **un puntero** a un
  `uint32_t volatile`.
- `*( ... )` → **desreferencia** ese puntero: accede al contenido de esa dirección.

Con eso podés **leer**:
```c
uint32_t valor = *(volatile uint32_t *) 0xDIRECCION;
```
y **escribir**:
```c
*(volatile uint32_t *) 0xDIRECCION = 0x12345678;
```

Eso es, literalmente, todo. El resto es saber **qué dirección** y **qué bits**.

> **¿Por qué siempre `uint32_t`?** No es capricho: el manual (cap. 2, §2.2) dice que los registros
> de los periféricos APB están alineados a palabra de 32 bits y **deben accederse enteros de una
> vez**: no se puede leer o escribir "solo el byte alto" de un registro por separado. Por eso el
> patrón usa `uint32_t` y no `uint8_t`. (El GPIO rápido es la excepción documentada: sus registros
> sí admiten acceso por byte y half-word, y CMSIS lo aprovecha, como vas a ver en la próxima página.)

---

## Ejemplo real: prender un LED en P0.22 sin ninguna librería

El LPC1769 controla los pines digitales con el periférico **GPIO rápido (FIO)**, que vimos que vive
en `0x2009_C000`. Los registros del puerto 0 son:

| Registro | Dirección | Qué hace |
|----------|-----------|----------|
| `FIO0DIR` | `0x2009C000` | Dirección de cada pin: bit en 1 = **salida**, 0 = entrada |
| `FIO0PIN` | `0x2009C014` | Lee/escribe el estado de los pines |
| `FIO0SET` | `0x2009C018` | Escribir un 1 en un bit **pone ese pin en alto** (los 0 no afectan) |
| `FIO0CLR` | `0x2009C01C` | Escribir un 1 en un bit **pone ese pin en bajo** (los 0 no afectan) |

> Capítulo 9 del manual: [`manual/ch09...`](../../manual/ch09_general-purpose-input-output.pdf).

Queremos manejar **P0.22** (un LED). P0.22 = bit 22 del puerto 0. Por defecto, tras el reset, los
pines ya están en función GPIO, así que ni siquiera hace falta tocar PINSEL todavía.

```c
#include <stdint.h>

// Definimos punteros a los registros de GPIO0
#define FIO0DIR  (*(volatile uint32_t *) 0x2009C000)
#define FIO0SET  (*(volatile uint32_t *) 0x2009C018)
#define FIO0CLR  (*(volatile uint32_t *) 0x2009C01C)

#define LED   (1u << 22)        // máscara: bit 22 en 1, el resto en 0

void delay(volatile uint32_t n) {
    while (n--) { }            // demora burda (ya veremos hacerlo bien con SysTick)
}

int main(void) {
    FIO0DIR |= LED;            // P0.22 como SALIDA (poné en 1 el bit 22, sin tocar los demás)

    while (1) {
        FIO0SET = LED;        // prender LED (1 en bit 22 -> pin alto)
        delay(1000000);
        FIO0CLR = LED;        // apagar LED (1 en bit 22 -> pin bajo)
        delay(1000000);
    }
}
```

**Eso es un programa embebido completo.** Sin `lpc17xx_gpio.h`, sin CMSIS, sin nada: solo punteros a
direcciones. Cuando uses el driver `GPIO_SetDir()` / `GPIO_SetValue()`, por dentro hace
**exactamente esto**.

---

## La parte que más cuesta: manipular bits sin pisar el resto

Un registro de 32 bits controla 32 cosas a la vez. Casi nunca querés escribir el registro entero:
querés tocar **un bit** y dejar los otros como estaban. Para eso están las operaciones bitwise
(repaso del [módulo 0, cap. 02](../00_lenguaje_c/02-operadores.md)), aplicadas a hardware:

| Quiero… | Operación | Idea |
|---------|-----------|------|
| **Poner** el bit *n* en 1 | `REG \|= (1u << n);` | OR con la máscara: 1 fuerza 1, 0 no cambia |
| **Borrar** el bit *n* (a 0) | `REG &= ~(1u << n);` | AND con la máscara invertida: 0 fuerza 0, 1 no cambia |
| **Invertir** (toggle) el bit *n* | `REG ^= (1u << n);` | XOR: 1 invierte, 0 no cambia |
| **Leer** el bit *n* | `(REG >> n) & 1u` | corre el bit a la posición 0 y lo aísla |
| **Probar** si el bit *n* está en 1 | `if (REG & (1u << n))` | la máscara da distinto de 0 si el bit está |

Ejemplos:
```c
FIO0DIR |=  (1u << 22);   // P0.22 como salida
FIO0DIR &= ~(1u << 22);   // P0.22 como entrada
FIO0PIN ^=  (1u << 22);   // invertir el estado de P0.22
if (FIO0PIN & (1u << 10)) { /* P0.10 está en alto */ }
```

> **Error clásico:** usar `=` en lugar de `|=`/`&=`. `FIO0DIR = (1u<<22);` pone **todo** el
> registro en ese valor, borrando la configuración de los otros 31 pines. Para tocar un solo bit
> sin afectar al resto: `|=` para poner, `&= ~` para borrar.

> **¿Por qué `1u` y no `1`?** El sufijo `u` hace la constante `unsigned`. Para corrimientos de
> bits siempre querés trabajar sin signo (ver [cap. 08](../00_lenguaje_c/08-tipos-de-ancho-fijo-y-volatile.md)).

### El caso especial de SET/CLR (¡no necesitan máscara de lectura-modificación-escritura!)

Notá una sutileza elegante del LPC: para `FIO0DIR` usamos `|=` (leer, modificar, escribir) porque es
un registro "normal". Pero `FIO0SET` y `FIO0CLR` están diseñados para que **escribir un 0 no haga
nada**. Por eso ahí usamos `=` directo y sin riesgo:

```c
FIO0SET = (1u << 22);    // prende SOLO P0.22; los 0 en los otros bits no apagan nada
```

Esto evita el problema de "leer-modificar-escribir" y es más rápido y seguro en interrupciones.
Muchos periféricos tienen registros así (SET/CLR separados); cuando los veas, aprovechálos.

---

## Recapitulando

- Un registro = una dirección. Acceso en C = `*(volatile uint32_t *) DIRECCION`.
- `volatile` es obligatorio: el hardware cambia el valor por su cuenta.
- Para tocar un bit sin pisar los demás: `|=`, `&= ~`, `^=`.
- Algunos registros (SET/CLR) están pensados para escribirse con `=` directo.

Esto ya te alcanza para manejar **cualquier** periférico leyendo el manual. Pero escribir
`*(volatile uint32_t *)0x2009C000` a cada rato es feo y propenso a errores. En la
[próxima página](./03-de-direcciones-a-structs-cmsis.md) vemos cómo organizarlo con `#define` y, sobre
todo, con `struct`, que es justo como lo hace CMSIS.

---

**Anterior:** [01 - Mapa de memoria](./01-mapa-de-memoria.md) ·
**Siguiente:** [03 - De direcciones a structs CMSIS](./03-de-direcciones-a-structs-cmsis.md)
