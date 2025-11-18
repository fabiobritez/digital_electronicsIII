#include "LPC17xx.h"

/*
 * Realizar un programa que guarde mediante interrupción el dato presente en los pines de
 * entrada P2.0 al P2.7 cada vez que cambie su valor. Cada dato nuevo debe guardarse de forma
 * consecutiva en una región de memoria que permita tener disponible siempre los últimos 16
 * datos guardados
 *
 * Implementación con debounce usando SysTick
 */

// Buffer circular para almacenar los últimos 16 datos
volatile uint8_t data[16];
volatile uint8_t index_write = 0;  // Índice para escritura circular

// Variables para debounce
volatile uint8_t gpio_interrupt_flag = 0;  // Flag que indica interrupción GPIO pendiente
volatile uint8_t debounce_counter = 0;     // Contador para tiempo de debounce
volatile uint8_t last_stable_value = 0;    // Último valor estable leído
volatile uint8_t current_reading = 0;      // Lectura actual para comparación

#define DEBOUNCE_TIME_MS    50  // Tiempo de debounce en milisegundos
#define SYSTICK_FREQ_HZ     1000 // Frecuencia del SysTick (1ms por tick)


void config_GPIO(void);
void config_SysTick(void);
void EINT3_IRQHandler(void);
void SysTick_Handler(void);
void save_data(uint8_t new_data);

int main(void)
{
    // Inicializar el buffer
    for(int i = 0; i < 16; i++) {
        data[i] = 0;
    }


    config_GPIO();
    config_SysTick();

    // Leer valor inicial del puerto
    last_stable_value = LPC_GPIO2->FIOPIN & 0xFF;

    while(1)
    {
    	__WFI();
    }

    return 0;
}

void config_GPIO(void)
{
    // Configurar P2.0 a P2.7 como GPIO
    LPC_PINCON->PINSEL4 &= ~(0xFFFF << 0);

    // Configurar P2.0 a P2.7 como entrada
    LPC_GPIO2->FIODIR &= ~(0xFF << 0);

    // Configurar resistencias pull-up para evitar estados flotantes
    LPC_PINCON->PINMODE4 &= ~(0xFFFF << 0);    // Pull-up P2.0 to P2.7

    // Habilitar interrupciones por flanco de bajada en P2.0 a P2.7
    LPC_GPIOINT->IO2IntEnF |= (0xFF << 0);

    // Habilitar interrupciones por flanco de subida en P2.0 a P2.7
    LPC_GPIOINT->IO2IntEnR |= (0xFF << 0);

    // Limpiar cualquier flag de interrupción pendiente
    LPC_GPIOINT->IO2IntClr = 0xFF;

    // Habilitar interrupciones GPIO en el NVIC
    NVIC_EnableIRQ(EINT3_IRQn);
}

void config_SysTick(void)
{
    // Configurar SysTick para generar interrupción cada 1ms
    // Asumiendo CCLK = 100MHz
    // Configurar el valor de recarga para 1ms
    SysTick->LOAD = (SystemCoreClock / SYSTICK_FREQ_HZ) - 1;

    // Limpiar el valor actual del contador
    SysTick->VAL = 0;

    // Configurar SysTick: habilitar, usar reloj del procesador, habilitar interrupción
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // Usar reloj del procesador
                    SysTick_CTRL_TICKINT_Msk |     // Habilitar interrupción
                    SysTick_CTRL_ENABLE_Msk;       // Habilitar SysTick
}

// Handler de interrupciones GPIO
void EINT3_IRQHandler(void)
{
    // Verificar si la interrupción proviene del puerto 2
    if((LPC_GPIOINT->IO2IntStatF | LPC_GPIOINT->IO2IntStatR) & 0xFF)
    {
        // Solo activar el proceso de debounce si no está ya activo
        if(gpio_interrupt_flag == 0)
        {
            gpio_interrupt_flag = 1;       // Activar flag de debounce
            debounce_counter = 0;          // Reiniciar contador
            current_reading = LPC_GPIO2->FIOPIN & 0xFF;  // Capturar lectura inicial
        }

        // Limpiar las flags de interrupción del puerto 2
        LPC_GPIOINT->IO2IntClr = 0xFF;
    }
}

// ejecuta cada 1ms
void SysTick_Handler(void)
{
    // Procesar debounce si hay una interrupción GPIO pendiente
    if(gpio_interrupt_flag)
    {
        uint8_t new_reading = LPC_GPIO2->FIOPIN & 0xFF;

        // Verificar si la lectura es estable
        if(new_reading == current_reading)
        {
            // La lectura es consistente, incrementar contador
            debounce_counter++;

            // Si ha sido estable durante el tiempo de debounce (50ms)
            if(debounce_counter >= DEBOUNCE_TIME_MS)
            {
                // Verificamos si el valor realmente cambió de la ultima lectura estable
                if(new_reading != last_stable_value)
                {
                    // Guardar el nuevo valor estable
                    save_data(new_reading);
                    last_stable_value = new_reading;
                }

                // Resetear el proceso de debounce
                gpio_interrupt_flag = 0;
                debounce_counter = 0;
            }
        }
        else
        {
            // La lectura cambió, reiniciar el proceso de debounce
            current_reading = new_reading;
            debounce_counter = 0;
        }
    }
}

// Función para guardar datos en el buffer circular
void save_data(uint8_t new_data)
{
    // Opción 1: Buffer circular (más eficiente)
	data[index_write] = new_data;
    index_write = (index_write + 1) & 0x0F;


    /* Opción 2: Desplazamiento de datos (menos eficiente pero mantiene orden temporal)

    for(int i = 15; i > 0; i--)
    {
        data[i] = data[i-1];
    }
    // Guardar el nuevo dato siempre en la posición 0
    data[0] = new_data;
    */
}

