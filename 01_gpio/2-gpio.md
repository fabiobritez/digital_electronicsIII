# **GPIO**
Los **GPIO** (*General Purpose Input/Output*) permiten usar los pines del microcontrolador como entradas o salidas digitales para interactuar con el mundo exterior: sensores, LEDs, botones, relés, etc.

    

### La configuración de los GPIO requiere considerar:

1. **Power (energía)**:
   El bloque GPIO está **siempre alimentado**, no es necesario activarlo manualmente.

2. **Selección de pines**:
   Los GPIO comparten pines con funciones alternativas (como UART, I2C, etc.).
   Se seleccionan con los registros **PINSELx** (ver sección correspondiente).

3. **Wake-up**:
   Los puertos **GPIO0 y GPIO2** pueden configurarse como fuente de **wake-up** (despertar) desde modos de bajo consumo (ver Sección 4.8.8 del manual).

4. **Interrupciones**:
   Los puertos **GPIO0 y GPIO2** pueden generar interrupciones por flanco ascendente, descendente o ambos.
   Las interrupciones se activan en los registros **IOxIntEnR/F**, y también deben habilitarse en el **NVIC** mediante el registro de habilitación global (ver `Table 117` del manual).

---

### **Características destacadas**

* GPIO acelerado por hardware:

  * Registros ubicados en un **bus AHB** para acceso rápido.
  * **Registros de máscara (FIOMASK)** para modificar solo ciertos bits sin afectar el resto.
  * Accesibles por byte, half-word (16 bits) o word (32 bits).
  * Posibilidad de escribir un valor completo de puerto en una sola instrucción.

* Compatible con DMA:

  * Los registros pueden ser accedidos por el controlador **GPDMA**, permitiendo enviar o recibir datos sincronizados con eventos de GPIO sin intervención del CPU.

* **Bit-banding (ARM Cortex-M3)**:

  * Permite acceso directo a bits individuales como si fueran variables.

* Todos los pines **inician como entradas con pull-up activado** después del reset.

---
 




## **¿Qué registros controlan los GPIOs?**

Los GPIO se configuran y controlan usando los siguientes registros:

| Registro   | Función principal                                          |
| ---------- | ---------------------------------------------------------- |
| `FIODIR`   | Dirección: define si cada pin es entrada (0) o salida (1)  |
| `FIOPIN`   | Estado actual de los pines                                 |
| `FIOSET`   | Pone a nivel alto (1) los pines de salida indicados        |
| `FIOCLR`   | Pone a nivel bajo (0) los pines de salida indicados        |
| `FIOMASK`  | Permite enmascarar bits para lectura/escritura parcial     |
| `FIOxINTx` | Registros para manejo de interrupciones por flanco o nivel |

---

## **3. Organización de puertos**

La LPC1769 ofrece hasta **5 puertos GPIO**:

* **P0, P1, P2, P3, P4**
* Cada puerto puede tener hasta 32 pines (`P0.0` a `P0.31`, etc.)
* No todos los pines están disponibles físicamente en todos los encapsulados.

### Tabla resumen:

| Puerto | Pines disponibles |
| ------ | ----------------- |
| P0     | P0.0 a P0.30      |
| P1     | P1.0 a P1.31      |
| P2     | P2.0 a P2.13      |
| P3     | P3.25 a P3.26     |
| P4     | P4.28 a P4.29     |

---

## **4. Acceso a registros mediante CMSIS**

CMSIS define punteros a estructuras que representan cada bloque de GPIO:

```c
#define LPC_GPIO0  ((LPC_GPIO_TypeDef *) 0x2009C000)
#define LPC_GPIO1  ((LPC_GPIO_TypeDef *) 0x2009C020)
...
```

Así se puede acceder a los registros con notación clara:

```c
LPC_GPIO0->FIODIR     // Dirección de los pines del puerto 0
LPC_GPIO1->FIOSET     // Set de salida del puerto 1
LPC_GPIO2->FIOPIN     // Lectura de pines del puerto 2
```

---

## **5. FIODIR – Dirección del pin**

Este registro define si cada pin es entrada o salida:

| Bit | Valor | Significado                  |
| --- | ----- | ---------------------------- |
| 0   | 0     | Pin configurado como entrada |
| 1   | 1     | Pin configurado como salida  |

**Ejemplo:** configurar `P0.22` como salida digital:

```c
LPC_GPIO0->FIODIR |= (1 << 22);   // P0.22 como salida
```

---

## **6. FIOSET y FIOCLR – Salida digital**

* `FIOSET`: pone en alto (1) los bits que se escriben como 1. No afecta los demás.
* `FIOCLR`: pone en bajo (0) los bits que se escriben como 1. No afecta los demás.

**Ejemplo:**

```c
LPC_GPIO0->FIOSET = (1 << 22);  // Pone P0.22 en alto
LPC_GPIO0->FIOCLR = (1 << 22);  // Pone P0.22 en bajo
```

---

## **7. FIOPIN – Estado actual del puerto**

Este registro permite:

* Leer el estado de los pines
* Escribir un valor completo al puerto (cuando se desea modificar todos los bits a la vez)

**Ejemplo:**

```c
uint32_t valor = LPC_GPIO2->FIOPIN;     // Leo estado del puerto 2
int pin13 = (valor >> 13) & 1;          // Extraigo valor de P2.13
```

---

## **8. FIOMASK – Enmascarado**

Este registro permite **ignorar bits** al hacer lectura o escritura en `FIOPIN`, `FIOSET` o `FIOCLR`.

* Bit = 1 → ignorado
* Bit = 0 → activo

Útil para modificar varios bits sin afectar otros.

---

## **9. Interrupciones en GPIO** (se profundiza en la sección de interrupciones)

* Solo **puertos P0 y P2** pueden generar interrupciones.
* Se puede detectar flancos ascendentes, descendentes o ambos.
* Los registros involucrados son:

  * `IO0IntEnR`, `IO0IntEnF` → habilitan interrupción por flanco
  * `IO0IntStatR`, `IO0IntStatF` → indican si ocurrió un evento
  * `IO0IntClr` → limpia la interrupción

**Ejemplo de activación por flanco ascendente en P0.10:**

```c
LPC_GPIOINT->IO0IntEnR |= (1 << 10);  // Habilita interrupción por flanco ascendente
```

---

## **10. Estado tras el reset**

* Todos los pines están como **entradas** con **pull-up activado**.
* Se requiere configurar tanto PINSEL (función), PINMODE (resistencia), como FIODIR (dirección) antes de usar.


--- 
## Ejemplo de código 

Encender los 3 leds RGB de la placa LPC1769.

<img src="./img/rgb.png" width="50%">
 
### Pines utilizados:

| Color del LED | Pin MCU | Puerto | Número de bit |
| ------------- | ------- | ------ | ------------- |
| Rojo          | `P0.22` | P0     | 22            |
| Verde         | `P3.25` | P3     | 25            |
| Azul          | `P3.26` | P3     | 26            |

* Los cátodos (negativos) de los LEDs están conectados a los pines del MCU.
* El ánodo está conectado a +3.3V → **los LEDs se encienden cuando el pin MCU está en nivel bajo (0 lógico)**.

> Entonces: para **prender un LED**, se debe escribir un **0 en el pin** correspondiente. Para **apagarlo**, un **1**.

---

## Objetivo del programa

* Inicializar los pines como **GPIO salida**.
* Encender uno o más LEDs, por ejemplo: rojo encendido, verde y azul apagados.

---

## Código en C (con CMSIS)

```c
#include "LPC17xx.h"

#define RED_LED_PORT     LPC_GPIO0
#define RED_LED_PIN      22

#define GREEN_LED_PORT   LPC_GPIO3
#define GREEN_LED_PIN    25

#define BLUE_LED_PORT    LPC_GPIO3
#define BLUE_LED_PIN     26

void delay(void) {
    for (int i = 0; i < 1000000; i++); // Pequeña demora
}

int main(void) {
    // 1. Configurar función GPIO (PINSEL)
    LPC_PINCON->PINSEL1 &= ~(0x3 << 12); // P0.22: bits 13:12 en 00 (GPIO)
    LPC_PINCON->PINSEL7 &= ~(0x3 << 18); // P3.25: bits 19:18 en 00 (GPIO)
    LPC_PINCON->PINSEL7 &= ~(0x3 << 20); // P3.26: bits 21:20 en 00 (GPIO)

    // 2. Configurar sin pull-up/pull-down (PINMODE)
    LPC_PINCON->PINMODE1 |=  (0x2 << 12); // P0.22 sin pull-up/down
    LPC_PINCON->PINMODE7 |=  (0x2 << 18); // P3.25 sin pull-up/down
    LPC_PINCON->PINMODE7 |=  (0x2 << 20); // P3.26 sin pull-up/down

    // 3. Configurar como salida (FIODIR)
    RED_LED_PORT->FIODIR   |= (1 << RED_LED_PIN);
    GREEN_LED_PORT->FIODIR |= (1 << GREEN_LED_PIN);
    BLUE_LED_PORT->FIODIR  |= (1 << BLUE_LED_PIN);

    while (1) {
        // Encender rojo, apagar verde y azul
        RED_LED_PORT->FIOCLR   = (1 << RED_LED_PIN);  // Prende rojo
        GREEN_LED_PORT->FIOSET = (1 << GREEN_LED_PIN); // Apaga verde
        BLUE_LED_PORT->FIOSET  = (1 << BLUE_LED_PIN);  // Apaga azul
        delay();

        // Apagar rojo, prender verde
        RED_LED_PORT->FIOSET   = (1 << RED_LED_PIN);   // Apaga rojo
        GREEN_LED_PORT->FIOCLR = (1 << GREEN_LED_PIN); // Prende verde
        BLUE_LED_PORT->FIOSET  = (1 << BLUE_LED_PIN);  // Apaga azul
        delay();

        // Apagar rojo y verde, prender azul
        RED_LED_PORT->FIOSET   = (1 << RED_LED_PIN);
        GREEN_LED_PORT->FIOSET = (1 << GREEN_LED_PIN);
        BLUE_LED_PORT->FIOCLR  = (1 << BLUE_LED_PIN);  // Prende azul
        delay();
    }
}
```

---

## Desafío


Hacer un desarrollo que simule el funcionamiento de un semáforo.

