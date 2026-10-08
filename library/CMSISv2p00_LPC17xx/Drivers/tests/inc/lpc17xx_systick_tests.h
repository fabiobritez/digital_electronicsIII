#ifndef LPC17XX_SYSTICK_TESTS_H
#define LPC17XX_SYSTICK_TESTS_H

#ifdef UNIT_TESTING_ENABLED

#include "lpc17xx_systick.h"
#include "test_asserts.h"
#include "test_utils.h"

void SYSTICK_RunTests(void);

#endif  // UNIT_TESTING_ENABLED
#endif  // LPC17XX_SYSTICK_TESTS_H
