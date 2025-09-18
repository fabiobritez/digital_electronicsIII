#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_pinsel.h"

// ===== DEFINICIONES DE HARDWARE =====
#define SENSOR_GOLPE_PORT    2
#define SENSOR_GOLPE_PIN     10    // P2.10 - EINT0
#define BTN_MODO_PORT        2
#define BTN_MODO_PIN         11    // P2.11
#define LED_VERDE_PORT       0
#define LED_VERDE_PIN        22    // P0.22
#define LED_ROJO_PORT        0
#define LED_ROJO_PIN         21    // P0.21
#define LED_AMARILLO_PORT    0
#define LED_AMARILLO_PIN     20    // P0.20
#define RELE_PUERTA_PORT     0
#define RELE_PUERTA_PIN      19    // P0.19
#define BUZZER_PORT          0
#define BUZZER_PIN           18    // P0.18

// ===== CONSTANTES DEL SISTEMA =====
#define MAX_GOLPES           6
#define MIN_GOLPES           2
#define TIMEOUT_MS           1500   // 1.5 segundos
#define TOLERANCIA_MS        150    // ±150 ms
#define ANTI_REBOTE_MS       50     // Anti-rebote mínimo
#define RESOLUCION_MS        10     // Resolución del timer


#define LED_VERDE_TIEMPO_MS  3000   // LED verde 3 segundos
#define RELE_TIEMPO_MS       5000   // Relé 5 segundos

// ===== ESTRUCTURAS DE DATOS =====
typedef struct {
    uint8_t  num_golpes;           // Cantidad de golpes (2-6)
    uint16_t intervalos[5];        // Tiempo entre golpes en ms
    uint8_t  usuario_id;           // Fijo: 1
} patron_t;

typedef struct {
    uint8_t  modo;                 // 0=NORMAL, 1=GRABAR
    uint8_t  capturando;           // Flag de captura activa
    uint8_t  procesando;           // Flag de procesamiento
} sistema_t;

typedef struct {
    uint8_t  contador_golpes;
    uint32_t t_ultimo_golpe_ms;
    uint16_t intervalos[5];
} captura_t;

// ===== VARIABLES GLOBALES =====
volatile sistema_t sistema = {0, 0, 0};
volatile captura_t captura = {0, 0, {0}};
volatile uint32_t tiempo_sistema_ms = 0;
volatile uint32_t led_verde_timer = 0;
volatile uint32_t led_rojo_timer = 0;
volatile uint32_t led_amarillo_timer = 0;
volatile uint32_t rele_timer = 0;
volatile uint8_t led_rojo_parpadeos = 0;
volatile uint8_t led_amarillo_parpadeos = 0;

// Patrón de fábrica por defecto: "Toc-Toc...pausa...Toc" (100ms, 800ms)
patron_t patron_usuario_1 = {3, {100, 800, 0, 0, 0}, 1};

// ===== PROTOTIPOS DE FUNCIONES =====
void Sistema_Init(void);
void configTimer(void);
void configGPIO(void);
void configEINT(void);
void CargarPatronPorDefecto(void);
void GuardarPatron(patron_t* p);
void EINT0_IRQHandler(void);
void TIMER0_IRQHandler(void);
void IniciarCaptura(void);
void ProcesarGolpeCaptura(void);
void FinalizarCaptura(void);
uint8_t Patron_Comparar(const patron_t* ref, const captura_t* med);
void ConcederAcceso(void);
void DenegarAcceso(void);
void ActualizarModo(void);
void ActualizarInterfaz(void); //LEDs y relé
void Buzzer_Beep(void);



void Sistema_Init(void) {
    SystemInit();
    configGPIO();
    configTimer();
    configEINT();
    
    // Cargar patrón por defecto, TODO: desde EEPROM/Flash
    CargarPatronPorDefecto();
    
    // Estado inicial
    sistema.modo = 0; // NORMAL
    sistema.capturando = 0;
    sistema.procesando = 0;
    
    // LEDs iniciales
    GPIO_ClearValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
    GPIO_ClearValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
    GPIO_ClearValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
    GPIO_ClearValue(RELE_PUERTA_PORT, (1<<RELE_PUERTA_PIN));
}

void configGPIO(void) {
    // Configurar pines como GPIO
    PINSEL_CFG_Type PinCfg;
    
    // Sensor golpe (P2.10) - ya configurado como EINT0
    // Botón modo
    PinCfg.Portnum = BTN_MODO_PORT;
    PinCfg.Pinnum = BTN_MODO_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_0; // GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(BTN_MODO_PORT, (1<<BTN_MODO_PIN), 0); // Input
    
    // LEDs como salidas
    GPIO_SetDir(LED_VERDE_PORT, (1<<LED_VERDE_PIN), 1);
    GPIO_SetDir(LED_ROJO_PORT, (1<<LED_ROJO_PIN), 1);
    GPIO_SetDir(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN), 1);
    GPIO_SetDir(RELE_PUERTA_PORT, (1<<RELE_PUERTA_PIN), 1);
    GPIO_SetDir(BUZZER_PORT, (1<<BUZZER_PIN), 1);
}

void configTimer(void) {
    TIM_TIMERCFG_Type TimerConfig;
    TIM_MATCHCFG_Type MatchConfig;
    
    // Configurar Timer0 para generar interrupciones cada 10ms
    TimerConfig.PrescaleOption = TIM_PRESCALE_TICKVAL;
    TimerConfig.PrescaleValue = 25000; // Para 10ms con reloj de 25MHz
    
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TimerConfig);
    
    // Configurar match para 10ms
    MatchConfig.MatchChannel = 0;
    MatchConfig.IntOnMatch = TRUE;
    MatchConfig.ResetOnMatch = TRUE;
    MatchConfig.StopOnMatch = FALSE;
    MatchConfig.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    MatchConfig.MatchValue = 1; // 1 tick = 10ms con prescaler configurado
    
    TIM_ConfigMatch(LPC_TIM0, &MatchConfig);
    
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void configEINT(void) {
    PINSEL_CFG_Type PinCfg;
    EXTI_InitTypeDef EXTICfg;
    
    // Configurar P2.10 como EINT0
    PinCfg.Portnum = SENSOR_GOLPE_PORT;
    PinCfg.Pinnum = SENSOR_GOLPE_PIN;
    PinCfg.Funcnum = PINSEL_FUNC_1; // EINT0
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);
    
    // Configurar interrupción externa
    EXTICfg.EXTI_Line = EXTI_EINT0;
    EXTICfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTICfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    
    EXTI_Init();
    EXTI_Config(&EXTICfg);
    
    NVIC_EnableIRQ(EINT0_IRQn);
}

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        
        tiempo_sistema_ms += RESOLUCION_MS;
        
        // Verificar timeout de captura
        if (sistema.capturando) {
            if ((tiempo_sistema_ms - captura.t_ultimo_golpe_ms) >= TIMEOUT_MS) {
                FinalizarCaptura();
            }
        }
        
        // Actualizar LEDs y relé
        ActualizarInterfaz();
    }
}

void EINT0_IRQHandler(void) {
    if (EXTI_GetEXTIFlag(EXTI_EINT0)) {
        EXTI_ClearEXTIFlag(EXTI_EINT0);
        
        // Anti-rebote por software
        uint32_t tiempo_actual = tiempo_sistema_ms;
        if ((tiempo_actual - captura.t_ultimo_golpe_ms) >= ANTI_REBOTE_MS) {
            ProcesarGolpeCaptura();
            Buzzer_Beep();
        }
    }
}

/**
 */
void IniciarCaptura(void) {
    captura.contador_golpes = 0;
    captura.t_ultimo_golpe_ms = tiempo_sistema_ms;
    sistema.capturando = 1;
    
    // Limpiar intervalos
    for (int i = 0; i < 5; i++) {
        captura.intervalos[i] = 0;
    }
}

void ProcesarGolpeCaptura(void) {
    if (!sistema.capturando && !sistema.procesando) {
        // Primer golpe - iniciar captura
        IniciarCaptura();
        captura.contador_golpes = 1;
    } else if (sistema.capturando) {
        // Golpe subsecuente
        uint32_t tiempo_actual = tiempo_sistema_ms;
        uint32_t intervalo = tiempo_actual - captura.t_ultimo_golpe_ms;
        
        if (captura.contador_golpes < MAX_GOLPES) {
            // Guardar intervalo (normalizar a múltiplos de 10ms)
            captura.intervalos[captura.contador_golpes - 1] = 
                ((intervalo + RESOLUCION_MS/2) / RESOLUCION_MS) * RESOLUCION_MS;
            captura.contador_golpes++;
        }
        
        captura.t_ultimo_golpe_ms = tiempo_actual;
        
        // Si alcanzamos el máximo, finalizar automáticamente
        if (captura.contador_golpes >= MAX_GOLPES) {
            FinalizarCaptura();
        }
    }
}

void FinalizarCaptura(void) {
    if (captura.contador_golpes >= MIN_GOLPES) {
        sistema.capturando = 0;
        sistema.procesando = 1;
        
        if (sistema.modo == 0) {
            // Modo NORMAL - reconocer patrón
            if (Patron_Comparar(&patron_usuario_1, &captura)) {
                ConcederAcceso();
            } else {
                DenegarAcceso();
            }
        } else {
            // Modo GRABAR - guardar nuevo patrón
            patron_usuario_1.num_golpes = captura.contador_golpes;
            for (int i = 0; i < captura.contador_golpes - 1; i++) {
                patron_usuario_1.intervalos[i] = captura.intervalos[i];
            }
            GuardarPatron(&patron_usuario_1);
            
            // Confirmar grabación con LED amarillo
            led_amarillo_parpadeos = 3;
            led_amarillo_timer = tiempo_sistema_ms;
        }
        
        sistema.procesando = 0;
    } else {
        // Muy pocos golpes, reiniciar
        sistema.capturando = 0;
        sistema.procesando = 0;
    }
}

uint8_t Patron_Comparar(const patron_t* ref, const captura_t* med) {
    if (ref->num_golpes != med->contador_golpes) {
        return 0;
    }
    
    for (uint8_t i = 0; i < ref->num_golpes - 1; i++) {
        uint16_t diff = (ref->intervalos[i] > med->intervalos[i])
                        ? (ref->intervalos[i] - med->intervalos[i])
                        : (med->intervalos[i] - ref->intervalos[i]);
        if (diff > TOLERANCIA_MS) {
            return 0;  // Fuera de tolerancia
        }
    }
    return 1; // Patrón válido
}


void ConcederAcceso(void) {
    // LED verde por 3 segundos
    GPIO_SetValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
    led_verde_timer = tiempo_sistema_ms + LED_VERDE_TIEMPO_MS;
    
    // Activar relé por 5 segundos
    GPIO_SetValue(RELE_PUERTA_PORT, (1<<RELE_PUERTA_PIN));
    rele_timer = tiempo_sistema_ms + RELE_TIEMPO_MS;
}

void DenegarAcceso(void) {
    // LED rojo parpadea 3 veces
    led_rojo_parpadeos = 6; // 3 parpadeos = 6 cambios de estado
    led_rojo_timer = tiempo_sistema_ms;
}

void ActualizarModo(void) {
    // Leer estado del botón
    uint8_t btn_estado = GPIO_ReadValue(BTN_MODO_PORT) & (1<<BTN_MODO_PIN);
    
    if (btn_estado == 0) { // Botón presionado (pull-up)
        sistema.modo = 1; // GRABAR
        GPIO_SetValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
    } else {
        sistema.modo = 0; // NORMAL
        if (led_amarillo_parpadeos == 0) {
            GPIO_ClearValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
        }
    }
}

void ActualizarInterfaz(void) {
    // Actualizar modo
    ActualizarModo();
    
    // LED Verde
    if (led_verde_timer > 0 && tiempo_sistema_ms >= led_verde_timer) {
        GPIO_ClearValue(LED_VERDE_PORT, (1<<LED_VERDE_PIN));
        led_verde_timer = 0;
    }
    
    // LED Rojo (parpadeo)
    if (led_rojo_parpadeos > 0) {
        if ((tiempo_sistema_ms - led_rojo_timer) >= 100) { // Cada 100ms
            if (GPIO_ReadValue(LED_ROJO_PORT) & (1<<LED_ROJO_PIN)) {
                GPIO_ClearValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
            } else {
                GPIO_SetValue(LED_ROJO_PORT, (1<<LED_ROJO_PIN));
            }
            led_rojo_parpadeos--;
            led_rojo_timer = tiempo_sistema_ms;
        }
    }
    
    // LED Amarillo (confirmación de grabación)
    if (led_amarillo_parpadeos > 0 && sistema.modo == 0) {
        if ((tiempo_sistema_ms - led_amarillo_timer) >= 200) { // Cada 200ms
            if (GPIO_ReadValue(LED_AMARILLO_PORT) & (1<<LED_AMARILLO_PIN)) {
                GPIO_ClearValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
            } else {
                GPIO_SetValue(LED_AMARILLO_PORT, (1<<LED_AMARILLO_PIN));
            }
            led_amarillo_parpadeos--;
            led_amarillo_timer = tiempo_sistema_ms;
        }
    }
    
    // Relé
    if (rele_timer > 0 && tiempo_sistema_ms >= rele_timer) {
        GPIO_ClearValue(RELE_PUERTA_PORT, (1<<RELE_PUERTA_PIN));
        rele_timer = 0;
    }
}

void Buzzer_Beep(void) {
    // Beep corto de confirmación
    GPIO_SetValue(BUZZER_PORT, (1<<BUZZER_PIN));
    // Nota: En aplicación real, usar un timer separado para apagar el buzzer
    // Por simplicidad, se apagaría en la siguiente actualización del sistema
}

void CargarPatronPorDefecto(void) {
    // El patrón ya está inicializado como variable global
    // En implementación real, cargar desde EEPROM/Flash
}

void GuardarPatron(patron_t* p) {
    // En implementación real, guardar en EEPROM/Flash
    // Por ahora, el patrón ya está actualizado en memoria
}

// ===== FUNCIÓN PRINCIPAL =====
int main(void) {
    Sistema_Init();
    
    while (1) {
        __WFI(); // Wait for interrupt - bajo consumo
    }
    
    return 0;
}