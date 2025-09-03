
# Sistema deserializador serial con distribución Par/Impar 

## Descripción del Problema

### Especificación Funcional

El sistema implementa un deserializador que toma una señal serial de entrada y distribuye cada bit a una de dos salidas paralelas según su **paridad de índice**:

* **Entrada (P0.0)**: Flujo serial de bits con período T.
* **Salida P0.2**: Bits con índice **par** (b0, b2, b4, b6, …).
* **Salida P0.1**: Bits con índice **impar** (b1, b3, b5, b7, …).

> **Pinout**
>
> | Pin  | Dirección | Función      |
> | ---- | --------- | ------------ |
> | P0.0 | Entrada   | Serial In    |
> | P0.1 | Salida    | Bits impares |
> | P0.2 | Salida    | Bits pares   |

### Características Temporales

```
Entrada serial (P0.0):
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │b0 │b1 │b2 │b3 │b4 │b5 │b6 │b7 │...
    └───┴───┴───┴───┴───┴───┴───┴───┘
    └─T─┘                               Período = T

Salida bits pares (P0.2):
    ┌───────────┬───────────┬───────────┬───────────┐
    │     b0    │     b2    │     b4    │     b6    │...
    └───────────┴───────────┴───────────┴───────────┘
    └────2T─────┘                           Período = 2T

Salida bits impares (P0.1):
    ┌───────────┬───────────┬───────────┬───────────┐
    │     b1    │     b3    │     b5    │     b7    │...
    └───────────┴───────────┴───────────┴───────────┘
    └────2T─────┘                           Período = 2T
```

> **Nota:** cada salida se actualiza **cada 2 bits** de la entrada, por eso su período efectivo es **2T**.

### Patrones de Control Especiales

| Patrón | Valor Hexadecimal | Acción                                                                              |
| ------ | ----------------- | ----------------------------------------------------------------------------------- |
| ZERO   | 0xF628            | Ambas salidas se fuerzan a cero (modo persistente hasta recibir un patrón distinto) |
| REPEAT | 0x28F6            | Se repite indefinidamente el **último patrón válido** detectado                     |

## Análisis de la Solución

### Arquitectura del Sistema

1. **Buffer deslizante de 16 bits**: mantiene siempre los últimos 16 bits.
2. **Detección continua**: compara el buffer con los patrones **en cada bit recibido**.
3. **Distribución por paridad**: usa un índice global (0=par, 1=impar) para enrutar a P0.2/P0.1.
4. **Estados de salida**: NORMAL / ZERO / REPEAT con transición inmediata.

### Diagrama de Bloques

```
                    ┌─────────────────────────┐
    P0.0 ───────────► Registro Desplazamiento │
    (Serial Input)  │  (Buffer deslizante)    │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │   Detector Patrones     │
                    │    0xF628 | 0x28F6      │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │  Distribuidor Par/Impar │
                    └─────┬───────────┬───────┘
                          │           │
                    ┌─────▼─────┐ ┌──▼───────┐
                    │Buffer Par │ │Buffer    │
                    │  8 bits   │ │Impar 8bit│
                    └─────┬─────┘ └──┬───────┘
                          │           │
                    ┌─────▼─────┐ ┌──▼───────┐
    P0.2 ◄──────────┤  Output   │ │ Output   ├────────► P0.1
    (Bits Pares)    │Controller │ │Controller│    (Bits Impares)
                    └───────────┘ └──────────┘
```

## Implementación Técnica

### Algoritmo de Distribución

#### Mapeo de Bits (referencia)

```
Dato de entrada (16 bits):
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ b15│ b14│ b13│ b12│ b11│ b10│ b9 │ b8 │ b7 │ b6 │ b5 │ b4 │ b3 │ b2 │ b1 │ b0 │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  MSB                                                                          LSB

Buffer de bits pares (8 bits):   [b14 b12 b10  b8  b6  b4  b2  b0]
Buffer de bits impares (8 bits): [b15 b13 b11  b9  b7  b5  b3  b1]
```

> **Nota:** el índice global `globalBitIndex` empieza en 0, por lo que **b0** va a **P0.2 (par)**, **b1** a **P0.1 (impar)**, y así sucesivamente.

### Temporización del Sistema

#### Configuración de Tiempos

| Parámetro              | Valor      | Descripción                              |
| ---------------------- | ---------- | ---------------------------------------- |
| `INPUT_PERIOD_MS`      | **10 ms**  | Período de bit de entrada (T)            |
| `OUTPUT_PERIOD`        | **20 ms**  | Período efectivo de cada salida (**2T**) |
| Frecuencia entrada     | **100 Hz** | 1/T = 100 bits/s                         |
| Frecuencia cada salida | **50 Hz**  | 1/(2T) = 50 bits/s                       |

#### Sincronización

```
Tiempo (ms):  0   5   10  15  20  25  30  35  40  45  50
Entrada:      └───b0──┘└───b1──┘└───b2──┘└───b3──┘└───b4──┘
Salida Par:   └────b0────┘       └────b2────┘
Salida Impar:         └────b1────┘       └────b3────┘
                     (2T)                    (2T)
```

### Máquina de Estados

#### Estados del Sistema

| Estado    | Descripción                             | Disparador / Salida                                                  |
| --------- | --------------------------------------- | -------------------------------------------------------------------- |
| `STARTUP` | Llenado inicial del buffer (16 bits)    | Al completar 16 bits → `NORMAL`                                      |
| `NORMAL`  | Operación estándar (enrutado par/impar) | Patrón `0xF628` → `ZERO`, `0x28F6` (si hay patrón válido) → `REPEAT` |
| `ZERO`    | Fuerza P0.2=0 y P0.1=0                  | Cualquier patrón ≠ `0xF628` → `NORMAL`                               |
| `REPEAT`  | Repite el último patrón válido          | Cualquier patrón ≠ `0x28F6` → `NORMAL`                               |

#### Diagrama de Transiciones

```
                 ┌─────────┐
                 │ NORMAL  │◄─────────┐
                 └────┬────┘          │
          0xF628 ────┘                │ Dato normal (≠ patrones)
                      ┌───────────┐   │
                      │ ZERO_MODE │───┘
                      └───────────┘

                 ┌─────────┐
                 │ NORMAL  │◄─────────┐
                 └────┬────┘          │
          0x28F6 ────┘                │ Dato normal (≠ patrones)
                      ┌───────────┐   │
                      │REPEAT_MODE│───┘
                      └───────────┘
```

## Casos de Uso

### Caso 1: Transmisión Normal

**Entrada**: 0xA5C3 = 1010 0101 1100 0011

**Distribución**:

```
Pares (idx 0,2,4,6,8,10,12,14): 1,0,0,1,0,0,1,1 = 0xC9
Impares (idx 1,3,5,7,9,11,13,15): 1,0,1,0,1,1,0,0 = 0x35
```

**Salida**:

* **P0.2**: 1,0,0,1,0,0,1,1
* **P0.1**: 1,0,1,0,1,1,0,0

### Caso 2: Comando ZERO (0xF628)

**Acción**:

* **P0.2 = 0** (continuo)
* **P0.1 = 0** (continuo)
* Permanece así hasta que el buffer de 16 bits **no** sea 0xF628.

### Caso 3: Comando REPEAT (0x28F6)

**Acción**:

* Repite indefinidamente el **último patrón válido** (`lastValidPattern`) detectado en `NORMAL`.
* La salida mantiene la alternancia par/impar según `globalBitIndex`.

---

# Análisis: Detección continua con buffer deslizante vs frames fijos


## Limitaciones del Enfoque por Frames

Cuando se analizan por frames fijos, se espera 16 bits completos antes de procesar:

```
Bits:  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 | 16 17 18...
       └──────────── Espera 160ms ─────────────────┘ ↓
                                                   Procesa

```
Latencia: 160ms para detectar cualquier patrón

 
* Latencia elevada (espera 16 bits).
* Riesgo de perder patrones que cruzan el límite de frame.
* Respuesta tardía a comandos.

## Solución: Buffer Deslizante (Sliding Window)

En cada bit nuevo:

1. `buffer = (buffer << 1) | bit`
2. Comparar contra `0xF628` y `0x28F6`
3. Aplicar acción inmediatamente (sin esperar 16 bits adicionales)


El buffer deslizante mantiene siempre los últimos 16 bits recibidos y evalúa después de cada bit nuevo:

```
Tiempo t:    [b15|b14|b13|b12|b11|b10|b9|b8|b7|b6|b5|b4|b3|b2|b1|b0]
Tiempo t+1:  [b14|b13|b12|b11|b10|b9|b8|b7|b6|b5|b4|b3|b2|b1|b0|bNEW]
             └─── Se desplaza ◄─── y se agrega el nuevo bit ───┘
```

### Estructura del Buffer Deslizante

```c
typedef struct {
    uint16_t data;        // Últimos 16 bits
    uint8_t bitCount;     // Para fase inicial
    uint8_t isReady;      // 1 después de 16 bits
} SlidingBuffer;
```

### Algoritmo de Actualización
 
```c
Por cada bit nuevo:
1. buffer = (buffer << 1) | nuevo_bit
2. Si buffer == PATRÓN_ESPECIAL:
   - Ejecutar acción inmediatamente
3. Distribuir bit a salida par/impar
``` 

## Consideraciones 


### Limitaciones Actuales

1. **Sin auto-baud** (velocidad fija).
2. **Sin detección de errores** (no hay paridad/CRC).
3. **Temporización fija en runtime** (definiciones por `#define`).

### Posibles Mejoras

1. Auto-baudrate.
2. CRC-16.
3. FIFO circular de mensajes.
4. Configuración de tiempos por comando.

---
