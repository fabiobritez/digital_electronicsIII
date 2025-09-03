/*
 * Sistema de Recepción de Datos con Modulación PWM
 * 
 * Descripción:
 * - Recibe una secuencia de 10 bits por P0.0
 * - Formato: 30ms bajo | 10 bits de 5ms c/u | 30ms bajo
 * - Convierte el valor recibido (0-1023) a PWM de 50ms en P0.2
 */

 #include "LPC17xx.h"

 // Estados del sistema
 #define STATE_IDLE      0      
 #define STATE_PRE_SYNC  1
 #define STATE_SYNC      2
 #define STATE_POST_SYNC 3
 #define STATE_ERROR     4
 
 // Variables globales para el sistema de recepción
 volatile uint32_t milliseconds = 0;
 volatile uint32_t syncStartTime = 0;
 volatile uint32_t syncTime = 0;
 volatile uint32_t receivedData = 0;
 volatile uint32_t bitCount = 0;
 volatile uint8_t cycleCount = 0;
 volatile uint8_t pinValue = 0;
 volatile uint8_t lastPinValue = 1;  // Asumimos inicio en alto
 volatile uint8_t currentState = STATE_IDLE;
 
 // Prototipos de funciones
 void config_GPIO(void);
 void generatePWM(uint32_t data);
 void delay_ms(uint32_t ms);
 void reset(void);
 
 int main(void) 
 {
     // Inicializar el sistema
     SystemInit();
     
     // Configurar GPIO
     config_GPIO();
     
     // Configurar SysTick para interrumpir cada 1ms
     SysTick_Config(SystemCoreClock / 1000);
     
     while (1) 
     {
         // Calcular tiempo transcurrido desde el inicio de la sincronización
         if (syncStartTime != 0) {
             syncTime = milliseconds - syncStartTime;
         }
         
         switch (currentState) 
         {
             case STATE_IDLE:
                 // Esperar detección de flanco descendente (manejado en ISR)
                 break;
                 
             case STATE_PRE_SYNC:
                 // Verificar que han pasado 30ms
                 if (syncTime >= 30) {
                     currentState = STATE_SYNC;
                     cycleCount = 0;
                     bitCount = 0;
                     receivedData = 0;
                 }
                 break;
                 
             case STATE_SYNC:
                 // Capturar 10 bits, cada uno de 5ms
                 if (syncTime >= 30 && syncTime < 80) {  // 30ms + 50ms (10 bits * 5ms)
                     // El muestreo del bit se hace en la ISR
                 } else if (syncTime >= 80) {
                     // Verificar que se recibieron los 10 bits
                     if (bitCount == 10) {
                         currentState = STATE_POST_SYNC;
                     } else {
                         currentState = STATE_ERROR;  // No se recibieron todos los bits
                     }
                 }
                 break;
                 
             case STATE_POST_SYNC:
                 // Verificar que han pasado los 30ms finales
                 if (syncTime >= 110) {  // 30ms + 50ms + 30ms
                     // Generar señal PWM con el dato recibido
                     generatePWM(receivedData);
                     reset();
                 }
                 break;
                 
             case STATE_ERROR:
                 // Error en la recepción, reiniciar
                 reset();
                 break;
         }
     }
     
     return 0;
 }
 
 /**
  * Handler de interrupción SysTick - Se ejecuta cada 1ms
  */
 void SysTick_Handler(void) 
 {
     milliseconds++;
     
     // Leer el valor actual del pin P0.0
     pinValue = (LPC_GPIO0->FIOPIN >> 0) & 1;
     
     switch (currentState) 
     {
         case STATE_IDLE:
             // Detectar flanco descendente (1 -> 0)
             if (lastPinValue == 1 && pinValue == 0) {
                 syncStartTime = milliseconds;
                 currentState = STATE_PRE_SYNC;
             }
             break;
             
         case STATE_PRE_SYNC:
             // Verificar que se mantiene en bajo durante los 30ms
             if (pinValue == 1) {
                 currentState = STATE_ERROR;  // Error: debe mantenerse en bajo
             }
             break;
             
         case STATE_SYNC:
             cycleCount++;
             
             // Muestrear en el centro del bit (3er ms de cada periodo de 5ms)
             // cycleCount va de 1 a 50 durante la fase SYNC
             if ((cycleCount - 1) % 5 == 2 && bitCount < 10) {  // Posiciones 3, 8, 13, 18, 23, 28, 33, 38, 43, 48
                 // Desplazar y agregar el nuevo bit
                 receivedData = (receivedData << 1) | pinValue;
                 bitCount++;
             }
             break;
             
         case STATE_POST_SYNC:
             // Verificar que se mantiene en bajo durante los 30ms finales
             if (pinValue == 1) {
                 currentState = STATE_ERROR;  // Error: debe mantenerse en bajo
             }
             break;
             
         case STATE_ERROR:
             // No hacer nada, esperar reset en el main
             break;
     }
     
     // Guardar el valor actual para detectar flancos
     lastPinValue = pinValue;
 }
 
 /**
  * Configuración de pines GPIO
  * P0.0: Entrada con pull-up para recepción de datos
  * P0.2: Salida para señal PWM
  */
 void config_GPIO(void)
 {
     // P0.0 como GPIO (entrada)
     LPC_PINCON->PINSEL0 &= ~(3 << 0);   // Bits [1:0] = 00 para GPIO
     
     // P0.2 como GPIO (salida)  
     LPC_PINCON->PINSEL0 &= ~(3 << 4);   // Bits [5:4] = 00 para GPIO
     
     // Configurar dirección: P0.0 entrada, P0.2 salida
     LPC_GPIO0->FIODIR &= ~(1 << 0);     // P0.0 como entrada
     LPC_GPIO0->FIODIR |= (1 << 2);      // P0.2 como salida
     
     // Configurar pull-up en P0.0 para estado definido
     LPC_PINCON->PINMODE0 &= ~(3 << 0);  // Limpiar bits [1:0]
     LPC_PINCON->PINMODE0 |= (0 << 0);   // 00 = pull-up habilitado
     
     // Inicializar P0.2 en bajo
     LPC_GPIO0->FIOCLR = (1 << 2);
 }
 
 /**
  * Genera una señal PWM de 50ms de duración total
  * El ancho del pulso es proporcional al valor de data (0-1023)
  * 
  * @param data Valor entre 0 y 1023
  */
 void generatePWM(uint32_t data) 
 {
     // Limitar el valor máximo
     if (data > 1023) {
         data = 1023;
     }
     
     // Calcular tiempos (regla de tres)
     // 0-1023 se mapea a 0-50ms
     uint32_t pulseWidth = (data * 50) / 1023;
     uint32_t offTime = 50 - pulseWidth;
     
     // Generar pulso alto
     if (pulseWidth > 0) {
         LPC_GPIO0->FIOSET = (1 << 2);   // P0.2 = 1
         delay_ms(pulseWidth);
     }
     
     // Generar pulso bajo
     if (offTime > 0) {
         LPC_GPIO0->FIOCLR = (1 << 2);   // P0.2 = 0
         delay_ms(offTime);
     }
 }
 
 /**
  * Función de retardo usando el contador de milisegundos del SysTick
  * 
  * @param ms Milisegundos a esperar
  */
 void delay_ms(uint32_t ms) 
 {
     uint32_t startTime = milliseconds;
     
     // Esperar hasta que transcurra el tiempo especificado
     while ((milliseconds - startTime) < ms) {
         // Espera activa
     }
 }
 
 /**
  * Reinicia todas las variables del sistema al estado inicial
  */
 void reset(void) 
 {
     syncStartTime = 0;
     syncTime = 0;
     receivedData = 0;
     bitCount = 0;
     cycleCount = 0;
     currentState = STATE_IDLE;
     // No reseteamos lastPinValue para mantener detección de flancos
 }
