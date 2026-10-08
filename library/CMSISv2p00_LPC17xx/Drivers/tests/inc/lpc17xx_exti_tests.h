#ifndef LPC17XX_EXTI_TESTS_H
#define LPC17XX_EXTI_TESTS_H

#ifdef UNIT_TESTING_ENABLED

#define EXTI_MASK 0xF

#include "lpc17xx_exti.h"
#include "test_asserts.h"
#include "test_utils.h"

void EXTI_RunTests(void);

#endif  // UNIT_TESTING_ENABLED
#endif  // LPC17XX_EXTI_TESTS_H
