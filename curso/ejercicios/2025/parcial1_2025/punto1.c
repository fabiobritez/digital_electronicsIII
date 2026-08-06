/*
 * Generador de Secuencia Temporal con EINT2 y SysTick
 * 
 * Descripción:
 * - EINT2 (flanco descendente) inicia la secuencia en P2.4
 * - Secuencia: 40ms bajo, 4 pulsos de 10ms, 40ms bajo
 * - Nueva interrupción durante secuencia: aborta y pone P2.4 en alto
 * - Sin retardos por software, solo SysTick
 * - Frecuencia del sistema: 60 MHz
 */

 #include "LPC17xx.h"

 // Estados
 typedef enum {
     STATE_IDLE,           // Esperando interrupción EINT2
     STATE_INITIAL_LOW,    // Primeros 40ms en bajo
     STATE_PULSE_HIGH,     // Parte alta del pulso (10ms)
     STATE_PULSE_LOW,      // Parte baja del pulso (10ms)
     STATE_FINAL_LOW,      // Últimos 40ms en bajo
     STATE_ABORTED         // Secuencia abortada
 } SequenceState;
 
 // Variables globales
 volatile SequenceState current_state = STATE_IDLE;
 volatile uint32_t time_counter = 0;      // Contador de tiempo en ms
 volatile uint8_t pulse_counter = 0;      // Contador de pulsos (0-3)
 volatile uint8_t sequence_active = 0;    // Flag de secuencia activa
 volatile uint8_t abort_requested = 0;    // Flag de aborto solicitado
 
 // Prototipos de funciones
 void config_GPIO(void);
 void config_EINT2(void);
 void config_SysTick(void);
 void start_sequence(void);
 void abort_sequence(void);
 void process_sequence(void);
 
 int main(void)
 {
     // Inicialización del sistema
     SystemInit();
     
     // Configuración de periféricos
     config_GPIO();
     config_EINT2();
     config_SysTick();
     
     
     while (1)
     {
         __WFI();  // Wait for Interrupt
     }
     
     return 0;
 }
 
 /**
  * Configuración de GPIO
  * P2.4: Salida para la secuencia
  * Otros pines del puerto 2: Enmascarados (no utilizados)
  */
 void config_GPIO(void)
 {
     // === Configurar P2.4 como GPIO ===
     LPC_PINCON->PINSEL4 &= ~(3 << 8);   // P2.4 = GPIO (bits [9:8] = 00)
     
     // === Configurar P2.4 como salida ===
     LPC_GPIO2->FIODIR |= (1 << 4);      // P2.4 como salida
     
     // === Enmascarar pines no utilizados del puerto 2 ===
     // Usamos FIOMASK para que las operaciones solo afecten a P2.4
     LPC_GPIO2->FIOMASK = ~(1 << 4);     // Solo P2.4 no está enmascarado
     
     // === Estado inicial: P2.4 en alto (idle) ===
     LPC_GPIO2->FIOSET = (1 << 4);
 }
 
 /**
  * Configuración de EINT2
  * P2.12: Entrada para EINT2
  * Detección por flanco descendente
  */
 void config_EINT2(void)
 {
     // === Configurar P2.12 como EINT2 ===
     // PINSEL4[25:24] = 01 para función EINT2
     LPC_PINCON->PINSEL4 &= ~(3 << 24);  // Limpiar bits [25:24]
     LPC_PINCON->PINSEL4 |= (1 << 24);   // Establecer función EINT2
     
     // === Configurar pull-up en P2.12 ===
     // PINMODE4[25:24] = 00 para pull-up
     LPC_PINCON->PINMODE4 &= ~(3 << 24); // Pull-up habilitado
     
     // === Configurar EINT2 para flanco descendente ===
     LPC_SC->EXTMODE |= (1 << 2);        // EINT2 sensible a flanco
     LPC_SC->EXTPOLAR &= ~(1 << 2);      // EINT2 flanco descendente (0)
     
     // === Limpiar flag de interrupción pendiente ===
     LPC_SC->EXTINT = (1 << 2);
     
     // === Habilitar interrupción EINT2 en NVIC ===
     NVIC_EnableIRQ(EINT2_IRQn);
     NVIC_SetPriority(EINT2_IRQn, 1);    // Prioridad alta para EINT2
 }
 
 /**
  * Configuración del SysTick
  * Frecuencia del sistema: 60 MHz
  * Período del SysTick: 1 ms
  * 
  * Cálculo:
  * RELOAD = (60,000,000 Hz / 1000) - 1 = 59,999
  */
 void config_SysTick(void)
 {
     // Cálculo del valor RELOAD para 1ms con clock de 60MHz
     // RELOAD = (CCLK / 1000) - 1 = (60,000,000 / 1000) - 1 = 59,999
     uint32_t reload_value = 59999;
     
     // Cargar el valor de recarga
     SysTick->LOAD = reload_value;
     
     // Limpiar el valor actual del contador
     SysTick->VAL = 0;
     
     // Configurar y habilitar SysTick
     SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // Usar reloj del procesador
                     SysTick_CTRL_TICKINT_Msk |    // Habilitar interrupción
                     SysTick_CTRL_ENABLE_Msk;      // Habilitar SysTick
     
     // Configurar prioridad del SysTick (menor que EINT2)
     NVIC_SetPriority(SysTick_IRQn, 2);
 }
 
 /**
  * Handler de interrupción EINT2
  * Se ejecuta en cada flanco descendente en P2.12
  */
 void EINT2_IRQHandler(void)
 {
     // Limpiar flag de interrupción EINT2
     LPC_SC->EXTINT = (1 << 2);
     
     if (!sequence_active)
     {
         // No hay secuencia activa, iniciar una nueva
         start_sequence();
     }
     else
     {
         // Secuencia en progreso, solicitar aborto
         abort_requested = 1;
     }
 }
 
 /**
  * Handler de interrupción SysTick
  * Se ejecuta cada 1ms
  */
 void SysTick_Handler(void)
 {
     // Verificar si hay solicitud de aborto
     if (abort_requested)
     {
         abort_sequence();
         abort_requested = 0;
         return;
     }
     
     // Procesar la secuencia si está activa
     if (sequence_active)
     {
         process_sequence();
     }
 }
 
 /**
  * Inicia una nueva secuencia
  */
 void start_sequence(void)
 {
     // Inicializar variables
     current_state = STATE_INITIAL_LOW;
     time_counter = 0;
     pulse_counter = 0;
     sequence_active = 1;
     
     // Poner P2.4 en bajo (inicio de secuencia)
     LPC_GPIO2->FIOCLR = (1 << 4);
 }
 
 /**
  * Aborta la secuencia actual
  */
 void abort_sequence(void)
 {
     // Poner P2.4 en alto
     LPC_GPIO2->FIOSET = (1 << 4);
     
     // Reiniciar variables
     current_state = STATE_IDLE;
     time_counter = 0;
     pulse_counter = 0;
     sequence_active = 0;
 }
 
 /**
  * Procesa la máquina de estados de la secuencia
  * Se ejecuta cada 1ms desde el SysTick Handler
  */
 void process_sequence(void)
 {
     time_counter++;
     
     switch (current_state)
     {
         case STATE_INITIAL_LOW:
             // Primeros 40ms en bajo
             if (time_counter >= 40)
             {
                 // Cambiar a primer pulso alto
                 LPC_GPIO2->FIOSET = (1 << 4);  // P2.4 = 1
                 current_state = STATE_PULSE_HIGH;
                 time_counter = 0;
                 pulse_counter = 0;
             }
             break;
             
         case STATE_PULSE_HIGH:
             // Parte alta del pulso (10ms)
             if (time_counter >= 10)
             {
                 // Cambiar a bajo
                 LPC_GPIO2->FIOCLR = (1 << 4);  // P2.4 = 0
                 
                 // Verificar si es el último pulso
                 if (pulse_counter >= 3)
                 {
                     // Ir a los últimos 40ms en bajo
                     current_state = STATE_FINAL_LOW;
                 }
                 else
                 {
                     // Continuar con siguiente parte baja del pulso
                     current_state = STATE_PULSE_LOW;
                 }
                 time_counter = 0;
             }
             break;
             
         case STATE_PULSE_LOW:
             // Parte baja del pulso (10ms)
             if (time_counter >= 10)
             {
                 // Cambiar a alto para siguiente pulso
                 LPC_GPIO2->FIOSET = (1 << 4);  // P2.4 = 1
                 current_state = STATE_PULSE_HIGH;
                 time_counter = 0;
                 pulse_counter++;  // Incrementar contador de pulsos
             }
             break;
             
         case STATE_FINAL_LOW:
             // Últimos 40ms en bajo
             if (time_counter >= 40)
             {
                 // Secuencia completada, volver a idle
                 LPC_GPIO2->FIOSET = (1 << 4);  // P2.4 = 1 (idle)
                 current_state = STATE_IDLE;
                 sequence_active = 0;
                 time_counter = 0;
                 pulse_counter = 0;
             }
             break;
             
         case STATE_IDLE:
         case STATE_ABORTED:
         default:
             // No hacer nada
             break;
     }
 }
 