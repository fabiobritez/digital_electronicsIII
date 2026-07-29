/*
 * Parpadeo de LED con período proporcional a señal de entrada
 * 
 * Descripción:
 * - Mide el período de una señal cuadrada en EINT1 (P2.11)
 * - LED en P1.18 parpadea con período proporcional (100-1000ms)
 * - SysTick como base de tiempo para medición y control
 */

 #include "LPC17xx.h"

 // Definiciones
 #define LED_PIN         (1 << 18)      // P1.18
 #define MIN_PERIOD_MS   100             // Período mínimo del LED
 #define MAX_PERIOD_MS   1000            // Período máximo del LED
 
 // Variables globales
 volatile uint32_t milliseconds = 0;     // Contador de milisegundos
 volatile uint32_t period_start = 0;     // Tiempo del último flanco
 volatile uint32_t measured_period = 500; // Período medido (inicializado a 500ms)
 volatile uint32_t led_period = 500;     // Período del LED limitado
 volatile uint32_t last_led_toggle = 0;  // Último cambio del LED
 volatile uint8_t new_period_flag = 0;   // Flag de nuevo período disponible
 
 // Prototipos
 void config_GPIO(void);
 void config_SysTick(void);
 void config_EINT1(void);
 void update_led_period(void);
 void toggle_led(void);
 
 int main(void)
 {
     // Inicialización del sistema
     SystemInit();
     
     // Configuración de periféricos
     config_GPIO();
     config_EINT1();
     config_SysTick();
     
     // Bucle principal
     while(1)
     {
         // Actualizar período del LED si hay nueva medición
         if (new_period_flag)
         {
             update_led_period();
             new_period_flag = 0;
         }
         
         // Controlar parpadeo del LED
         // El LED cambia cada led_period/2 ms para completar un ciclo en led_period ms
         if ((milliseconds - last_led_toggle) >= (led_period / 2))
         {
             toggle_led();
             last_led_toggle = milliseconds;
         }
     }
     
     return 0;
 }
 
 /**
  * Handler de SysTick - Se ejecuta cada 1ms
  * Base de tiempo para medición y control
  */
 void SysTick_Handler(void)
 {
     milliseconds++;
 }
 
 /**
  * Handler de EINT1 - Detecta flancos en P2.11
  * Mide el período entre flancos ascendentes
  */
 void EINT1_IRQHandler(void)
 {
     // Verificar y limpiar flag de interrupción
     if (LPC_SC->EXTINT & (1 << 1))
     {
         uint32_t current_time = milliseconds;
         
         // Calcular período solo si no es la primera vez
         if (period_start != 0)
         {
             uint32_t new_period = current_time - period_start;
             
             // Filtrar mediciones válidas (evitar glitches)
             if (new_period > 10 && new_period < 5000)
             {
                 measured_period = new_period;
                 new_period_flag = 1;
             }
         }
         
         // Actualizar tiempo para próxima medición
         period_start = current_time;
         
         // Limpiar flag de interrupción (escribir 1 para limpiar)
         LPC_SC->EXTINT = (1 << 1);
     }
 }
 
 /**
  * Configuración de GPIO
  * P1.18: Salida para LED
  */
 void config_GPIO(void)
 {
     // Configurar P1.18 como GPIO
     LPC_PINCON->PINSEL3 &= ~(3 << 4);   // P1.18 = GPIO (bits [5:4] = 00)
     
     // Configurar P1.18 como salida
     LPC_GPIO1->FIODIR |= LED_PIN;
     
     // LED inicialmente apagado
     LPC_GPIO1->FIOCLR = LED_PIN;
 }
 
 /**
  * Configuración de SysTick
  * Configurado para interrumpir cada 1ms
  * 
  * Asumiendo CCLK = 100MHz (típico en LPC1769)
  * RELOAD = (100,000,000 / 1000) - 1 = 99,999
  */
 void config_SysTick(void)
 {
     // Configurar para 1ms @ 100MHz
     // Si el reloj es diferente, ajustar este valor
     SysTick->LOAD = SystemCoreClock / 1000 - 1;  // Para 1ms
     
     // Limpiar contador actual
     SysTick->VAL = 0;
     
     // Configurar y habilitar SysTick
     SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // Bit 2: Clock del procesador
                     SysTick_CTRL_TICKINT_Msk |    // Bit 1: Habilitar interrupción
                     SysTick_CTRL_ENABLE_Msk;      // Bit 0: Habilitar SysTick
     
     // Configurar prioridad
     // SysTick debe tener MENOR prioridad que EINT1
     // para no interferir con la medición de tiempo
     NVIC_SetPriority(SysTick_IRQn, 3);  // Prioridad baja
 }
 
 /**
  * Configuración de EINT1
  * P2.11: Entrada para EINT1
  * Modo: Detección por flanco ascendente
  */
 void config_EINT1(void)
 {
     // Configurar P2.11 como EINT1
     // PINSEL4[23:22] = 01 para función EINT1
     LPC_PINCON->PINSEL4 &= ~(3 << 22);   // Limpiar bits [23:22]
     LPC_PINCON->PINSEL4 |= (1 << 22);    // Establecer función EINT1
     
     // Configurar pull-down en P2.11 para estado definido
     // PINMODE4[23:22] = 11 para pull-down
     LPC_PINCON->PINMODE4 &= ~(3 << 22);  // Limpiar bits
     LPC_PINCON->PINMODE4 |= (3 << 22);   // Pull-down
     
     // Configurar EINT1 para detección por flanco
     // EXTMODE[1] = 1: Sensible a flanco (no a nivel)
     LPC_SC->EXTMODE |= (1 << 1);
     
     // Configurar para flanco ascendente
     // EXTPOLAR[1] = 1: Flanco ascendente
     LPC_SC->EXTPOLAR |= (1 << 1);
     
     // Limpiar flag de interrupción pendiente
     LPC_SC->EXTINT = (1 << 1);
     
     // Habilitar interrupción EINT1 en NVIC
     NVIC_EnableIRQ(EINT1_IRQn);
     
     // Configurar prioridad
     // EINT1 debe tener MAYOR prioridad que SysTick
     // para medición precisa del período
     NVIC_SetPriority(EINT1_IRQn, 1);  // Prioridad alta
 }
 
 /**
  * Actualiza el período del LED basado en la medición
  * Limita el rango entre MIN_PERIOD_MS y MAX_PERIOD_MS
  */
 void update_led_period(void)
 {
     if (measured_period < MIN_PERIOD_MS)
     {
         led_period = MIN_PERIOD_MS;
     }
     else if (measured_period > MAX_PERIOD_MS)
     {
         led_period = MAX_PERIOD_MS;
     }
     else
     {
         led_period = measured_period;
     }
 }
 
 /**
  * Cambia el estado del LED
  */
 void toggle_led(void)
 {
     // Leer estado actual y cambiar
     if (LPC_GPIO1->FIOPIN & LED_PIN)
     {
         LPC_GPIO1->FIOCLR = LED_PIN;  // Apagar
     }
     else
     {
         LPC_GPIO1->FIOSET = LED_PIN;  // Encender
     }
 }
