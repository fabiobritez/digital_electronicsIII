# Debounce y filtrado de entradas

Este es el bug que casi todos sufren la primera vez: el botón "anda en la simulación pero en la placa
hace cosas raras". Presionás una vez y el contador sube 3. Soltás y a veces dispara de nuevo. El código
está bien; el problema es **físico** y se llama **rebote** (*bounce*). Esta página explica por qué pasa
y cómo filtrarlo bien.

## Por qué un botón "rebota"

Un pulsador es dos láminas de metal que se tocan. Cuando las apretás, **no** hacen contacto limpio de
una vez: chocan, rebotan, vuelven a tocar, varias veces, durante unos **5 a 20 ms** hasta que se
estabilizan. Para vos es instantáneo; para un micro que lee el pin millones de veces por segundo, es
una ráfaga de transiciones:

```
Lo que creés que pasa:        ___________
                          ____|           (un flanco limpio)

Lo que realmente pasa:        _ _ ___ _____
                          ____| | | |       (rebota durante ~10 ms)
```

Cada uno de esos flancos, si lo leés por *polling* rápido o (peor) con una **interrupción por flanco**
(módulo 7), cuenta como una pulsación. Por eso un solo apretón dispara la ISR varias veces.

Lo mismo, en menor medida, le pasa a cualquier entrada digital ruidosa: un sensor con cable largo, un
final de carrera, un contacto de relé. El debounce es un caso particular de **filtrado de entradas**.

## Lo que NO alcanza

- **Un `delay` en la ISR:** bloquea el micro y no garantiza nada; el rebote puede caer justo después.
- **Subir/bajar la prioridad de la interrupción:** no tiene nada que ver, el rebote es físico.
- **`volatile`:** resuelve otro problema (compartir variables con la ISR, módulo 7), no el rebote.

La idea correcta es **esperar a que la señal se quede quieta** antes de creerle. Hay dos familias de
solución: por **tiempo** y por **conteo**.

## Solución 1: debounce por tiempo (con SysTick)

La más usada. Filosofía: *cuando detecto un cambio, ignoro nuevos cambios durante ~20 ms*. Si después
de ese rato el pin sigue en el nuevo estado, lo doy por bueno. Apoyate en el tick de 1 ms del SysTick
(módulo 6) para no bloquear:

```c
#define DEBOUNCE_MS 20

// 'now_ms' lo incrementa el SysTick_Handler cada 1 ms.
extern volatile uint32_t now_ms;

uint8_t button_pressed(void)
{
    static uint8_t  last_stable = 0;   // último estado confirmado
    static uint8_t  last_read   = 0;   // última lectura cruda
    static uint32_t last_change = 0;   // cuándo cambió la lectura cruda

    uint8_t raw = (GPIO_ReadValue(0) >> 10) & 1;   // P0.10, por ejemplo

    if (raw != last_read) {            // la señal se movió: arranca el reloj
        last_read   = raw;
        last_change = now_ms;
    }
    // ¿se mantuvo quieta el tiempo suficiente?
    if ((now_ms - last_change) >= DEBOUNCE_MS && raw != last_stable) {
        last_stable = raw;             // recién acá lo confirmo
        if (last_stable == 1)          // flanco confirmado de 0 -> 1
            return 1;                  // "pulsación válida"
    }
    return 0;
}
```

Llamás a `button_pressed()` en el superloop ([módulo 0, cap. 17](../00_lenguaje_c/17-superloop-y-codigo-no-bloqueante.md)) y solo devuelve 1 **una vez** por pulsación
real. No bloquea, no usa `delay`, y el rebote desaparece porque exige estabilidad sostenida.

## Solución 2: debounce por conteo (muestreo periódico)

Variante muy robusta para entradas ruidosas. Cada cierto intervalo fijo (por ejemplo, cada 1 ms en el
`SysTick_Handler`) leés el pin y solo confirmás el cambio tras **N lecturas iguales seguidas**:

```c
// Se llama cada 1 ms desde SysTick_Handler.
void button_scan(void)
{
    static uint8_t count = 0;
    static uint8_t state = 0;

    if (((LPC_GPIO0->FIOPIN >> 10) & 1) == !state) {
        if (++count >= 10) {           // 10 ms estable -> cambio confirmado
            state = !state;
            count = 0;
            if (state) button_event = 1;
        }
    } else {
        count = 0;                     // se movió: reiniciá el conteo
    }
}
```

Una versión más elegante usa un **registro de desplazamiento**: metés el bit leído en un `uint8_t`
(`shift = (shift << 1) | raw;`) y considerás el botón presionado cuando `shift == 0xFF` (ocho lecturas
seguidas en alto). Filtra glitches sueltos casi gratis.

## ¿Y la interrupción por flanco?

Si usás interrupción por GPIO/EINT (módulo 7) para el botón, el rebote te dispara la ISR de más. Dos
enfoques sanos:

1. **ISR + ventana de tiempo:** en la ISR marcás un flag y guardás `now_ms`; ignorás nuevas
   interrupciones hasta que pasen 20 ms. Simple, pero perdés flancos durante la ventana (para un botón
   está bien).
2. **ISR solo despierta, el muestreo confirma:** la ISR habilita un pequeño muestreo periódico que
   aplica la solución 2 y, cuando se estabiliza, vuelve a dormir. Más robusto.

Para botones de usuario, lo más práctico y predecible es **no** usar interrupción por flanco y hacer el
*polling* con debounce por tiempo del superloop.

## Debounce por hardware (mención)

También se puede filtrar el rebote con **un capacitor** (RC) en la entrada, o con el clásico
integrado **74HC14** (Schmitt trigger), o un flip-flop SR para interruptores de dos posiciones. Es
robusto y descarga al software, pero agrega componentes. En la mayoría de los proyectos del curso, el
debounce por software alcanza y sobra.

## Lo que te llevás
- El **rebote** es físico: un pulsador genera una ráfaga de flancos durante ~5–20 ms.
- No se arregla con `delay` ni con prioridades: hay que **esperar estabilidad** antes de creerle al pin.
- **Por tiempo** (ignorar cambios durante ~20 ms tras detectar uno) o **por conteo** (N lecturas
  iguales seguidas), ambos apoyados en el SysTick, sin bloquear.
- Para botones, preferí *polling* con debounce en el superloop antes que interrupción por flanco.
- Es la causa #1 de "anda en simulación pero no en la placa".

---

**Anterior:** [02 - GPIO con el driver](./02-gpio-con-driver.md) ·
**Módulo:** [GPIO](./README.md) · **Siguiente:** [04 - FIOMASK y acceso por byte](./04-fiomask-y-acceso-por-byte.md)
