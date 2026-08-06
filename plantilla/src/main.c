/* ============================================================================
 * main.c - Blink del LED de la placa, escribiendo los registros a mano
 * ============================================================================
 *
 * Sin CMSIS, sin drivers, sin nada: punteros a direcciones de memoria. Es el
 * "hola mundo" de los sistemas embebidos y sirve para verificar que toda la
 * cadena funciona (compilador -> linker -> grabador -> placa).
 *
 * Placa: LPCXpresso LPC1769 (OM13085). El LED de a bordo esta en P0.22 y
 * enciende con el pin en ALTO.
 *
 * Si tu placa tiene el LED en otro pin, cambia LED_PIN de mas abajo.
 * ========================================================================= */

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Los registros que vamos a tocar
 * ---------------------------------------------------------------------------
 * Un registro de periferico es una direccion de memoria fija cableada al
 * hardware. Para usarlo desde C se castea esa direccion a puntero y se
 * desreferencia.
 *
 * El "volatile" NO es opcional: le prohibe al compilador optimizar los
 * accesos. Sin el, el compilador ve que escribis dos veces la misma variable
 * sin leerla y borra una de las escrituras, o cachea una lectura en un
 * registro del CPU y nunca vuelve a mirar la memoria. Con hardware del otro
 * lado, las dos cosas rompen el programa. (Modulo 0, capitulo 8.)
 *
 * Las direcciones salen del capitulo 2 del UM10360 y coinciden con LPC17xx.h.
 * ------------------------------------------------------------------------ */
#define REG(dir)        (*(volatile uint32_t *)(dir))

/* Pin Connect Block: elige que funcion cumple cada pin (GPIO, UART, ADC...) */
#define PINSEL1         REG(0x4002C004)   /* controla P0.16 a P0.31 */

/* GPIO puerto 0, en su version rapida (Fast IO), sobre el bus AHB */
#define FIO0DIR         REG(0x2009C000)   /* 1 = salida, 0 = entrada */
#define FIO0PIN         REG(0x2009C014)   /* estado actual de los pines */
#define FIO0SET         REG(0x2009C018)   /* escribir 1 -> pone el pin en ALTO */
#define FIO0CLR         REG(0x2009C01C)   /* escribir 1 -> pone el pin en BAJO */

#define LED_PIN         22u
#define LED_MASK        (1u << LED_PIN)


/* ---------------------------------------------------------------------------
 * Un delay cualquiera, a puro quemar ciclos
 * ---------------------------------------------------------------------------
 * Bloqueante y sin precision: es lo que NO hay que hacer en un programa de
 * verdad (modulo 17), pero para el primer arranque alcanza y no depende de
 * ningun periferico. Cuando llegues al modulo 6 esto se reemplaza por SysTick.
 *
 * El volatile en el contador evita que el compilador borre el lazo entero por
 * considerarlo inutil, que es exactamente lo que haria con -O2.
 * ------------------------------------------------------------------------ */
static void delay_lazos(volatile uint32_t lazos)
{
    while (lazos--) {
        __asm__ volatile ("nop");
    }
}


int main(void)
{
    /* 1) Que P0.22 sea GPIO y no otra cosa.
     *
     *    En PINSEL1 cada pin ocupa 2 bits. Como PINSEL1 arranca en P0.16,
     *    a P0.22 le tocan los bits (22-16)*2 = 12 y 13. El valor 00 es GPIO.
     *
     *    Despues del reset PINSEL1 vale 0, asi que en rigor esta linea es
     *    redundante. Se deja igual porque depender de los valores por defecto
     *    es una mala costumbre: el dia que otro codigo toco ese registro
     *    antes que vos, el bug es imposible de encontrar. */
    PINSEL1 &= ~(3u << 12);

    /* 2) Configurar el pin como SALIDA. */
    FIO0DIR |= LED_MASK;

    /* 3) Parpadear.
     *
     *    Notar que no se usa FIO0PIN para cambiar el pin, sino FIO0SET y
     *    FIO0CLR. La diferencia importa: FIO0PIN obliga a leer-modificar-
     *    escribir los 32 bits del puerto, y si justo en el medio salta una
     *    interrupcion que toca otro pin del mismo puerto, se pierde ese
     *    cambio. FIO0SET y FIO0CLR escriben solo los bits que marcas, en una
     *    sola operacion, sin tocar el resto. (Modulo 5.) */
    while (1) {
        FIO0SET = LED_MASK;        /* LED encendido */
        delay_lazos(150000u);

        FIO0CLR = LED_MASK;        /* LED apagado */
        delay_lazos(150000u);
    }

    /* Nunca se llega aca. Si main() retornara, el Reset_Handler queda en un
       while(1) esperando (ver startup_lpc1769.c). */
}


/* ============================================================================
 * NOTA SOBRE LA VELOCIDAD DEL PARPADEO
 * ============================================================================
 * Los 150000 lazos estan elegidos para que se vea bien con el micro corriendo
 * a 4 MHz, que es como arranca por defecto (oscilador RC interno).
 *
 * Si compilas con  make USE_CMSIS=1  entra en juego el SystemInit() de CMSIS,
 * que engancha el cristal de 12 MHz y configura la PLL para dejar el core a
 * 100 MHz. El mismo binario va a parpadear unas 25 veces mas rapido, tanto que
 * casi no se distingue.
 *
 * No es un error: es la demostracion mas barata de por que los delays por
 * conteo de lazos no sirven, y de que el clock del sistema es algo que se
 * configura. Los dos temas se ven en los modulos 3 (clock y PLL) y 6 (SysTick).
 * ========================================================================= */
