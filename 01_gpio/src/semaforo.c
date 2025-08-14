#include "LPC17xx.h"

// Definiciones para los pines del LED RGB
#define RED_LED_PORT     LPC_GPIO0
#define RED_LED_PIN      22

#define GREEN_LED_PORT   LPC_GPIO3
#define GREEN_LED_PIN    25

#define BLUE_LED_PORT    LPC_GPIO3
#define BLUE_LED_PIN     26

// Función para realizar un retardo (no precisa timers)
void delay_ms(unsigned int ms) {
    for (unsigned int i = 0; i < ms * 4000; i++) {
        __NOP();  // Instrucción que no hace nada 
    }
}

// Inicializa los tres pines como GPIO salida sin pull-up/pull-down
void init_RGB_LEDs(void) {
    // Selección de función GPIO
    LPC_PINCON->PINSEL1 &= ~(0x3 << 12); // P0.22
    LPC_PINCON->PINSEL7 &= ~(0x3 << 18); // P3.25
    LPC_PINCON->PINSEL7 &= ~(0x3 << 20); // P3.26

    // Desactivar resistencias internas
    LPC_PINCON->PINMODE1 |= (0x2 << 12);
    LPC_PINCON->PINMODE7 |= (0x2 << 18);
    LPC_PINCON->PINMODE7 |= (0x2 << 20);

    // Configurar como salida
    RED_LED_PORT->FIODIR   |= (1 << RED_LED_PIN);
    GREEN_LED_PORT->FIODIR |= (1 << GREEN_LED_PIN);
    BLUE_LED_PORT->FIODIR  |= (1 << BLUE_LED_PIN);
}

// Apaga todos los LEDs
void apagar_todos(void) {
    RED_LED_PORT->FIOSET   = (1 << RED_LED_PIN);
    GREEN_LED_PORT->FIOSET = (1 << GREEN_LED_PIN);
    BLUE_LED_PORT->FIOSET  = (1 << BLUE_LED_PIN);
}

// Enciende un LED específico
void encender_rojo(void) {
    apagar_todos();
    RED_LED_PORT->FIOCLR = (1 << RED_LED_PIN);
}

void encender_verde(void) {
    apagar_todos();
    GREEN_LED_PORT->FIOCLR = (1 << GREEN_LED_PIN);
}

void encender_azul(void) {
    apagar_todos();
    BLUE_LED_PORT->FIOCLR = (1 << BLUE_LED_PIN);
}

int main(void) {
    init_RGB_LEDs();  // Configuración inicial

    while (1) {

        encender_verde();
        delay_ms(20000);

        encender_azul(); // simula el amarillo
        delay_ms(1000);
 
        encender_rojo();
        delay_ms(20000);
    }
}
