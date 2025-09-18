## Sistema de Reconocimiento de Patrón de Golpes

### Descripción del Sistema

Desarrollar un sistema básico que reconozca un patrón de golpes (como tocar la puerta) para activar un dispositivo. Un único usuario debe reproducir una secuencia específica de golpes con el **timing** correcto para obtener acceso. El sistema permite **regrabar** el patrón (modo GRABAR).

### Requerimientos Funcionales

#### 1. **Captura del Patrón de Golpes**:

* Detectar golpes usando un pulsador o sensor simple (EINT0)
* **Timer0**: Medir tiempo entre golpes (resolución de 10 ms)
* Registrar **hasta 6 golpes** máximo
* **Timeout** de **1.5 s** para finalizar secuencia (silencio ⇒ fin)
* Anti-rebote por **50 ms** mínimo entre golpes

#### 2. **Estructura del Patrón**:

```c
typedef struct {
    uint8_t  num_golpes;           // Cantidad de golpes (2-6)
    uint16_t intervalos[5];        // Tiempo entre golpes en ms
    uint8_t  usuario_id;           // Fijo: 1
} patron_t;
```

#### 3. **Sistema con 1 Usuario**:

* **Usuario 1** (único): patrón grabado en memoria (valor por defecto de fábrica)
* Tolerancia de **±150 ms** en cada intervalo

#### 4. **Modos de Operación** (Selección por switch):

* **Modo NORMAL**: Reconocer patrón y dar acceso
* **Modo GRABAR**: Grabar nuevo patrón para el usuario 1 (reemplaza el anterior)

#### 5. **Salidas del Sistema**:

* **LED Verde**: Acceso concedido (3 s)
* **LED Rojo**: Acceso denegado (parpadeo rápido ×3)
* **LED Amarillo**: Modo grabación activo (encendido fijo mientras graba, parpadeo ×3 al confirmar)
* **Buzzer**: Feedback de golpe detectado (beep corto)
* **Relé/LED especial**: Simular apertura de puerta (5 s)

### SOLUCION

#### 1. **Configuración de Hardware**:

```c
// Pines a utilizar
#define SENSOR_GOLPE    EINT0    // P2.10 - Entrada de golpes
#define BTN_MODO        P2.11    // Cambiar NORMAL/GRABAR
#define LED_VERDE       P0.22    // Acceso OK
#define LED_ROJO        P0.21    // Acceso denegado  
#define LED_AMARILLO    P0.20    // Modo grabación
#define RELE_PUERTA     P0.19    // Salida principal
#define BUZZER          P0.18    // Feedback auditivo
```

#### 2. **Variables Globales Necesarias**:

```c
// Estado del sistema
typedef struct {
    uint8_t  modo;              // 0=NORMAL, 1=GRABAR
    uint8_t  capturando;        // Flag de captura activa
} sistema_t;

// Buffer de captura temporal
typedef struct {
    uint8_t  contador_golpes;
    uint32_t t_ultimo_golpe_ms;
    uint16_t intervalos[5];
} captura_t;

// Patrón persistente del usuario 1
extern patron_t patron_usuario_1;
```

### Ejemplo de Operación

#### Secuencia Normal:

1. Sistema en **NORMAL**, esperando primer golpe
2. Usuario toca: “Toc-Toc…pausa…Toc”
3. Timer mide intervalos: p. ej. `[90 ms, 780 ms]`
4. Sistema compara contra **patrón del Usuario 1** (en memoria)
5. Si todos los intervalos cumplen tolerancia (±150 ms) ⇒ **Acceso OK**
6. **LED verde 3 s**, **Relé 5 s**

#### Secuencia de Grabación:

1. Colocar switch en **GRABAR** → **LED amarillo ON**
2. Tocar el nuevo patrón (mín. 2 y máx. 6 golpes)
3. Tras **1.5 s sin golpes** ⇒ fin de captura
4. Guardar patrón ⇒ **LED amarillo parpadea ×3** (confirmación)
5. Volver a **NORMAL** (switch)

### Algoritmo Simplificado de Comparación

```c
// Pseudocódigo para comparación
uint8_t Patron_Comparar(const patron_t* ref, const patron_t* med) {
    if (ref->num_golpes != med->num_golpes) return 0;
    for (uint8_t i = 0; i < ref->num_golpes - 1; i++) {
        uint16_t diff = (ref->intervalos[i] > med->intervalos[i])
                        ? (ref->intervalos[i] - med->intervalos[i])
                        : (med->intervalos[i] - ref->intervalos[i]);
        if (diff > 150) return 0;  // ±150 ms
    }
    return 1;
}
```

### Estados del Sistema (Máquina de Estados Simple)

```
IDLE → CAPTURANDO → PROCESANDO → RESULTADO
  ↑                                    ↓
  ←────────────────────────────────────
```


**Notas** (opcionales pero recomendadas):

* Implementar **anti-rebote por soft** en la ISR (ignorar golpes <50 ms desde el anterior).
* Normalizar levemente los intervalos a múltiplos de 10 ms para acoplarse a la resolución del **Timer0**.
* Persistir el patrón en **EEPROM/Flash** y cargarlo al inicio; si no hay datos válidos, usar el patrón de fábrica.



--
 