/*
 * Generador de Secuencia Temporal -  con Array
 * 
 * Descripción:
 * - Cada elemento del array representa 10ms
 * - Valor 0 = P2.4 en bajo, Valor 1 = P2.4 en alto
 * - EINT2 inicia la secuencia
 * - Nueva interrupción aborta y pone P2.4 en alto
 * - Frecuencia del sistema: 60 MHz
 */

 #include "LPC17xx.h"

 // Array simple: cada elemento = 10ms
 // 0 = bajo, 1 = alto
 const uint8_t sequence[] = {
     // 40ms en bajo (4 × 10ms)
     0, 0, 0, 0,
     
     // Pulso 1: 10ms alto, 10ms bajo
     1, 0,
     
     // Pulso 2: 10ms alto, 10ms bajo
     1, 0,
     
     // Pulso 3: 10ms alto, 10ms bajo
     1, 0,
     
     // Pulso 4: 10ms alto, 10ms bajo
     1, 0,
     
     // 40ms en bajo (4 × 10ms)
     0, 0, 0, 0
 };
 
 // Tamaño del array (16 elementos = 160ms total)
 #define SEQUENCE_LENGTH     (sizeof(sequence) / sizeof(uint8_t))
 
 // Variables globales
 volatile uint8_t sequence_active = 0;   // Secuencia activa
 volatile uint8_t abort_requested = 0;   // Solicitud de aborto
 volatile uint8_t sequence_index = 0;    // Índice actual en el array
 
 // Prototipos
 void config_GPIO(void);
 void config_EINT2(void);
 void config_SysTick(void);
 
 int main(void)
 {
     // Inicialización
     SystemInit();
     
     // Configuración
     config_GPIO();
     config_EINT2();
     config_SysTick();
     
     while (1)
     {
         __WFI();  // Esperar interrupciones
     }
     
     return 0;
 }
 
 /**
  * Configuración de GPIO
  * P2.4: Salida para la secuencia
  */
 void config_GPIO(void)
 {
     // P2.4 como GPIO
     LPC_PINCON->PINSEL4 &= ~(3 << 8);
     
     // P2.4 como salida
     LPC_GPIO2->FIODIR |= (1 << 4);
     
     // Enmascarar otros pines
     LPC_GPIO2->FIOMASK = ~(1 << 4);
     
     // Estado inicial alto (idle)
     LPC_GPIO2->FIOSET = (1 << 4);
 }
 
 /**
  * Configuración de EINT2
  * P2.12: Entrada, flanco descendente
  */
 void config_EINT2(void)
 {
     // P2.12 como EINT2
     LPC_PINCON->PINSEL4 &= ~(3 << 24);
     LPC_PINCON->PINSEL4 |= (1 << 24);
     
     // Pull-up en P2.12
     LPC_PINCON->PINMODE4 &= ~(3 << 24);
     
     // EINT2 flanco descendente
     LPC_SC->EXTMODE |= (1 << 2);
     LPC_SC->EXTPOLAR &= ~(1 << 2);
     
     // Limpiar flag
     LPC_SC->EXTINT = (1 << 2);
     
     // Habilitar EINT2
     NVIC_EnableIRQ(EINT2_IRQn);
     NVIC_SetPriority(EINT2_IRQn, 1);
 }
 
 /**
  * Configuración del SysTick
  * 60 MHz → 10ms
  * RELOAD = 599,999
  */
 void config_SysTick(void)
 {
     SysTick->LOAD = 599999;       // Para 10ms @ 60MHz
     SysTick->VAL = 0;
     SysTick->CTRL = (1 << 2) |   // Clock del procesador
                     (1 << 1) |   // Habilitar interrupción
                     (1 << 0);    // Habilitar SysTick
     
     NVIC_SetPriority(SysTick_IRQn, 2);
 }
 
 /**
  * Handler EINT2
  */
 void EINT2_IRQHandler(void)
 {
     // Limpiar flag
     LPC_SC->EXTINT = (1 << 2);
     
     if (!sequence_active)
     {
         // Iniciar secuencia
         sequence_active = 1;
         sequence_index = 0;
         
         // Aplicar primer valor
         if (sequence[0])
             LPC_GPIO2->FIOSET = (1 << 4);
         else
             LPC_GPIO2->FIOCLR = (1 << 4);
     }
     else
     {
         // Abortar secuencia
         abort_requested = 1;
     }
 }
 
 /**
  * Handler SysTick - cada 1ms
  */
 void SysTick_Handler(void)
 {
     // Verificar aborto
     if (abort_requested)
     {
         LPC_GPIO2->FIOSET = (1 << 4);  // P2.4 = alto
         sequence_active = 0;
         sequence_index = 0;
         tick_counter = 0;
         abort_requested = 0;
         return;
     }
     
     // Procesar secuencia activa
     if (sequence_active)
     {
         
         // Cada 10ms, avanzar al siguiente elemento
         
             sequence_index++;
             
             // Verificar fin de secuencia
             if (sequence_index >= SEQUENCE_LENGTH)
             {
                 // Fin: volver a idle
                 LPC_GPIO2->FIOSET = (1 << 4);  // P2.4 = alto
                 sequence_active = 0;
                 sequence_index = 0;
             }
             else
             {
                 // Aplicar siguiente valor del array
                 if (sequence[sequence_index])
                     LPC_GPIO2->FIOSET = (1 << 4);   // Alto
                 else
                     LPC_GPIO2->FIOCLR = (1 << 4);   // Bajo
             }
     }
 }
 