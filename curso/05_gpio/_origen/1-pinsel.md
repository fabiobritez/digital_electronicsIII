# **Registros PINSEL**

En la LPC1769, la mayoría de los pines del microcontrolador pueden cumplir más de una función.
Por ejemplo, un mismo pin puede ser:

* Entrada/salida digital (GPIO)
* Entrada de un periférico (como UART, I²C, SPI, ADC…)
* Salida de un periférico (como PWM, DAC…)

Para decidir **qué función va a cumplir un pin**, el microcontrolador tiene multiplexores internos.
La selección se realiza a través de **registros especiales** llamados **PINSEL** (*Pin Function Select Registers*).

El PINSEL0 está mapeado en la dirección `0x4002C000`.

| Registro | Dirección | Descripción                        |
| -------- | --------- | ---------------------------------- |
| PINSEL0  | 0x4002C000 | Registro de selección de pines 0   |
| PINSEL1  | 0x4002C004 | Registro de selección de pines 1   |
| …        | …         | …                                  | 

[Revisar en las tablas del User Manual Rev 4.1, pagina 117 en adelante]

---

## ¿Cómo funcionan?

Cada pin tiene asociado un **par de bits** en algún registro PINSEL (PINSEL0, PINSEL1, … PINSEL9).
La combinación de esos dos bits define la función del pin:

| Bits | Función asignada                    |
| ---- | ----------------------------------- |
| 00   | Función primaria (GPIO por defecto) |
| 01   | Primera función alternativa         |
| 10   | Segunda función alternativa         |
| 11   | Tercera función alternativa         |

**Ejemplo:**
En el registro **PINSEL0**, los bits **1:0** controlan el pin **P0.0**.
Si ponemos:

* `00` → P0.0 es GPIO (entrada/salida digital)
* `01` → P0.0 se conecta a la señal RD1 (periférico externo)
* `10` → P0.0 se conecta a TXD3 (UART3 transmisión)
* `11` → P0.0 se conecta a SDA1 (I²C)

---


* Cada **registro PINSEL** puede controlar hasta 16 pines (dos bits por pin). 
* El valor **después del reset** es siempre `00` → todos los pines como GPIO.


 

## **Registros PINMODE**

Una vez que se selecciona la función del pin mediante los registros **PINSEL**, podemos configurar su **modo de entrada** a través de los registros **PINMODE** y **PINMODE\_OD**.

### 1. **¿Qué hace PINMODE?**

Controla cómo se comporta el pin cuando actúa como **entrada**, especialmente en relación con las resistencias internas de *pull-up* o *pull-down*.

Cada pin tiene **2 bits** en algún registro PINMODE que definen su configuración:

| Bits | Modo                                   |
| ---- | -------------------------------------- |
| 00   | Con resistencia *pull-up* habilitada   |
| 01   | Modo *repeater*                        |
| 10   | Sin *pull-up* ni *pull-down*           |
| 11   | Con resistencia *pull-down* habilitada |

> El valor por defecto (tras un reset) es `00`, es decir, con *pull-up* activo.

---

### 2. **¿Qué es el modo Repeater?**

El **modo Repeater** es útil para mantener el último estado lógico del pin:

* Si el pin está alto, activa el *pull-up*.
* Si el pin está bajo, activa el *pull-down*.

Este modo ayuda a evitar que el pin flote cuando no está siendo conducido, y se utiliza especialmente en estado de bajo consumo (*Deep Power-down*).

---

## **Registros PINMODE\_OD**

Este registro permite habilitar el **modo Open Drain** para pines de salida.

| Bit | Estado del pin               |
| --- | ---------------------------- |
| 0   | Modo normal (push-pull)      |
| 1   | Modo *open drain* habilitado |

En modo **open drain**:

* El pin puede **tirar a 0**, pero no puede forzar un 1.
* El 1 lógico debe ser provisto externamente (normalmente mediante una resistencia *pull-up*).


El registro PINMODE_OD0 está mapeado en la dirección `0x4002C068`



---

###  **Relación entre PINMODE y PINMODE\_OD**

* El **PINMODE** solo aplica cuando el pin está configurado como **entrada**, o bien en *open drain* cuando el pin **no está saliendo 0**.
* Si el pin está en open drain y saliendo 0, el valor de PINMODE no tiene efecto (el pin simplemente se fuerza a 0).

Esto permite, por ejemplo, configurar un pin como open drain **con pull-up activado solo cuando no se tira a 0**, útil en buses como **I²C**.

---

## Explicación de CMSIS

Si quisieramos configurar estos registros, deberiamos hacer punteros a cada uno de los registros y luego modificar los bits correspondientes. Pero alguien ya lo hizo por nosotros, y lo hizo un estandar para todos los microcontroladores ARM, y se llama CMSIS. Tienen un conjunto de librerias que nos permiten acceder a los registros de manera uniforme y similar en los micros ARM.

CMSIS traduce los registros de hardware a estructuras de C fáciles de usar.

 

## **¿Cómo accede CMSIS a los registros?**

En lugar de usar direcciones de memoria como `0x4002C000`, CMSIS define **estructuras de datos** que representan cada periférico del microcontrolador, y luego te permite acceder a ellos mediante punteros predefinidos.

Vamos a analizar el bloque **PINCON**, que maneja los registros PINSEL, PINMODE y PINMODE\_OD.
 
### **Definición de la estructura LPC\_PINCON\_TypeDef**

CMSIS define esta estructura en el archivo de cabecera:

```c
typedef struct {
  __IO uint32_t PINSEL0;
  __IO uint32_t PINSEL1;
  ...
  __IO uint32_t PINMODE0;
  ...
  __IO uint32_t PINMODE_OD0;
  ...
} LPC_PINCON_TypeDef;
```

Cada campo de la estructura representa **un registro de 32 bits** del microcontrolador.
La palabra clave `__IO` indica que se trata de una variable de **entrada/salida** (*input/output*), es decir, puede ser leída y escrita por el software o el hardware. Es un *macro* que define una variable como **volatile**.
 
### **Dirección base y puntero al periférico**

El bloque PINCON comienza en una dirección fija: `0x4002 C000`. CMSIS define un puntero a esa dirección:

```c
#define LPC_PINCON_BASE  (LPC_APB0_BASE + 0x2C000)
#define LPC_PINCON       ((LPC_PINCON_TypeDef *) LPC_PINCON_BASE)
```

> Así, `LPC_PINCON` es un puntero a una estructura que representa todos los registros de configuración de pines.

Por ejemplo:

* `LPC_PINCON->PINSEL0` accede al registro PINSEL0.
* `LPC_PINCON->PINMODE1` accede al registro PINMODE1.
* `LPC_PINCON->PINMODE_OD0` accede al registro de modo open drain del puerto 0. 
 

---

## **Ejemplo en C: Configuración de P0.0 como TXD3, sin pull-ups, en modo push-pull**

```c
#include <LPC17xx.h>

int main(void) {
    // 1. Selecciono la función alternativa TXD3 (valor 10 en PINSEL0 bits 1:0)
    LPC_PINCON->PINSEL0 &= ~(0x3 << 0);   // Limpio bits
    LPC_PINCON->PINSEL0 |=  (0x2 << 0);   // Escribo 10

    // 2. Desactivo pull-up y pull-down (valor 10 en PINMODE0 bits 1:0)
    LPC_PINCON->PINMODE0 &= ~(0x3 << 0);
    LPC_PINCON->PINMODE0 |=  (0x2 << 0);

    // 3. Modo normal (no open-drain)
    LPC_PINCON->PINMODE_OD0 &= ~(1 << 0); // Bit 0 en 0

    while (1) {
        // En este ejemplo solo configuramos el pin,
        // el periférico UART3 debe configurarse por separado.
    }
}
``` 
