#ifdef UNIT_TESTING_ENABLED

#include "run_tests.h"

void run_all_tests(void) {
    PINSEL_RunTests();
    GPIO_RunTests();
    SYSTICK_RunTests();
    EXTI_RunTests();
    TIMER_RunTests();
    PWM_RunTests();
    ADC_RunTests();
    DAC_RunTests();
    GPDMA_RunTests();
    UART_RunTests();
}

#endif  // UNIT_TESTING_ENABLED
