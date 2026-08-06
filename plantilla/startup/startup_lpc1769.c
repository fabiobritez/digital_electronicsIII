/* ============================================================================
 * startup_lpc1769.c - Lo que corre ANTES de main()
 * ============================================================================
 *
 * Cuando el LPC1769 sale del reset, NO salta a main(). No sabe que existe.
 * Lo que hace el hardware es literalmente esto:
 *
 *   1. Lee 4 bytes de la direccion 0x00000000 -> los carga en el stack pointer
 *   2. Lee 4 bytes de la direccion 0x00000004 -> salta ahi
 *
 * Nada mas. Esas dos palabras son las dos primeras entradas de la tabla de
 * vectores que esta mas abajo, y el linker script se encarga de que la tabla
 * quede justo en 0x00000000.
 *
 * La direccion del paso 2 es Reset_Handler, y su trabajo es dejar la memoria
 * en el estado que el lenguaje C promete (globales inicializadas) antes de
 * llamar a main.
 *
 * Este archivo es equivalente al cr_startup_lpc175x_6x.c que genera MCUXpresso,
 * pero escrito para leerse.
 * ========================================================================= */

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Simbolos que define el linker script (lpc1769.ld)
 * ---------------------------------------------------------------------------
 * Ojo con esto, porque es la fuente de confusion clasica: lo que importa NO es
 * el valor de estas variables, sino su DIRECCION. El linker no crea variables,
 * solo pega etiquetas en posiciones de memoria. Por eso siempre se usan como
 * &_sdata, nunca como _sdata.
 * ------------------------------------------------------------------------ */
extern uint32_t _sidata;   /* .data guardado en FLASH (de donde copiar)  */
extern uint32_t _sdata;    /* .data en RAM (a donde copiar), inicio      */
extern uint32_t _edata;    /* .data en RAM, fin                          */
extern uint32_t _sbss;     /* .bss, inicio                               */
extern uint32_t _ebss;     /* .bss, fin                                  */
extern uint32_t _estack;   /* tope de la RAM = stack pointer inicial     */

extern int main(void);

/* Corre los constructores (__attribute__((constructor)) y, en C++, los
   objetos globales). La provee newlib. */
extern void __libc_init_array(void);

/* SystemInit() configura el clock (PLL a 100 MHz, flash wait states...).
   La version de verdad esta en system_LPC17xx.c de CMSIS. Si compilas sin
   CMSIS (USE_CMSIS=0), se usa la version vacia de mas abajo y el micro se
   queda corriendo con el oscilador interno de 4 MHz. Anda igual, pero todo
   es 25 veces mas lento. */
void SystemInit(void);
__attribute__((weak)) void SystemInit(void) { }

void Reset_Handler(void);
void Default_Handler(void);


/* ---------------------------------------------------------------------------
 * Los handlers
 * ---------------------------------------------------------------------------
 * Estan declarados "weak" y apuntando todos a Default_Handler. Weak significa:
 * "usa esta definicion, salvo que alguien mas defina una funcion con el mismo
 * nombre; en ese caso usa la de el".
 *
 * Por eso alcanza con que escribas en tu codigo
 *
 *     void TIMER0_IRQHandler(void) { ... }
 *
 * y sin configurar nada mas, el linker la mete sola en su lugar de la tabla.
 * Y por eso, si te equivocas en una letra del nombre, no hay ningun error de
 * compilacion: simplemente tu funcion nunca se llama y la interrupcion cae en
 * Default_Handler. Es el bug numero uno con interrupciones.
 * ------------------------------------------------------------------------ */
#define ALIAS_DEFAULT __attribute__((weak, alias("Default_Handler")))

/* Excepciones del nucleo Cortex-M3 (las tiene cualquier Cortex-M) */
void NMI_Handler(void)          ALIAS_DEFAULT;
void HardFault_Handler(void)    ALIAS_DEFAULT;
void MemManage_Handler(void)    ALIAS_DEFAULT;
void BusFault_Handler(void)     ALIAS_DEFAULT;
void UsageFault_Handler(void)   ALIAS_DEFAULT;
void SVC_Handler(void)          ALIAS_DEFAULT;
void DebugMon_Handler(void)     ALIAS_DEFAULT;
void PendSV_Handler(void)       ALIAS_DEFAULT;
void SysTick_Handler(void)      ALIAS_DEFAULT;

/* Interrupciones de los perifericos del LPC1769 (IRQ 0 a 34).
   El orden es el de la tabla 50 del UM10360, y coincide exactamente con el
   enum IRQn_Type de LPC17xx.h. */
void WDT_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ  0 */
void TIMER0_IRQHandler(void)      ALIAS_DEFAULT;  /* IRQ  1 */
void TIMER1_IRQHandler(void)      ALIAS_DEFAULT;  /* IRQ  2 */
void TIMER2_IRQHandler(void)      ALIAS_DEFAULT;  /* IRQ  3 */
void TIMER3_IRQHandler(void)      ALIAS_DEFAULT;  /* IRQ  4 */
void UART0_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ  5 */
void UART1_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ  6 */
void UART2_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ  7 */
void UART3_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ  8 */
void PWM1_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ  9 */
void I2C0_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 10 */
void I2C1_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 11 */
void I2C2_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 12 */
void SPI_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 13 */
void SSP0_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 14 */
void SSP1_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 15 */
void PLL0_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 16 */
void RTC_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 17 */
void EINT0_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ 18 */
void EINT1_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ 19 */
void EINT2_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ 20 */
void EINT3_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ 21 - tambien las int. de GPIO */
void ADC_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 22 */
void BOD_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 23 */
void USB_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 24 */
void CAN_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 25 */
void DMA_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 26 */
void I2S_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 27 */
void ENET_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 28 */
void RIT_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 29 */
void MCPWM_IRQHandler(void)       ALIAS_DEFAULT;  /* IRQ 30 */
void QEI_IRQHandler(void)         ALIAS_DEFAULT;  /* IRQ 31 */
void PLL1_IRQHandler(void)        ALIAS_DEFAULT;  /* IRQ 32 */
void USBActivity_IRQHandler(void) ALIAS_DEFAULT;  /* IRQ 33 */
void CANActivity_IRQHandler(void) ALIAS_DEFAULT;  /* IRQ 34 */


/* ---------------------------------------------------------------------------
 * LA TABLA DE VECTORES
 * ---------------------------------------------------------------------------
 * Un arreglo de punteros a funcion, y nada mas que eso. El atributo section()
 * lo manda a ".isr_vector", que el linker script pone primero de todo, o sea
 * en la direccion 0x00000000.
 *
 * "used" evita que el compilador lo elimine por no estar referenciado desde
 * ningun lado (nadie lo lee desde C: lo lee el hardware).
 *
 * El indice de cada entrada es su numero de excepcion. Los IRQ de perifericos
 * arrancan en el indice 16: por eso TIMER0 (IRQ 1) esta en la posicion 17.
 * ------------------------------------------------------------------------ */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) =
{
    /* --- Excepciones del nucleo Cortex-M3 --- */
    (void (*)(void)) &_estack,   /*  0  0x00  valor inicial del stack pointer  */
    Reset_Handler,               /*  1  0x04  aca salta el chip al resetear    */
    NMI_Handler,                 /*  2  0x08                                   */
    HardFault_Handler,           /*  3  0x0C  algo salio muy mal (modulo 12)   */
    MemManage_Handler,           /*  4  0x10                                   */
    BusFault_Handler,            /*  5  0x14                                   */
    UsageFault_Handler,          /*  6  0x18                                   */

    /*  7  0x1C  RESERVADO por ARM... pero NXP lo usa para el checksum!
                 La boot ROM suma las primeras 8 palabras de esta tabla y exige
                 que el resultado de 0. Si no da 0, considera que la FLASH esta
                 vacia o corrupta y se queda esperando en modo ISP, sin correr
                 tu programa.
                 Lo dejamos en 0 y lo completa la herramienta de grabado o
                 nuestro tools/lpc_checksum.py. Ver el README. */
    0,

    0,                           /*  8  0x20  reservado */
    0,                           /*  9  0x24  reservado */
    0,                           /* 10  0x28  reservado */
    SVC_Handler,                 /* 11  0x2C                                   */
    DebugMon_Handler,            /* 12  0x30                                   */
    0,                           /* 13  0x34  reservado */
    PendSV_Handler,              /* 14  0x38                                   */
    SysTick_Handler,             /* 15  0x3C  el tick del sistema (modulo 6)   */

    /* --- Interrupciones de los perifericos (IRQ 0 en adelante) --- */
    WDT_IRQHandler,              /* 16 */
    TIMER0_IRQHandler,           /* 17 */
    TIMER1_IRQHandler,           /* 18 */
    TIMER2_IRQHandler,           /* 19 */
    TIMER3_IRQHandler,           /* 20 */
    UART0_IRQHandler,            /* 21 */
    UART1_IRQHandler,            /* 22 */
    UART2_IRQHandler,            /* 23 */
    UART3_IRQHandler,            /* 24 */
    PWM1_IRQHandler,             /* 25 */
    I2C0_IRQHandler,             /* 26 */
    I2C1_IRQHandler,             /* 27 */
    I2C2_IRQHandler,             /* 28 */
    SPI_IRQHandler,              /* 29 */
    SSP0_IRQHandler,             /* 30 */
    SSP1_IRQHandler,             /* 31 */
    PLL0_IRQHandler,             /* 32 */
    RTC_IRQHandler,              /* 33 */
    EINT0_IRQHandler,            /* 34 */
    EINT1_IRQHandler,            /* 35 */
    EINT2_IRQHandler,            /* 36 */
    EINT3_IRQHandler,            /* 37 */
    ADC_IRQHandler,              /* 38 */
    BOD_IRQHandler,              /* 39 */
    USB_IRQHandler,              /* 40 */
    CAN_IRQHandler,              /* 41 */
    DMA_IRQHandler,              /* 42 */
    I2S_IRQHandler,              /* 43 */
    ENET_IRQHandler,             /* 44 */
    RIT_IRQHandler,              /* 45 */
    MCPWM_IRQHandler,            /* 46 */
    QEI_IRQHandler,              /* 47 */
    PLL1_IRQHandler,             /* 48 */
    USBActivity_IRQHandler,      /* 49 */
    CANActivity_IRQHandler,      /* 50 */
};


/* ---------------------------------------------------------------------------
 * Reset_Handler - la primera funcion de tu programa
 * ---------------------------------------------------------------------------
 * Prepara la memoria para que C funcione como C promete, y recien despues
 * llama a main. Los cuatro pasos, en orden:
 * ------------------------------------------------------------------------ */
void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* 1) Copiar .data de FLASH a RAM.
          Esto es lo que hace que "int contador = 42;" valga 42 en el primer
          ciclo de main. El valor estaba guardado en FLASH; aca se copia al
          lugar de la RAM donde la variable vive de verdad. */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2) Poner .bss en cero.
          Esto es lo que hace que "static int estado;" arranque en 0 sin que
          vos lo escribas. Si borraras este for, esas variables arrancarian
          con la basura que hubiera quedado en la RAM. */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3) Configurar el clock del sistema.
          Sin esto el micro corre a 4 MHz (oscilador interno). SystemInit()
          engancha el cristal de 12 MHz, configura la PLL y deja el core a
          100 MHz, ademas de ajustar los wait states de la FLASH. Ver el
          modulo 3 del curso. */
    SystemInit();

    /* 4) Correr los constructores (en C casi nunca hay; en C++ son los
          objetos globales). */
    __libc_init_array();

    /* Recien ahora empieza tu programa. */
    main();

    /* Si main() retorna no hay sistema operativo al que volver, asi que
       lo unico sensato es quedarse quieto. */
    while (1) {
    }
}


/* ---------------------------------------------------------------------------
 * Default_Handler - donde caen las interrupciones que no atendiste
 * ---------------------------------------------------------------------------
 * Un while(1) a proposito. Si tu programa "se cuelga" sin razon aparente,
 * pone un breakpoint aca: si cae, es que se disparo una interrupcion que
 * habilitaste pero cuyo handler no escribiste (o le erraste al nombre).
 *
 * Con el debugger conectado, mirando el registro IPSR sabes exactamente que
 * excepcion fue: IPSR = numero de la entrada de la tabla de arriba.
 * ------------------------------------------------------------------------ */
void Default_Handler(void)
{
    while (1) {
    }
}
