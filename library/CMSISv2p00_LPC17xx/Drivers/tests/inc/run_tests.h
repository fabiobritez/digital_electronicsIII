#ifndef RUN_TESTS_H
#define RUN_TESTS_H

#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_adc_tests.h"
#include "lpc17xx_dac_tests.h"
#include "lpc17xx_exti_tests.h"
#include "lpc17xx_gpdma_tests.h"
#include "lpc17xx_gpio_tests.h"
#include "lpc17xx_pinsel_tests.h"
#include "lpc17xx_pwm_tests.h"
#include "lpc17xx_systick_tests.h"
#include "lpc17xx_timer_tests.h"
#include "lpc17xx_uart_tests.h"

void run_all_tests(void);

#endif  // UNIT_TESTING_ENABLED
#endif  // RUN_TESTS_H
