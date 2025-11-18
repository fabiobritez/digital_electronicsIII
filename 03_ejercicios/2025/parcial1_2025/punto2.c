/*
 * Parpadeo de LED con período proporcional a señal de entrada 
 *  
 * - t: contador de milisegundos (modificado de flag a contador)
 * - t_prev: tiempo anterior para cálculo de período
 * - periodo: período medido de la señal
 */

 #include "LPC17xx.h"

 #define LED (1<<18)
 
 // Variables originales del ejercicio
 volatile uint32_t t, t_prev, periodo;
 
 // Variables adicionales necesarias para el funcionamiento
 volatile uint32_t t_led = 0;           // Tiempo del último cambio del LED
 volatile uint32_t periodo_led = 500;   // Período del LED (limitado 100-1000ms)
 
 /**
  * Handler de SysTick - Base de tiempo de 1ms
  * Modificado: 't' ahora es contador de milisegundos, no un flag
  */
 void SysTick_Handler(void)
 {
     t++;  // Incrementar contador de milisegundos
 }
 
 /**
  * Handler de EINT1 - Medición del período
  * Corregido para usar contador ascendente
  */
 void EINT1_IRQHandler(void)
 {
     if(LPC_SC->EXTINT & (1<<1))
     {
         // Usar 't' como referencia de tiempo (contador de ms)
         uint32_t now = t;
         
         // Calcular período solo si no es la primera medición
         if(t_prev != 0)
         {
             periodo = now - t_prev;
             
             // Limitar período del LED entre 100 y 1000ms
             if(periodo < 100)
                 periodo_led = 100;
             else if(periodo > 1000)
                 periodo_led = 1000;
             else
                 periodo_led = periodo;
         }
         
         t_prev = now;
         
         // Limpiar flag de interrupción
         LPC_SC->EXTINT = (1<<1);
     }
 }
 
 /**
  * Configuración de SysTick para 1ms
  * Corregido: operadores OR en lugar de AND
  */
 void systick_config(void)
 {
     // Para 1ms con clock de 100MHz
     // RELOAD = (100,000,000 / 1,000) - 1 = 99,999
     SysTick->LOAD = SystemCoreClock/1000 - 1;  // Ajustado para 1ms
     SysTick->VAL = 0;
     
     // CORREGIDO: Usar OR (|) en lugar de AND (&)
     SysTick->CTRL = (1<<0) | (1<<1) | (1<<2);  // Enable | TickInt | ClkSource
     
     // Configurar prioridad: SysTick menor que EINT1
     NVIC_SetPriority(SysTick_IRQn, 3);  // Prioridad baja
 }
 
 /**
  * Configuración de EINT1
  * Corregido completamente
  */
 void eint1_config_reg(void)
 {
     // Configurar P2.11 como EINT1
     LPC_PINCON->PINSEL4 &= ~(3<<22);
     LPC_PINCON->PINSEL4 |= (1<<22);    // Función EINT1
     
     // CORREGIDO: Configurar modo de interrupción EINT1 (no GPIO interrupt)
     // Modo flanco (edge-triggered)
     LPC_SC->EXTMODE |= (1<<1);
     
     // Flanco ascendente
     LPC_SC->EXTPOLAR |= (1<<1);
     
     // Limpiar flag pendiente
     LPC_SC->EXTINT = (1<<1);
     
     // CORREGIDO: Habilitar EINT1_IRQn, no EINT3_IRQn
     NVIC_EnableIRQ(EINT1_IRQn);
     
     // Configurar prioridad: EINT1 mayor que SysTick
     NVIC_SetPriority(EINT1_IRQn, 1);   // Prioridad alta
 }
 
 /**
  * Configuración del LED en P1.18
  * Función agregada (no estaba en el original)
  */
 void led_config(void)
 {
     // P1.18 como GPIO
     LPC_PINCON->PINSEL3 &= ~(3<<4);
     
     // P1.18 como salida
     LPC_GPIO1->FIODIR |= LED;
     
     // LED inicialmente apagado
     LPC_GPIO1->FIOCLR = LED;
 }
 
 int main(void)
 {
     // Inicialización del sistema
     SystemInit();
     
     // Configuraciones
     led_config();     
     systick_config();
     eint1_config_reg();
     
     // Inicializar variables
     t = 0;
     t_prev = 0;
     periodo = 500;      // Valor inicial
     periodo_led = 500;  // Período inicial del LED
     t_led = 0;
     
     while(1)
     {
         // El LED cambia cada periodo_led/2 ms
         if((t - t_led) >= (periodo_led / 2))
         {
             // Toggle LED
             if(LPC_GPIO1->FIOPIN & LED)
                 LPC_GPIO1->FIOCLR = LED;  // Apagar
             else
                 LPC_GPIO1->FIOSET = LED;  // Encender
             
             t_led = t;  // Actualizar tiempo del último cambio
         }
     }
     
     return 0;
 }
 