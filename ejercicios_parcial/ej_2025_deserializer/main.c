/*
 * Sistema Deserializador Serial con Detección Continua
 * 
 * Descripción:
 * - Buffer deslizante de 16 bits para detección continua de patrones
 * - Distribución en tiempo real de bits pares/impares
 * - Detección inmediata de patrones especiales sin esperar frame completo
 * - Latencia mínima después de los primeros 16 bits
 */

 #include "LPC17xx.h"

 // Configuración del sistema
 #define SERIAL_INPUT_PIN    0   // P0.0 para entrada serial
 #define ODD_OUTPUT_PIN      1   // P0.1 para bits impares  
 #define EVEN_OUTPUT_PIN     2   // P0.2 para bits pares
 
 // Patrones especiales
 #define PATTERN_ZERO        0xF628  // Patrón para poner salidas en cero
 #define PATTERN_REPEAT      0x28F6  // Patrón para repetir último dato
 
 // Configuración de timing
 #define INPUT_PERIOD_MS     10   // Período T de entrada en ms
 
 // Estados del sistema
 typedef enum {
     MODE_NORMAL = 0,
     MODE_ZERO,
     MODE_REPEAT,
     MODE_STARTUP    // Estado inicial hasta recibir 16 bits
 } SystemMode;
 
 // Estructura para el buffer deslizante
 typedef struct {
     uint16_t data;           // Buffer de 16 bits
     uint8_t bitCount;        // Bits totales recibidos (para startup)
     uint8_t isReady;         // 1 cuando ya tenemos 16 bits
 } SlidingBuffer;
 
 // Estructura para gestión de repetición
 typedef struct {
     uint16_t pattern;        // Patrón a repetir
     uint8_t position;        // Posición actual en el patrón
     uint8_t active;          // 1 si está activo
 } RepeatManager;
 
 // Variables globales
 volatile uint32_t milliseconds = 0;
 volatile uint32_t lastInputSample = 0;
 
 // Buffer deslizante
 volatile SlidingBuffer slidingBuffer = {0, 0, 0};
 
 // Gestión de modos
 volatile SystemMode currentMode = MODE_STARTUP;
 volatile SystemMode previousMode = MODE_STARTUP;
 
 // Gestión de repetición
 volatile RepeatManager repeatManager = {0, 0, 0};
 
 // Salidas actuales
 volatile uint8_t currentInputBit = 0;
 volatile uint8_t globalBitIndex = 0;     // Índice global del bit (0=par, 1=impar)
 
 // Último patrón válido para repetición
 volatile uint16_t lastValidPattern = 0;
 volatile uint8_t hasValidPattern = 0;
  
 void config_GPIO(void);
 void config_SysTick(void);
 void processBitInput(uint8_t bit);
 void updateSlidingBuffer(uint8_t bit);
 void checkPatterns(void);
 void outputBitByMode(uint8_t bit);
 void outputRepeatBit(void);
 void setOutputs(uint8_t evenBit, uint8_t oddBit);
 
 int main(void) 
 {
     // Inicialización del sistema
     SystemInit();
     
     // Configurar periféricos
     config_GPIO();
     config_SysTick(); 

     while (1) {
         __WFI();  // Wait for interrupt 
     }
     
     return 0;
 }
 
 /**
  * Handler de interrupción SysTick - Se ejecuta cada 1ms
  */
 void SysTick_Handler(void) 
 {
     milliseconds++;
     
     // Muestrear entrada cada INPUT_PERIOD_MS (período T)
     if ((milliseconds - lastInputSample) >= INPUT_PERIOD_MS) 
     {
         lastInputSample = milliseconds;
         
         // Leer bit de entrada
         currentInputBit = (LPC_GPIO0->FIOPIN >> SERIAL_INPUT_PIN) & 1;
         
         // Procesar el bit inmediatamente
         processBitInput(currentInputBit);
     }
 }
 
 /**
  * Procesa cada bit de entrada con detección continua
  */
 void processBitInput(uint8_t bit)
 {
     // Actualizar buffer deslizante
     updateSlidingBuffer(bit);
     
     // Si el buffer está listo (después de 16 bits iniciales)
     if (slidingBuffer.isReady) 
     {
         // Verificar patrones especiales en cada bit nuevo
         checkPatterns();
         
         // Procesar salida según el modo actual
         switch (currentMode) 
         {
             case MODE_NORMAL:
                 outputBitByMode(bit);
                 break;
                 
             case MODE_ZERO:
                 setOutputs(0, 0);
                 break;
                 
             case MODE_REPEAT:
                 outputRepeatBit();
                 break;
                 
             default:
                 break;
         }
     }
     
     // Incrementar índice global de bit
     globalBitIndex = (globalBitIndex + 1) & 1;  // Alterna entre 0 y 1
 }
 
 /**
  * Actualiza el buffer deslizante con el nuevo bit
  */
 void updateSlidingBuffer(uint8_t bit)
 {
     // Desplazar buffer a la izquierda y agregar nuevo bit
     slidingBuffer.data = (slidingBuffer.data << 1) | bit;
     
     // Durante startup, contar bits hasta tener 16
     if (!slidingBuffer.isReady) 
     {
         slidingBuffer.bitCount++;
         
         if (slidingBuffer.bitCount >= 16) 
         {
             slidingBuffer.isReady = 1;
             currentMode = MODE_NORMAL;
         }
     }
 }
 
 /**
  * Verifica si el buffer actual contiene un patrón especial
  */
 void checkPatterns(void)
 {
     uint16_t currentPattern = slidingBuffer.data;
     
     if (currentPattern == PATTERN_ZERO) 
     {
         // Cambiar a modo cero inmediatamente
         if (currentMode != MODE_ZERO) 
         {
             previousMode = currentMode;
             currentMode = MODE_ZERO;
         }
     }
     else if (currentPattern == PATTERN_REPEAT) 
     {
         // Cambiar a modo repetición si hay un patrón válido guardado
         if (hasValidPattern && currentMode != MODE_REPEAT) 
         {
             previousMode = currentMode;
             currentMode = MODE_REPEAT;
             
             // Inicializar el gestor de repetición
             repeatManager.pattern = lastValidPattern;
             repeatManager.position = 0;
             repeatManager.active = 1;
         }
     }
     else 
     {
         // Patrón normal - guardar como último válido
         if (currentMode == MODE_NORMAL) 
         {
             // Solo actualizar el patrón válido si no estamos en modo especial
             lastValidPattern = currentPattern;
             hasValidPattern = 1;
         }
         else if (currentMode == MODE_ZERO || currentMode == MODE_REPEAT) 
         {
             // Salir del modo especial al detectar un patrón diferente
             currentMode = MODE_NORMAL;
             repeatManager.active = 0;
             
             // Guardar el nuevo patrón como válido
             lastValidPattern = currentPattern;
             hasValidPattern = 1;
         }
     }
 }
 
 /**
  * Envía el bit actual a la salida correspondiente según su índice
  */
 void outputBitByMode(uint8_t bit)
 {
     if (globalBitIndex == 0) 
     {
         // Bit con índice par - actualizar P0.2
         if (bit) {
             LPC_GPIO0->FIOSET = (1 << EVEN_OUTPUT_PIN);
         } else {
             LPC_GPIO0->FIOCLR = (1 << EVEN_OUTPUT_PIN);
         }
     }
     else 
     {
         // Bit con índice impar - actualizar P0.1
         if (bit) {
             LPC_GPIO0->FIOSET = (1 << ODD_OUTPUT_PIN);
         } else {
             LPC_GPIO0->FIOCLR = (1 << ODD_OUTPUT_PIN);
         }
     }
 }
 
 /**
  * Envía el siguiente bit del patrón en repetición
  */
 void outputRepeatBit(void)
 {
     if (!repeatManager.active) return;
     
     // Extraer el bit actual del patrón
     uint8_t bitToSend = (repeatManager.pattern >> (15 - repeatManager.position)) & 1;
     
     // Enviar a la salida correspondiente
     outputBitByMode(bitToSend);
     
     // Avanzar posición en el patrón
     repeatManager.position = (repeatManager.position + 1) & 0x0F;  // Módulo 16
 }
 
 /**
  * Establece directamente los valores de las salidas
  */
 void setOutputs(uint8_t evenBit, uint8_t oddBit)
 {
     // Actualizar salida par (P0.2)
     if (evenBit) {
         LPC_GPIO0->FIOSET = (1 << EVEN_OUTPUT_PIN);
     } else {
         LPC_GPIO0->FIOCLR = (1 << EVEN_OUTPUT_PIN);
     }
     
     // Actualizar salida impar (P0.1)
     if (oddBit) {
         LPC_GPIO0->FIOSET = (1 << ODD_OUTPUT_PIN);
     } else {
         LPC_GPIO0->FIOCLR = (1 << ODD_OUTPUT_PIN);
     }
 }
 
 /**
  * Configuración de pines GPIO
  */
 void config_GPIO(void)
 {
     // Configurar pines como GPIO
     LPC_PINCON->PINSEL0 &= ~((3 << (SERIAL_INPUT_PIN * 2)) |
                              (3 << (ODD_OUTPUT_PIN * 2)) |
                              (3 << (EVEN_OUTPUT_PIN * 2)));
     
     // Configurar direcciones
     LPC_GPIO0->FIODIR &= ~(1 << SERIAL_INPUT_PIN);  // P0.0 entrada
     LPC_GPIO0->FIODIR |= (1 << ODD_OUTPUT_PIN);     // P0.1 salida
     LPC_GPIO0->FIODIR |= (1 << EVEN_OUTPUT_PIN);    // P0.2 salida
     
     // Pull-up en entrada
     LPC_PINCON->PINMODE0 &= ~(3 << (SERIAL_INPUT_PIN * 2));
     
     // Inicializar salidas en bajo
     LPC_GPIO0->FIOCLR = (1 << ODD_OUTPUT_PIN) | (1 << EVEN_OUTPUT_PIN);
 }
 
 /**
  * Configuración del SysTick Timer
  */
 void config_SysTick(void)
 {
     // Configurar para interrumpir cada 1ms
     SysTick_Config(SystemCoreClock / 1000);
 }