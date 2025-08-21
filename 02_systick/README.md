## ¿Qué es el SysTick?

El **System Tick Timer** (SysTick) es un temporizador especial integrado en el procesador ARM Cortex-M3. Puede generar interrupciones regulares que son muy útiles para sistemas operativos y aplicaciones que necesitan ejecutar tareas periódicamente.

### Características principales
- **Intervalos de tiempo de 10 milisegundos** (configurable)
- **Vector de excepción dedicado**
- **Dos opciones de reloj**: CPU interno o pin externo (STCLK)
- **Contador de 24 bits** (cuenta desde un valor hasta cero)

---

## Configuración básica

Para usar el SysTick necesitas configurar tres cosas:

### 1. Fuente de reloj
En el registro STCTRL, se puede seleccionar entre:
- **Reloj interno del CPU** (CCLK)
- **Reloj externo** desde el pin STCLK (P3.26)

### 2. Configuración de pin
Si usas el reloj externo, habilita la función STCLK en el pin P3.26 usando el registro PINMODE.

### 3. Interrupción
Habilita la interrupción del SysTick en el NVIC para que tu programa pueda responder a cada "tick".

---

## ¿Cómo funciona?

El SysTick funciona como un **contador regresivo**:

1. Cargas un valor inicial en el registro `STRELOAD`
2. El contador cuenta hacia atrás desde ese valor hasta llegar a 0
3. Cuando llega a 0, genera una interrupción
4. Automáticamente se recarga con el valor de `STRELOAD` y vuelve a empezar

> **Tip**: El valor por defecto está configurado para generar interrupciones cada 10ms cuando el CPU funciona a 100MHz.

> **Nota**: La frecuencia máxima del reloj externo es 1/4 de la frecuencia del CPU.
---

## Registros del SysTick

| Registro | Dirección | Función |
|----------|-----------|---------|
| **STCTRL** | 0xE000E010 | Control y estado |
| **STRELOAD** | 0xE000E014 | Valor de recarga |
| **STCURR** | 0xE000E018 | Valor actual del contador |
| **STCALIB** | 0xE000E01C | Calibración |

### Registro STCTRL (Control)

| Bit | Nombre | Función |
|-----|--------|---------|
| 0 | ENABLE | `1` = Habilita el contador, `0` = Deshabilita |
| 1 | TICKINT | `1` = Habilita interrupción, `0` = Sin interrupción |
| 2 | CLKSOURCE | `1` = Usa reloj CPU, `0` = Usa STCLK externo |
| 16 | COUNTFLAG | Se pone en `1` cuando el contador llega a 0 |

### Registro STRELOAD (Recarga)
- **Bits 23:0**: Valor que se carga en el contador cuando llega a 0
- **Valor por defecto**: 0 (debe ser configurado por software)

### Registro STCURR (Actual)
- **Bits 23:0**: Valor actual del contador (solo lectura)
- **Escribir cualquier valor**: Limpia el contador y el flag COUNTFLAG

### Registro STCALIB (Calibración)
- **TENMS**: Valor precalculado para generar interrupciones de 10ms. 0x0F 423F = 10ms a 100MHz
- **SKEW**: Indica si el valor TENMS es preciso (0 = si, 1 = no)
- **NOREF**: Indica si hay reloj de referencia externo disponible (0 = si, 1 = no)


El valor por defecto que se encuentra en STCALIB es 0x0F423F, si suponemos que la frecuencia de reloj es de 100MHz, podemos calcular el tiempo que tarda en llegar hasta 0 (y generar una interrupción).

    Debido a que tiene que contar hasta 0, la cantidad de cuentas que hará será el valor de recarga + 1.



$$
Ticks = 999999+1 = 1000000
$$

$$
Frequency = 100[MHz]
$$

$$
Time = Ticks \hspace{2mm} \frac{1}{Frequency}
$$

$$
Time = \frac{1000000}{100000000[1/s]} =  0,01 [s] = 10[ms]
$$


---

## Cálculos de ejemplo

Para configurar el SysTick para **10 milisegundos** de intervalo:

### Ejemplo 1: CPU a 100MHz
```
Frecuencia CPU = 100,000,000 Hz
Tiempo deseado = 0.01 segundos (10ms)

STRELOAD = (Frecuencia / 100) - 1
STRELOAD = (100,000,000 / 100) - 1 = 999,999 = 0xF423F
```

### Ejemplo 2: CPU a 80MHz
```
Frecuencia CPU = 80,000,000 Hz
Tiempo deseado = 0.01 segundos (10ms)

STRELOAD = (80,000,000 / 100) - 1 = 799,999 = 0xC34FF
```

### Ejemplo 3: CPU a 4MHz (oscilador interno)
```
Frecuencia CPU = 4,000,000 Hz
Tiempo deseado = 0.01 segundos (10ms)

STRELOAD = (4,000,000 / 100) - 1 = 39,999 = 0x9C3F
```

### Ejemplo 4: Reloj externo a 32.768 kHz
```
Frecuencia STCLK = 32,768 Hz
Tiempo deseado = 0.01 segundos (10ms)

STRELOAD = (32,768 / 100) - 1 = 327 = 0x147
```

---

## Código de ejemplo básico

```c
#include "LPC17xx.h"

void SysTick_Handler(void) {
    // Código que se ejecuta cada 10 ms
}

int main(void) {
    // Configurar SysTick para 1 ms
    SysTick->LOAD = (SysTick->CALIB & 0xFFFFFF)/10 // 1ms con 100 MHz
    
    SysTick->VAL  = 0;                     // Resetear contador
    SysTick->CTRL = (1 << 2) |  // Reloj = CPU
                    (1 << 1) |  // Interrupción habilitada
                    (1 << 0);   // Encender SysTick

    while(1) {
        // Código principal
    }
}

```

---

## Puntos importantes

- **Frecuencia máxima STCLK**: 1/4 de la frecuencia del CPU
- **Contador de 24 bits**: Valor máximo = 16,777,215
- **Lectura de STCTRL**: Automáticamente limpia el flag COUNTFLAG
- **Precisión**: Depende de la estabilidad del reloj utilizado