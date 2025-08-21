# Documentación Librería GPIO para LPC17xx

## Introducción
Esta librería proporciona funciones básicas para manejar los pines GPIO (General Purpose Input/Output) del microcontrolador LPC17xx. Permite configurar pines como entrada o salida, leer y escribir valores digitales.

## Archivo de Cabecera (gpioHandler.h)

### Inclusiones
```c
#include <stdint.h>
```
- **stdint.h**: Biblioteca estándar que define tipos de datos enteros de tamaño fijo como `uint32_t` (entero sin signo de 32 bits)

### Enumeraciones

#### `enum Ports`
```c
enum Ports {PORT0, PORT1, PORT2, PORT3, PORT4};
```
Define los puertos GPIO disponibles en el LPC17xx:
- **PORT0 = 0**: Puerto 0 (pines P0.0 - P0.31)
- **PORT1 = 1**: Puerto 1 (pines P1.0 - P1.31)
- **PORT2 = 2**: Puerto 2 (pines P2.0 - P2.31)
- **PORT3 = 3**: Puerto 3 (pines P3.0 - P3.31)
- **PORT4 = 4**: Puerto 4 (pines P4.0 - P4.31)

#### `enum State`
```c
enum State {LOW, HIGH};
```
Define los estados lógicos de un pin:
- **LOW = 0**: Nivel lógico bajo (0V)
- **HIGH = 1**: Nivel lógico alto (3.3V)

#### `enum Dir`
```c
enum Dir {INPUT, OUTPUT};
```
Define la dirección del pin:
- **INPUT = 0**: Pin configurado como entrada (lee señales)
- **OUTPUT = 1**: Pin configurado como salida (genera señales)

#### `enum Mode`
```c
enum Mode {PULLUP, REPEATER, NEITHER, PULLDOWN};
```
Define el modo de resistencia interna del pin:
- **PULLUP = 0**: Resistencia pull-up activada (pin tiende a HIGH cuando no está conectado)
- **REPEATER = 1**: Modo repetidor (mantiene el último estado)
- **NEITHER = 2**: Sin resistencia interna
- **PULLDOWN = 3**: Resistencia pull-down activada (pin tiende a LOW cuando no está conectado)

### Prototipos de Funciones
```c
void gpioConfig(int portNumber, int pin, int pinMode, int direction);
void gpioWrite(int portNumber, int pin, int state);
int gpioRead(int portNumber, int pin);
```

## Archivo de Implementación (gpioHandler.c)

### Inclusiones
```c
#include <stdio.h>
#include <stdint.h>
#include <LPC17xx.h>
#include "gpioHandler.h"
```
- **LPC17xx.h**: Biblioteca específica del microcontrolador que define registros y estructuras

### Variables Globales (Punteros a Registros)

```c
volatile uint32_t* pinselBase = &LPC_PINCON->PINSEL0;
volatile uint32_t* pinmodeBase = &LPC_PINCON->PINMODE0;
volatile uint32_t* fiodirBase = &LPC_GPIO0->FIODIR;
volatile uint32_t* fiopinBase = &LPC_GPIO0->FIOPIN;
```

Estas variables apuntan a los registros base del hardware:

- **pinselBase**: Apunta a PINSEL0, controla la función de cada pin (GPIO, UART, SPI, etc.)
- **pinmodeBase**: Apunta a PINMODE0, controla las resistencias pull-up/pull-down
- **fiodirBase**: Apunta a FIODIR del GPIO0, controla la dirección (entrada/salida)
- **fiopinBase**: Apunta a FIOPIN del GPIO0, lee/escribe el estado de los pines

**¿Por qué `volatile`?** Indica al compilador que estos registros pueden cambiar por hardware, evitando optimizaciones incorrectas.

### Función `gpioConfig`

```c
void gpioConfig(int portNumber, int pin, int pinMode, int direction) {
    int column = (pin<=15) ? 0:1;
    int pinAux = (pin>=16) ? pin-16:pin;
    
    *(pinselBase + portNumber*2 + column) &= ~(3<<2*pinAux);
    *(pinmodeBase + portNumber*2 + column) &= ~(3<<2*pinAux);
    *(fiodirBase + portNumber*8) &= ~(1<<pin);
    
    if(pinMode!=0)
        *(pinmodeBase + portNumber*2 + column) |= (pinMode<<2*pinAux);
    
    *(fiodirBase + portNumber*8) |= (direction<<pin);
}
```

**Propósito**: Configura un pin específico con el modo y dirección deseados.

**Parámetros**:
- `portNumber`: Número del puerto (0-4)
- `pin`: Número del pin dentro del puerto (0-31)
- `pinMode`: Modo de resistencia (PULLUP, REPEATER, NEITHER, PULLDOWN)
- `direction`: Dirección del pin (INPUT, OUTPUT)

**Explicación paso a paso**:

1. **Cálculo de registros**:
   ```c
   int column = (pin<=15) ? 0:1;
   int pinAux = (pin>=16) ? pin-16:pin;
   ```
   - Los registros PINSEL y PINMODE están divididos en pares (PINSEL0/1, PINSEL2/3, etc.)
   - Pines 0-15 usan el registro par (column=0), pines 16-31 usan el impar (column=1)
   - `pinAux` ajusta el número de pin para el registro correspondiente

2. **Configurar función GPIO**:
   ```c
   *(pinselBase + portNumber*2 + column) &= ~(3<<2*pinAux);
   ```
   - Limpia los 2 bits correspondientes al pin en PINSEL
   - `3<<2*pinAux` crea una máscara con 11 binario en la posición correcta
   - `&= ~(...)` pone esos bits en 00, configurando el pin como GPIO

3. **Limpiar modo anterior**:
   ```c
   *(pinmodeBase + portNumber*2 + column) &= ~(3<<2*pinAux);
   ```
   - Limpia la configuración previa de resistencias

4. **Limpiar dirección anterior**:
   ```c
   *(fiodirBase + portNumber*8) &= ~(1<<pin);
   ```
   - Pone el bit del pin en 0 (INPUT por defecto)
   - `portNumber*8` porque cada puerto tiene 8 registros de separación

5. **Configurar nuevo modo**:
   ```c
   if(pinMode!=0)
       *(pinmodeBase + portNumber*2 + column) |= (pinMode<<2*pinAux);
   ```
   - Si no es PULLUP (0), configura el modo especificado

6. **Configurar dirección**:
   ```c
   *(fiodirBase + portNumber*8) |= (direction<<pin);
   ```
   - Configura la dirección del pin (0=INPUT, 1=OUTPUT)

### Función `gpioWrite`

```c
void gpioWrite(int portNumber, int pin, int state) {
    if (state==LOW)
        *(fiopinBase + portNumber*8) &= ~(1<<pin);
    else
        *(fiopinBase + portNumber*8) |= (1<<pin);
}
```

**Propósito**: Escribe un valor digital (HIGH/LOW) en un pin de salida.

**Parámetros**:
- `portNumber`: Número del puerto
- `pin`: Número del pin
- `state`: Estado a escribir (LOW o HIGH)

**Funcionamiento**:
- **Si state == LOW**: `&= ~(1<<pin)` pone el bit del pin en 0
- **Si state == HIGH**: `|= (1<<pin)` pone el bit del pin en 1

### Función `gpioRead`

```c
int gpioRead(int portNumber, int pin) {
    return (*(fiopinBase + portNumber*8)>>pin) & 0x01;
}
```

**Propósito**: Lee el estado actual de un pin.

**Parámetros**:
- `portNumber`: Número del puerto
- `pin`: Número del pin

**Funcionamiento**:
- Lee el registro FIOPIN del puerto especificado
- `>>pin` desplaza el bit del pin a la posición LSB
- `& 0x01` enmascara para obtener solo el bit menos significativo
- **Retorna**: 0 (LOW) o 1 (HIGH)

## Ejemplo de Uso

```c
#include "gpioHandler.h"

int main() {
    // Configurar P0.22 como salida sin resistencias
    gpioConfig(PORT0, 22, NEITHER, OUTPUT);
    
    // Configurar P0.15 como entrada con pull-up
    gpioConfig(PORT0, 15, PULLUP, INPUT);
    
    // Encender LED en P0.22
    gpioWrite(PORT0, 22, HIGH);
    
    // Leer estado del botón en P0.15
    int buttonState = gpioRead(PORT0, 15);
    
    return 0;
}
```
