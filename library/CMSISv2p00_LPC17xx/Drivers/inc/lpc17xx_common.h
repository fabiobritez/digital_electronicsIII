#ifndef LPC17XX_LPC17XX_COMMON_H
#define LPC17XX_LPC17XX_COMMON_H

/**
 * @brief Port selection for GPIO and PINSEL.
 */
typedef enum {
    PORT_0 = 0,
    PORT_1,
    PORT_2,
    PORT_3,
    PORT_4
} LPC_PORT;
/** Check port option parameter for GPIO and PINSEL. */
#define PARAM_LPC_PORT(PORT) ((PORT) >= PORT_0 && (PORT) <= PORT_4)

/**
 * @brief Pin selection for PINSEL (0-31).
 */
typedef enum {
    PIN_0 = 0,
    PIN_1,
    PIN_2,
    PIN_3,
    PIN_4,
    PIN_5,
    PIN_6,
    PIN_7,
    PIN_8,
    PIN_9,
    PIN_10,
    PIN_11,
    PIN_12,
    PIN_13,
    PIN_14,
    PIN_15,
    PIN_16,
    PIN_17,
    PIN_18,
    PIN_19,
    PIN_20,
    PIN_21,
    PIN_22,
    PIN_23,
    PIN_24,
    PIN_25,
    PIN_26,
    PIN_27,
    PIN_28,
    PIN_29,
    PIN_30,
    PIN_31
} LPC_PIN;
/** Check pin option parameter. */
#define PARAM_LPC_PIN(PIN) ((PIN) >= PIN_0 && (PIN) <= PIN_31)

#endif  //LPC17XX_LPC17XX_COMMON_H
