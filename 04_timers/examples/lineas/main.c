#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_pinsel.h"

// ===== DEFINICIONES DE HARDWARE =====
#define SENSOR_PRODUCTOS_PORT    1
#define SENSOR_PRODUCTOS_PIN     26    // P1.26 - CAP0.0
#define SENSOR_RECHAZOS_PORT     1
#define SENSOR_RECHAZOS_PIN      18    // P1.18 - CAP1.0
#define BTN_START_STOP_PORT      2
#define BTN_START_STOP_PIN       10    // P2.10 - EINT0
#define BTN_RESET_PORT           2
#define BTN_RESET_PIN            11    // P2.11 - EINT1
#define BTN_DISPLAY_PORT         2
#define BTN_DISPLAY_PIN          12    // P2.12 - GPIO (Botón 3)
#define LED_VERDE_PORT           0
#define LED_VERDE_PIN            22    // P0.22
#define LED_AMARILLO_PORT        0
#define LED_AMARILLO_PIN         21    // P0.21
#define LED_ROJO_PORT            0
#define LED_ROJO_PIN             20    // P0.20
#define LED_AZUL_PORT            0
#define LED_AZUL_PIN             19    // P0.19
#define BUZZER_PORT              0
#define BUZZER_PIN               18    // P0.18

// ===== CONSTANTES DEL SISTEMA =====
#define MIN_PPM_DEFAULT          30     // Mínimo productos por minuto
#define MAX_RECHAZO_PCT_DEFAULT  10     // Máximo % rechazo
#define TIMEOUT_SEGUNDOS         10     // Timeout línea detenida
#define LOTE_DEFAULT             100    // Productos por lote por defecto
#define BUZZER_BEEPS             3      // Número de beeps para alarma

// Estados del sistema
#define ESTADO_STOP              0
#define ESTADO_RUN               1
#define ESTADO_ALARM             2

// Modos de visualización
#define DISPLAY_TOTAL            0
#define DISPLAY_PPM              1
#define DISPLAY_RECHAZOS         2

// ===== ESTRUCTURAS DE DATOS =====
typedef struct {
    uint32_t productos_total;
    uint32_t rechazos_total;
    uint32_t productos_minuto;
    uint8_t  porcentaje_rechazo;
    uint8_t  estado;              // 0:STOP, 1:RUN, 2:ALARM
} system_status_t;

typedef struct {
    uint32_t min_ppm;             // Mínimo productos/minuto
    uint8_t  max_rechazo_pct;     // Máximo % rechazo
    uint32_t timeout_segundos;    // Timeout línea detenida
} system_config_t;

typedef struct {
    uint32_t id_lote;
    uint32_t cantidad_objetivo;
    uint32_t cantidad_actual;
    uint32_t rechazos;
    uint8_t  completado;
} production_batch_t;

// ===== VARIABLES GLOBALES =====
volatile system_status_t sistema = {0, 0, 0, 0, ESTADO_STOP};
volatile system_config_t config = {MIN_PPM_DEFAULT, MAX_RECHAZO_PCT_DEFAULT, TIMEOUT_SEGUNDOS};
volatile production_batch_t lote_actual = {1, LOTE_DEFAULT, 0, 0, 0};

volatile uint32_t contador_segundos = 0;
volatile uint32_t productos_anterior = 0;
volatile uint32_t productos_ultimo_minuto[60] = {0}; // Buffer circular para PPM
volatile uint8_t indice_segundo = 0;
volatile uint32_t ultimo_producto_tiempo = 0;
volatile uint8_t display_mode = DISPLAY_TOTAL;

// Variables para control de LEDs y buzzer
volatile uint32_t buzzer_timer = 0;
volatile uint8_t buzzer_beeps_restantes = 0;
volatile uint8_t led_parpadeo_estado = 0;
volatile uint32_t led_parpadeo_timer = 0;
volatile uint8_t btn_display_anterior = 1;

// ===== PROTOTIPOS DE FUNCIONES =====
void System_Init(void);
void configTimers(void);
void configGPIO(void);
void configEINT(void);

void StartProduction(void);
void StopProduction(void);
void ResetProduction(void);

void Calculate_Statistics(system_status_t* status);
void Check_Alarms(system_status_t* status);
void Update_LEDs(void);
void Update_Buzzer(void);
void Update_Display_Mode(void);

void Batch_Load(production_batch_t* batch, uint32_t objetivo);
uint8_t Batch_Update(production_batch_t* batch, uint32_t productos, uint32_t rechazos);
void Batch_Show_Progress(production_batch_t* batch);

void TIMER2_IRQHandler(void);
void EINT0_IRQHandler(void);
void EINT1_IRQHandler(void);

// ===== IMPLEMENTACIÓN =====

void System_Init(void) {
    SystemInit();
    
    configGPIO();
    configTimers();
    configEINT();
    
    // Inicializar lote por defecto
    Batch_Load((production_batch_t*)&lote_actual, LOTE_DEFAULT);
    
    // Estado inicial
    sistema.estado = ESTADO_STOP;
    display_mode = DISPLAY_TOTAL;
    
    // LEDs iniciales - solo verde apagado indica STOP
    GPIO_ClearValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
    GPIO_ClearValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
    GPIO_ClearValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
    GPIO_ClearValue(LED_AZUL_PORT, (1<<LED_AZUL_PIN));
    GPIO_ClearValue(BUZZER_PORT, (1<<BUZZER_PIN));
}

void configGPIO(void) {
    PINSEL_CFG_Type PinCfg;
    
    // Configurar CAP0.0 (P1.26) para Timer0 - Sensor productos
    PinCfg.Portnum = SENSOR_PRODUCTOS_PORT;
    PinCfg.Pinnum = SENSOR_PRODUCTOS_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_3; // CAP0.0
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    
    // Configurar CAP1.0 (P1.18) para Timer1 - Sensor rechazos
    PinCfg.Portnum = SENSOR_RECHAZOS_PORT;
    PinCfg.Pinnum = SENSOR_RECHAZOS_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_3; // CAP1.0
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    
    // Botón de display como GPIO
    PinCfg.Portnum = BTN_DISPLAY_PORT;
    PinCfg.Pinnum = BTN_DISPLAY_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_0; // GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(BTN_DISPLAY_PORT, (1<<BTN_DISPLAY_PIN), 0); // Input
    
    // LEDs y buzzer como salidas
    GPIO_SetDir(LED_VERDE_PORT, (1<<LED_VERDE_PIN), 1);
    GPIO_SetDir(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN), 1);
    GPIO_SetDir(LED_ROJO_PORT, (1<<LED_ROJO_PIN), 1);
    GPIO_SetDir(LED_AZUL_PORT, (1<<LED_AZUL_PIN), 1);
    GPIO_SetDir(BUZZER_PORT, (1<<BUZZER_PIN), 1);
}

void configTimers(void) {
    TIM_TIMERCFG_Type TimerConfig;
    TIM_CAPTURECFG_Type CaptureConfig;
    TIM_MATCHCFG_Type MatchConfig;
    
    // ===== TIMER0 - Contador de productos (modo COUNTER) =====
    TimerConfig.PrescaleOption = TIM_PRESCALE_TICKVAL;
    TimerConfig.PrescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_COUNTER_RISING_MODE, &TimerConfig);
    
    // Configurar captura en CAP0.0 para contar flancos de subida
    CaptureConfig.CaptureChannel = 0;
    CaptureConfig.RisingEdge = ENABLE;
    CaptureConfig.FallingEdge = DISABLE;
    CaptureConfig.IntOnCapture = FALSE; // No interrumpir por cada producto
    TIM_ConfigCapture(LPC_TIM0, &CaptureConfig);
    
    // ===== TIMER1 - Contador de rechazos (modo COUNTER) =====
    TIM_Init(LPC_TIM1, TIM_COUNTER_RISING_MODE, &TimerConfig);
    
    // Configurar captura en CAP1.0 para contar flancos de subida
    CaptureConfig.CaptureChannel = 0;
    CaptureConfig.RisingEdge = ENABLE;
    CaptureConfig.FallingEdge = DISABLE;
    CaptureConfig.IntOnCapture = FALSE;
    TIM_ConfigCapture(LPC_TIM1, &CaptureConfig);
    
    // ===== TIMER2 - Base de tiempo 1 segundo (modo TIMER) =====
    TimerConfig.PrescaleOption = TIM_PRESCALE_TICKVAL;
    TimerConfig.PrescaleValue = 25000; // Para base de 1ms (25MHz/25000 = 1kHz)
    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &TimerConfig);
    
    // Match cada 1000ms = 1 segundo
    MatchConfig.MatchChannel = 0;
    MatchConfig.IntOnMatch = TRUE;
    MatchConfig.ResetOnMatch = TRUE;
    MatchConfig.StopOnMatch = FALSE;
    MatchConfig.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    MatchConfig.MatchValue = 1000; // 1000 ticks = 1 segundo
    TIM_ConfigMatch(LPC_TIM2, &MatchConfig);
    
    NVIC_EnableIRQ(TIMER2_IRQn);
    
    // Iniciar todos los timers
    TIM_Cmd(LPC_TIM0, ENABLE);
    TIM_Cmd(LPC_TIM1, ENABLE);
    TIM_Cmd(LPC_TIM2, ENABLE);
}

void configEINT(void) {
    PINSEL_CFG_Type PinCfg;
    EXTI_InitTypeDef EXTICfg;
    
    // EINT0 - Botón Start/Stop (P2.10)
    PinCfg.Portnum = BTN_START_STOP_PORT;
    PinCfg.Pinnum = BTN_START_STOP_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_1; // EINT0
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    
    EXTICfg.EXTI_Line = EXTI_EINT0;
    EXTICfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTICfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    
    EXTI_Init();
    EXTI_Config(&EXTICfg);
    NVIC_EnableIRQ(EINT0_IRQn);
    
    // EINT1 - Botón Reset (P2.11)
    PinCfg.Portnum = BTN_RESET_PORT;
    PinCfg.Pinnum = BTN_RESET_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_1; // EINT1
    PINSEL_ConfigPin(&PinCfg);
    
    EXTICfg.EXTI_Line = EXTI_EINT1;
    EXTI_Config(&EXTICfg);
    NVIC_EnableIRQ(EINT1_IRQn);
}

void TIMER2_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
        
        contador_segundos++;
        
        if (sistema.estado == ESTADO_RUN) {
            // Leer contadores actuales de los timers
            uint32_t productos_actuales = TIM_GetCaptureValue(LPC_TIM0, 0);
            uint32_t rechazos_actuales = TIM_GetCaptureValue(LPC_TIM1, 0);
            
            // Actualizar totales del sistema
            sistema.productos_total = productos_actuales;
            sistema.rechazos_total = rechazos_actuales;
            
            // Calcular productos producidos en este segundo
            uint32_t productos_este_segundo = productos_actuales - productos_anterior;
            productos_ultimo_minuto[indice_segundo] = productos_este_segundo;
            
            // Verificar actividad de la línea
            if (productos_este_segundo > 0) {
                ultimo_producto_tiempo = contador_segundos;
            }
            
            productos_anterior = productos_actuales;
            indice_segundo = (indice_segundo + 1) % 60;
            
            // Calcular estadísticas
            Calculate_Statistics((system_status_t*)&sistema);
            
            // Verificar alarmas
            Check_Alarms((system_status_t*)&sistema);
            
            // Actualizar lote
            Batch_Update((production_batch_t*)&lote_actual, 
                        sistema.productos_total, sistema.rechazos_total);
        }
        
        // Actualizar interfaz
        Update_Display_Mode();
        Update_LEDs();
        Update_Buzzer();
    }
}

void EINT0_IRQHandler(void) {
    if (EXTI_GetEXTIFlag(EXTI_EINT0)) {
        EXTI_ClearEXTIFlag(EXTI_EINT0);
        
        if (sistema.estado == ESTADO_STOP) {
            StartProduction();
        } else {
            StopProduction();
        }
    }
}

void EINT1_IRQHandler(void) {
    if (EXTI_GetEXTIFlag(EXTI_EINT1)) {
        EXTI_ClearEXTIFlag(EXTI_EINT1);
        ResetProduction();
    }
}

void StartProduction(void) {
    sistema.estado = ESTADO_RUN;
    ultimo_producto_tiempo = contador_segundos;
}

void StopProduction(void) {
    sistema.estado = ESTADO_STOP;
}

void ResetProduction(void) {
    // Reset de los timers contadores
    TIM_ResetCounter(LPC_TIM0);
    TIM_ResetCounter(LPC_TIM1);
    
    // Reset de variables del sistema
    sistema.productos_total = 0;
    sistema.rechazos_total = 0;
    sistema.productos_minuto = 0;
    sistema.porcentaje_rechazo = 0;
    productos_anterior = 0;
    
    // Limpiar buffer de productos por segundo
    for (int i = 0; i < 60; i++) {
        productos_ultimo_minuto[i] = 0;
    }
    indice_segundo = 0;
    
    // Reset del lote actual
    lote_actual.cantidad_actual = 0;
    lote_actual.rechazos = 0;
    lote_actual.completado = 0;
    lote_actual.id_lote++;
    
    // Detener alarmas
    buzzer_beeps_restantes = 0;
    GPIO_ClearValue(BUZZER_PORT, (1<<BUZZER_PIN));
}

void Calculate_Statistics(system_status_t* status) {
    // Calcular productos por minuto (suma de últimos 60 segundos)
    uint32_t productos_minuto = 0;
    for (int i = 0; i < 60; i++) {
        productos_minuto += productos_ultimo_minuto[i];
    }
    status->productos_minuto = productos_minuto;
    
    // Calcular porcentaje de rechazo
    if (status->productos_total > 0) {
        status->porcentaje_rechazo = (status->rechazos_total * 100) / status->productos_total;
    } else {
        status->porcentaje_rechazo = 0;
    }
}

void Check_Alarms(system_status_t* status) {
    uint8_t alarma_detectada = 0;
    
    // Verificar línea detenida (no hay productos en 10 segundos)
    if ((contador_segundos - ultimo_producto_tiempo) >= config.timeout_segundos) {
        alarma_detectada = 1;
    }
    
    // Verificar alta tasa de rechazo
    if (status->porcentaje_rechazo > config.max_rechazo_pct) {
        alarma_detectada = 1;
    }
    
    // Verificar producción lenta
    if (status->productos_minuto < config.min_ppm && status->productos_total > 10) {
        alarma_detectada = 1;
    }
    
    // Activar o desactivar alarma
    if (alarma_detectada && status->estado == ESTADO_RUN) {
        if (status->estado != ESTADO_ALARM) {
            status->estado = ESTADO_ALARM;
            buzzer_beeps_restantes = BUZZER_BEEPS;
            buzzer_timer = contador_segundos;
        }
    } else if (!alarma_detectada && status->estado == ESTADO_ALARM) {
        status->estado = ESTADO_RUN;
        buzzer_beeps_restantes = 0;
        GPIO_ClearValue(BUZZER_PORT, (1<<BUZZER_PIN));
    }
}

void Update_Display_Mode(void) {
    // Leer botón de display con anti-rebote simple
    uint8_t btn_actual = (GPIO_ReadValue(BTN_DISPLAY_PORT) & (1<<BTN_DISPLAY_PIN)) ? 0 : 1;
    
    if (btn_actual && !btn_display_anterior) {
        // Flanco de subida detectado - cambiar modo
        display_mode = (display_mode + 1) % 3;
    }
    btn_display_anterior = btn_actual;
}

void Update_LEDs(void) {
    // Controlar parpadeo (cada 500ms)
    if ((contador_segundos * 2) != led_parpadeo_timer) {
        led_parpadeo_timer = contador_segundos * 2;
        led_parpadeo_estado = !led_parpadeo_estado;
    }
    
    // Limpiar todos los LEDs primero
    GPIO_ClearValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
    GPIO_ClearValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
    GPIO_ClearValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
    GPIO_ClearValue(LED_AZUL_PORT, (1<<LED_AZUL_PIN));
    
    // Mostrar progreso del lote (25%, 50%, 75%, 100%)
    Batch_Show_Progress((production_batch_t*)&lote_actual);
    
    // Estado del sistema
    switch (sistema.estado) {
        case ESTADO_STOP:
            // Solo mostrar progreso del lote
            break;
            
        case ESTADO_RUN:
            GPIO_SetValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN)); // Verde fijo
            break;
            
        case ESTADO_ALARM:
            // LED amarillo parpadeando para advertencias
            if (led_parpadeo_estado) {
                GPIO_SetValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
            }
            // LED rojo fijo para problemas críticos
            GPIO_SetValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
            break;
    }
    
    // LED azul si lote completado
    if (lote_actual.completado) {
        GPIO_SetValue(LED_AZUL_PORT, (1<<LED_AZUL_PIN));
    }
}

void Update_Buzzer(void) {
    if (buzzer_beeps_restantes > 0) {
        // Generar beeps cada segundo
        if ((contador_segundos - buzzer_timer) >= 1) {
            if (buzzer_beeps_restantes % 2 == 1) {
                GPIO_SetValue(BUZZER_PORT, (1<<BUZZER_PIN));
            } else {
                GPIO_ClearValue(BUZZER_PORT, (1<<BUZZER_PIN));
            }
            buzzer_beeps_restantes--;
            buzzer_timer = contador_segundos;
        }
    }
}

void Batch_Load(production_batch_t* batch, uint32_t objetivo) {
    batch->cantidad_objetivo = objetivo;
    batch->cantidad_actual = 0;
    batch->rechazos = 0;
    batch->completado = 0;
}

uint8_t Batch_Update(production_batch_t* batch, uint32_t productos, uint32_t rechazos) {
    batch->cantidad_actual = productos;
    batch->rechazos = rechazos;
    
    if (productos >= batch->cantidad_objetivo && !batch->completado) {
        batch->completado = 1;
        return 1; // Lote completado
    }
    
    return 0; // Lote en progreso
}

void Batch_Show_Progress(production_batch_t* batch) {
    if (batch->cantidad_objetivo == 0) return;
    
    uint32_t progreso = (batch->cantidad_actual * 100) / batch->cantidad_objetivo;
    
    // Mostrar progreso en LEDs adicionales
    if (progreso >= 25) {
        // Usar parpadeo lento para indicar progreso
        if (contador_segundos % 4 < 2) {
            GPIO_SetValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
        }
    }
    if (progreso >= 50) {
        if (contador_segundos % 4 < 2) {
            GPIO_SetValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
        }
    }
    if (progreso >= 75) {
        if (contador_segundos % 4 < 2) {
            GPIO_SetValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
        }
    }
    if (progreso >= 100) {
        GPIO_SetValue(LED_AZUL_PORT, (1<<LED_AZUL_PIN));
    }
}

// ===== FUNCIÓN PRINCIPAL =====
int main(void) {
    System_Init();
    
    while (1) {
        __WFI(); // Wait for interrupt - modo de bajo consumo
    }
    
    return 0;
}