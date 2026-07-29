/*
 * Ejercicio N° 2 - Parcial 2022
 *
 * Utilizando interrupciones por systick y por eventos externos EINT.
 * Realizar un programa que permita habilitar y deshabilitar el temporizador por
 * el flanco ascendente en el pin P2.11.
 *
 * El temporizador debe desbordar cada 10 milisegundos utilizando un reloj de core cclk = 62MHz.
 * Por cada interrupcion del Systick se debe mostrar por el puerto P0 el promedio de los
 * valores guardados en la variables "uint8_t values[8]"
 *
 * Se pide ademas detallar los calculos realizados para obtener el valor a cargar en el
 * registro RELOAD y asegurar que la interrupcion por Systick sea mayor que la
 * prioridad de la interrupcion del evento externo.
 *
 * El codigo debe estar debidamente comentado.
 */

 #include "LPC17xx.h"
 #include <stdint.h>
 
 void configSysTick(void);
 void configGPIO(void);
 void configEINT(void);
 uint8_t calcularPromedio(uint8_t values[8]);
 
 // Variables globales
 uint8_t values[8] = {6, 2, 7, 4, 9, 6, 7, 1}; // Valores para calcular el promedio
 volatile uint8_t systick_habilitado = 1;       // Flag para controlar el estado del SysTick
 
 int main(void)
 {
     // Configuración inicial del sistema
     SystemInit();
     
     // Configuración de periféricos
     configGPIO();
     configEINT();
     configSysTick();
     
     while (1) 
     {
         __WFI();
     }
     
     return 0;
 }
 
 /**
  * Configuración del SysTick Timer
  * Cálculo del valor RELOAD:
  * - Frecuencia del core (cclk) = 62 MHz = 62,000,000 Hz
  * - Período deseado = 10 ms = 0.01 s
  * - Ticks necesarios = cclk * período = 62,000,000 * 0.01 = 620,000
  * - Valor RELOAD = Ticks - 1 = 620,000 - 1 = 619,999
  */
 void configSysTick(void)
 {   
     // Cargar el valor de recarga (24 bits máximo)
     SysTick->LOAD = 619999U; // agregamos U para que se tome como unsigned
     
     // Limpiar el valor actual del contador
     SysTick->VAL = 0;
     
     // Configurar y habilitar SysTick
     SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // Bit 2: Usar reloj del procesador (cclk)
                     SysTick_CTRL_TICKINT_Msk |    // Bit 1: Habilitar interrupción
                     SysTick_CTRL_ENABLE_Msk;      // Bit 0: Habilitar SysTick
     

     // IMPORTANTE: Prioridad 2 (menor que EINT1 que tiene prioridad 1)
     // Según el enunciado, SysTick debe tener MENOR prioridad que EINT 
     NVIC_SetPriority(SysTick_IRQn, 2);
 }
 
 /**
  * Configuración de GPIO
  * - P0[7:0] como salidas para mostrar el promedio
  */
 void configGPIO(void) 
 {
     // Configurar P0[7:0] como GPIO (función por defecto)
     LPC_PINCON->PINSEL0 &= ~0xFFFF;  // Limpiar bits [15:0] para P0[7:0]
     // Configurar P0[7:0] como salidas
     LPC_GPIO0->FIODIR |= 0xFF;  // Bits [7:0] como salidas
     
     // Inicializar puerto P0[7:0] en 0
     LPC_GPIO0->FIOCLR = 0xFF;
 }
 
 /**
  * Configuración de interrupción externa EINT1
  * - P2.11 como entrada para EINT1
  * - Detección por flanco ascendente
  * - Resistencia pull-down habilitada
  */
 void configEINT(void)
 {
     // Configurar P2.11 como EINT1
     // PINSEL4[23:22] = 01 para función EINT1
     LPC_PINCON->PINSEL4 &= ~(3 << 22);  // Limpiar bits [23:22]
     LPC_PINCON->PINSEL4 |= (1 << 22);   // Establecer función EINT1
     
     // Configurar resistencia pull-down en P2.11
     // PINMODE4[23:22] = 11 para pull-down
     LPC_PINCON->PINMODE4 &= ~(3 << 22); // Limpiar bits [23:22]
     LPC_PINCON->PINMODE4 |= (3 << 22);  // Establecer pull-down
     
     // Configurar EINT1 para detección por flanco
     LPC_SC->EXTMODE |= (1 << 1);   // EINT1 sensible a flanco
     
     // Configurar EINT1 para flanco ascendente
     LPC_SC->EXTPOLAR |= (1 << 1);  // EINT1 sensible a flanco ascendente
     
     // Limpiar flag de interrupción pendiente
     LPC_SC->EXTINT = (1 << 1);
     
     // Habilitar interrupción EINT1 en NVIC
     NVIC_EnableIRQ(EINT1_IRQn);
     
     // Configurar prioridad de EINT1
     // Prioridad 1 (mayor que SysTick que tiene prioridad 2)
     NVIC_SetPriority(EINT1_IRQn, 1);
 }
 
 /**
  * Manejador de interrupción EINT1
  * Se ejecuta en cada flanco ascendente en P2.11
  * Alterna entre habilitar y deshabilitar el SysTick
  */
 void EINT1_IRQHandler(void)
 {
     // Limpiar flag de interrupción EINT1
     LPC_SC->EXTINT = (1 << 1);
     
     // Alternar el estado del SysTick
     systick_habilitado = !systick_habilitado;
     
     if (systick_habilitado) 
     {
         // Habilitar interrupción del SysTick
         SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
     }
     else 
     {
         // Deshabilitar interrupción del SysTick
         SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
     }
 }
 
 /**
  * Manejador de interrupción SysTick
  * Se ejecuta cada 10ms cuando está habilitado
  * Muestra el promedio de values[] en P0[7:0]
  */
 void SysTick_Handler(void)
 {
     uint8_t promedio;
     
     // Calcular el promedio de los valores
     promedio = calcularPromedio(values);
     
     // Mostrar el promedio en el puerto P0[7:0]
     // Usar máscara para asegurar que solo se modifican los bits [7:0]
     LPC_GPIO0->FIOPIN = (LPC_GPIO0->FIOPIN & ~0xFF) | (promedio & 0xFF);
 }
 
 /**
  * Función para calcular el promedio de un array de 8 valores
  * @param values Array de 8 valores uint8_t
  * @return Promedio de los valores como uint8_t
  */
 uint8_t calcularPromedio(uint8_t values[8])
 {
     uint16_t suma = 0;
     uint8_t i;
     
     // Sumar todos los valores
     for (i = 0; i < 8; i++)
     {
         suma += values[i];
     }
     
     // Retornar el promedio (división entera)
     return (uint8_t)(suma / 8);
 }