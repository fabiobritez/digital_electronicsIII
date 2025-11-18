## Sistema de Conteo de Producción con Detección de Anomalías

### Descripción del Sistema

Desarrollar un sistema básico de monitoreo para una línea de producción que cuente productos y detecte problemas comunes. El sistema usará los Timers en modo COUNTER para contar pulsos de sensores.

### Requerimientos Funcionales  

#### 1. **Conteo de Productos** (Funcionalidad Principal):
   - **Timer0 en modo COUNTER**: Contar productos que pasan por el sensor (CAP0.0)
   - **Timer1 en modo COUNTER**: Contar productos rechazados/defectuosos (CAP1.0)
   - **Timer2 en modo TIMER**: Base de tiempo de 1 segundo para cálculos
   - Configurar para detectar flanco de subida (RISING_MODE)

#### 2. **Cálculo de Estadísticas Básicas**:
   - Cada segundo calcular:
     * Productos por minuto (PPM)
     * Porcentaje de rechazo
     * Total acumulado del turno
   - Mostrar información en LEDs/Display

#### 3. **Sistema de Lotes Simple**:
   ```c
   typedef struct {
       uint32_t id_lote;
       uint32_t cantidad_objetivo;
       uint32_t cantidad_actual;
       uint32_t rechazos;
       uint8_t  completado;
   } production_batch_t;
   ```
   - Configurar un lote con cantidad objetivo (ej: 100 productos)
   - Indicar progreso con LEDs (25%, 50%, 75%, 100%)
   - Alarma cuando se completa el lote

#### 4. **Detección de Problemas Básicos**:
   - **Línea detenida**: No hay conteo en 10 segundos
   - **Alta tasa de rechazo**: Más del 10% de productos rechazados
   - **Producción lenta**: Menos de 30 productos/minuto
   - Indicar alarmas con LED rojo y buzzer

#### 5. **Interfaz de Usuario Simple**:
   - **Botón 1 (EINT0)**: Iniciar/Detener producción
   - **Botón 2 (EINT1)**: Reset de contadores
   - **Botón 3 (GPIO)**: Cambiar visualización (total/PPM/rechazos)
   - **4 LEDs**: Estado del sistema
     * Verde: Producción normal
     * Amarillo: Advertencia
     * Rojo: Error/Problema
     * Azul: Lote completado

### Requerimientos Técnicos Simplificados

#### 1. **Configuración de Hardware**:
```c
// Pines a utilizar
#define SENSOR_PRODUCTOS    CAP0.0  // P1.26
#define SENSOR_RECHAZOS     CAP1.0  // P1.18
#define BTN_START_STOP      EINT0   // P2.10
#define BTN_RESET           EINT1   // P2.11
#define LED_VERDE           P0.22
#define LED_AMARILLO        P0.21
#define LED_ROJO            P0.20
#define LED_AZUL            P0.19
#define BUZZER              P0.18
```

#### 2. **Estructuras de Datos Mínimas**:
```c
// Estado del sistema
typedef struct {
    uint32_t productos_total;
    uint32_t rechazos_total;
    uint32_t productos_minuto;
    uint8_t  porcentaje_rechazo;
    uint8_t  estado;  // 0:STOP, 1:RUN, 2:ALARM
} system_status_t;

// Configuración
typedef struct {
    uint32_t min_ppm;           // Mínimo productos/minuto
    uint8_t  max_rechazo_pct;   // Máximo % rechazo
    uint32_t timeout_segundos;  // Timeout línea detenida
} system_config_t;
```
 
### Ejemplo de Operación

1. **Inicio**:
   - Operador presiona botón START
   - Sistema carga lote de 100 productos
   - LEDs indican sistema activo

2. **Producción Normal**:
   - Timer0 cuenta cada producto que pasa
   - Timer1 cuenta productos rechazados
   - Cada segundo (Timer2) se actualizan estadísticas

3. **Visualización**:
   - LED verde encendido (normal)
   - Display/Serial muestra: "PPM: 45 | Rechazos: 2% | Total: 67/100"

4. **Problema Detectado**:
   - Rechazos suben a 15%
   - LED amarillo parpadea
   - Buzzer suena 3 veces

5. **Lote Completado**:
   - Contador llega a 100
   - LED azul se enciende
   - Sistema espera reset o nuevo lote
 