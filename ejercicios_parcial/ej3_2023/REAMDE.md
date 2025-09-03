# Sistema de recepción de datos con modulación PWM 

## Descripción del Problema

### Señal de Entrada

El sistema debe recibir una secuencia de datos través del pin P0.0 con el siguiente formato:

```
       ┌─── 30ms ───┬─── 50ms (10 bits × 5ms) ─────┬─── 30ms ───┐
       │            │                              │            │
    ───┘            └──────────────────────────────┘            └───
     Alto          Bajo      Datos binarios       Bajo         Alto
```

**Características de la señal:**
- Preámbulo: 30ms en nivel bajo
- Datos: 10 bits de 5ms cada uno (total 50ms)
- Postámbulo: 30ms en nivel bajo
- Duración total del frame: 110ms

### Señal de Salida

El valor recibido (0-1023) debe convertirse a una señal PWM de 50ms de duración total en el pin P0.2:

```
       ┌──── Ancho de pulso ─────┬──── Tiempo en bajo ─────┐
       │                         │                         │
    ───┘                         └─────────────────────────┘───
                                  
       └────────────────── 50ms total ─────────────────────┘
```

**Relación de conversión:**
- Entrada: 0 a 1023 (10 bits)
- Salida: 0 a 50ms de ancho de pulso
- Mapeo lineal: `ancho_pulso = (valor × 50) / 1023`

## Análisis de la Solución

### Arquitectura

El sistema implementa una máquina de estados finitos (FSM) con los siguientes estados:

| Estado | Descripción | Duración | Condición de Salida |
|--------|-------------|----------|---------------------|
| IDLE | Espera inicio de transmisión | Variable | Detección de flanco descendente |
| PRE_SYNC | Validación del preámbulo | 30ms | Timeout o error si pin = 1 |
| SYNC | Captura de datos | 50ms | Recepción de 10 bits |
| POST_SYNC | Validación del postámbulo | 30ms | Timeout o error si pin = 1 |
| ERROR | Estado de error | Inmediato | Reset automático |

### Diagrama de Estados

```
                    ┌─────────┐
                    │  IDLE   │◄──────────────┐
                    └────┬────┘               │
                         │                    │
                 Flanco descendente           │
                         │                    │
                    ┌────▼────┐               │
                ┌───│PRE_SYNC │               │
                │   └────┬────┘               │
          Error │        │ 30ms               │ Reset
         (pin=1)│   ┌────▼────┐               │
                ├───│  SYNC   │               │
                │   └────┬────┘               │
                │        │ 10 bits            │
                │   ┌────▼─────┐              │
                ├───│POST_SYNC │──────────────┤
                │   └──────────┘   110ms      │
                │                              │
                │   ┌─────────┐               │
                └──►│  ERROR  │───────────────┘
                    └─────────┘
```

## Implementación Técnica

### Configuración del Hardware

#### Timer SysTick
- Período: 1ms
- Frecuencia de muestreo: 1 kHz
- Registro RELOAD: (SystemCoreClock / 1000) - 1

#### Configuración de GPIO

| Pin | Función | Dirección | Pull | Justificación |
|-----|---------|-----------|------|---------------|
| P0.0 | Entrada de datos | Input | Pull-up | Estado definido en reposo |
| P0.2 | Salida PWM | Output | N/A | Señal de control |

### Algoritmo de Muestreo

#### Estrategia de Captura de Bits

El muestreo se realiza en el punto medio de cada bit para maximizar el margen de ruido:

```
Bit de 5ms:  |←─ 1ms ─→|←─ 1ms ─→|←─ 1ms ─→|←─ 1ms ─→|←─ 1ms ─→|
             └─────────────────────────────────────────────────┘
                                   ▲
                            Punto de muestreo
                               (3er ms)
```

**Cálculo del momento de muestreo:**
```c
if ((cycleCount - 1) % 5 == 2 && bitCount < 10) {
    // Muestrear bit
}
```

Esto genera muestreos en los milisegundos: 33, 38, 43, 48, 53, 58, 63, 68, 73, 78 (relativos al inicio de la transmisión).

### Detección de Inicio

La detección del inicio de transmisión se realiza mediante:
1. Monitoreo continuo del pin en estado IDLE
2. Detección de flanco descendente (1→0)
3. Registro del timestamp de inicio
4. Transición a estado PRE_SYNC

```c
if (lastPinValue == 1 && pinValue == 0) {
    syncStartTime = milliseconds;
    currentState = STATE_PRE_SYNC;
}
```

### Validación de Integridad

El sistema implementa validación en múltiples niveles:

1. **Validación temporal**: Verificación de duraciones esperadas
2. **Validación de nivel**: Pin debe mantenerse bajo durante preámbulo y postámbulo
3. **Contador de bits**: Verificación de recepción exacta de 10 bits

### Generación de PWM

#### Algoritmo de Conversión

```
Entrada: data ∈ [0, 1023]
Salida: pulseWidth ∈ [0, 50] ms

pulseWidth = (data × 50) / 1023
offTime = 50 - pulseWidth
```

#### Implementación

```c
void generatePWM(uint32_t data) {
    uint32_t pulseWidth = (data * 50) / 1023;
    uint32_t offTime = 50 - pulseWidth;
    
    LPC_GPIO0->FIOSET = (1 << 2);  // Alto
    delay_ms(pulseWidth);
    LPC_GPIO0->FIOCLR = (1 << 2);  // Bajo
    delay_ms(offTime);
}
```

## Variables del Sistema

### Variables de Tiempo

| Variable | Tipo | Descripción | Rango |
|----------|------|-------------|-------|
| milliseconds | uint32_t | Contador global de ms | 0 - 2³² |
| syncStartTime | uint32_t | Timestamp de inicio | 0 - 2³² |
| syncTime | uint32_t | Tiempo desde inicio | 0 - 110 |

### Variables de Estado

| Variable | Tipo | Descripción | Valores |
|----------|------|-------------|---------|
| currentState | uint8_t | Estado actual de FSM | 0-4 |
| pinValue | uint8_t | Valor actual del pin | 0-1 |
| lastPinValue | uint8_t | Valor anterior del pin | 0-1 |

### Variables de Datos

| Variable | Tipo | Descripción | Rango |
|----------|------|-------------|-------|
| receivedData | uint32_t | Datos recibidos | 0-1023 |
| bitCount | uint32_t | Bits recibidos | 0-10 |
| cycleCount | uint8_t | Contador de ciclos en SYNC | 0-50 |



## Casos de Error

### Errores Detectables

| Condición | Detección | Acción |
|-----------|-----------|--------|
| Preámbulo inválido | Pin alto durante PRE_SYNC | Transición a ERROR |
| Postámbulo inválido | Pin alto durante POST_SYNC | Transición a ERROR |
| Timeout | Tiempos fuera de especificación | Reset automático |
| Bits incompletos | bitCount < 10 al finalizar SYNC | Transición a ERROR |

### Errores No Detectables

- Inversión de bits durante transmisión
- Corrupción de datos sin cambio de timing
- Frames parcialmente válidos


## Análisis de Rendimiento

### Características Temporales

- **Latencia de detección**: < 1ms (limitada por SysTick)
- **Precisión de muestreo**: ±1ms
- **Tiempo total de procesamiento**: 110ms + tiempo PWM (50ms) = 160ms máximo
- **Throughput máximo**: 6.25 frames/segundo

### Limitaciones del Sistema

1. **Resolución temporal**: 1ms mínimo detectable
2. **Detección de errores**: No implementa CRC o checksum
3. **Sincronización**: No hay recuperación de clock
4. **Buffer**: Procesa un frame a la vez, no hay cola
5. **Blocking PWM**: La generación PWM bloquea recepción de nuevo frame
## Optimizaciones Posibles

### Mejora de Robustez

1. **Implementación de CRC**: Agregar byte de verificación
2. **Buffer circular**: Permitir recepción continua
3. **PWM por hardware**: Usar timer dedicado para no bloquear
4. **Filtro digital**: Reducir sensibilidad a ruido

### Mejora de Rendimiento

1. **DMA para captura**: Reducir overhead de CPU
2. **Interrupt on change**: Usar EINT en lugar de polling
3. **Timer capture**: Medición precisa de anchos de pulso

## Pruebas y Verificación

### Casos de Prueba

1. **Transmisión válida**: Todos los bits en 0 → PWM mínimo
2. **Transmisión válida**: Todos los bits en 1 → PWM máximo
3. **Patrón alternante**: 0101010101 → PWM ~50%
4. **Error en preámbulo**: Pulso alto durante primeros 30ms
5. **Error en postámbulo**: Pulso alto durante últimos 30ms