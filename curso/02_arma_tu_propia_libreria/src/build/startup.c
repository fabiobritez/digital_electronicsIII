#include <stdint.h>
/* Símbolos del linker */
extern uint32_t _etext, _sdata, _edata, _sbss, _ebss, _estack;
int main(void);
void Reset_Handler(void);
void Default_Handler(void) { while (1) {} }
#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))
void NMI_Handler(void)        WEAK_ALIAS;
void HardFault_Handler(void)  WEAK_ALIAS;
void MemManage_Handler(void)  WEAK_ALIAS;
void BusFault_Handler(void)   WEAK_ALIAS;
void UsageFault_Handler(void) WEAK_ALIAS;
void SVC_Handler(void)        WEAK_ALIAS;
void DebugMon_Handler(void)   WEAK_ALIAS;
void PendSV_Handler(void)     WEAK_ALIAS;
void SysTick_Handler(void)    WEAK_ALIAS;

/* Tabla de vectores: SP inicial + Reset + excepciones del núcleo */
__attribute__((section(".isr_vector"), used))
void (* const g_vectors[])(void) = {
    (void (*)(void)) &_estack,   /* 0x00: stack pointer inicial */
    Reset_Handler,               /* 0x04: reset */
    NMI_Handler, HardFault_Handler, MemManage_Handler, BusFault_Handler,
    UsageFault_Handler, 0, 0, 0, 0, SVC_Handler, DebugMon_Handler, 0,
    PendSV_Handler, SysTick_Handler,
    /* (los IRQ de periféricos irían a continuación; mygpio no los usa) */
};

void Reset_Handler(void) {
    uint32_t *src = &_etext, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;   /* copiar .data a RAM */
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;  /* poner .bss en 0 */
    main();
    while (1) {}
}
