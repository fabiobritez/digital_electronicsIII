#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#ifdef UNIT_TESTING_ENABLED

//==========================================================================
// Defines for available pins in each port (double register per port).
//==========================================================================
#define PORT_0_MASK_LOW  ((0xC0FFFFFF))
#define PORT_0_MASK_HIGH ((0x3FFFFFFF))
#define PORT_1_MASK_LOW  ((0xF03F030F))
#define PORT_1_MASK_HIGH ((0xFFFFFFFF))
#define PORT_2_MASK_LOW  ((0x0FFFFFFF))
#define PORT_3_MASK_HIGH ((0x003C0000))
#define PORT_4_MASK_HIGH ((0x0F000000))

//==========================================================================
// Defines for available pins in each port (single register per port).
//==========================================================================
#define PORT_0_MASK ((0x7FFF8FFF))
#define PORT_1_MASK ((0xFFFFC713))
#define PORT_2_MASK ((0x00003FFF))
#define PORT_3_MASK ((0x06000000))
#define PORT_4_MASK ((0x30000000))

#define PORT_0_HW_MASK_LOW  ((0x8FFF))
#define PORT_0_HW_MASK_HIGH ((0x7FFF))
#define PORT_1_HW_MASK_LOW  ((0xC713))
#define PORT_1_HW_MASK_HIGH ((0xFFFF))
#define PORT_2_HW_MASK_LOW  ((0x3FFF))
#define PORT_3_HW_MASK_HIGH ((0x0600))
#define PORT_4_HW_MASK_HIGH ((0x3000))

#define PORT_0_B0_MASK ((0xFF))
#define PORT_0_B1_MASK ((0x8F))
#define PORT_0_B2_MASK ((0xFF))
#define PORT_0_B3_MASK ((0x7F))
#define PORT_1_B0_MASK ((0x13))
#define PORT_1_B1_MASK ((0xC7))
#define PORT_1_B2_MASK ((0xFF))
#define PORT_1_B3_MASK ((0xFF))
#define PORT_2_B0_MASK ((0xFF))
#define PORT_2_B1_MASK ((0x3F))
#define PORT_3_B3_MASK ((0x06))
#define PORT_4_B3_MASK ((0x30))

#define RUN_TESTS_INIT() \
    uint8_t total  = 0;  \
    uint8_t passed = 0
#define RUN_TEST(fn) \
    total++;         \
    passed += fn();
#define RUN_TESTS_END(name) printf("Passed %d/%d %s tests\n\n", passed, total, name)

#define PROPAGATION_DELAY()                                  \
    do {                                                     \
        for (volatile unsigned int _d = 0; _d < 50; ++_d) {} \
    } while (0)

#define EDGE_INT_P0_LOW(n)                           \
    do {                                             \
        LPC_PINCON->PINMODE0 |= (0x3 << ((n) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
        LPC_PINCON->PINMODE0 &= ~(0x3 << ((n) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
    } while (0)
#define EDGE_INT_P0_HIGH(n)                                 \
    do {                                                    \
        LPC_PINCON->PINMODE1 |= (0x3 << (((n) - 16) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
        LPC_PINCON->PINMODE1 &= ~(0x3 << (((n) - 16) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
    } while (0)
#define EDGE_INT_P1_LOW(n)                           \
    do {                                             \
        LPC_PINCON->PINMODE2 |= (0x3 << ((n) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
        LPC_PINCON->PINMODE2 &= ~(0x3 << ((n) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
    } while (0)
#define EDGE_INT_P1_HIGH(n)                                 \
    do {                                                    \
        LPC_PINCON->PINMODE3 |= (0x3 << (((n) - 16) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
        LPC_PINCON->PINMODE3 &= ~(0x3 << (((n) - 16) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
    } while (0)
#define EDGE_INT_P2_LOW(n)                           \
    do {                                             \
        LPC_PINCON->PINMODE4 |= (0x3 << ((n) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
        LPC_PINCON->PINMODE4 &= ~(0x3 << ((n) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}  \
    } while (0)
#define EDGE_INT_P2_HIGH(n)                                 \
    do {                                                    \
        LPC_PINCON->PINMODE5 |= (0x3 << (((n) - 16) * 2));  \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
        LPC_PINCON->PINMODE5 &= ~(0x3 << (((n) - 16) * 2)); \
        for (volatile int _d = 0; _d < 50; ++_d) {}         \
    } while (0)

#endif  // UNIT_TESTING_ENABLED
#endif  // TEST_UTILS_H
