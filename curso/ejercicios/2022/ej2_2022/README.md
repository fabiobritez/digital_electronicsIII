
## Descripción General

### Ejercicio N° 2 - Parcial 2022
 
 Utilizando interrupciones por systick y por eventos externos EINT.
  Realizar un programa que permita habilitar y deshabilitar el temporizador por
 el flanco ascendente en el pin P2.11.

 El temporizador debe desbordar cada 10 milisegundos utilizando un reloj de core cclk = 62MHz.
 Por cada interrupcion del Systick se debe mostrar por el puerto P0 el promedio de los
 valores guardados en la variables "uint8_t values[8]"
 
 Se pide ademas detallar los calculos realizados para obtener el valor a cargar en el
 registro RELOAD y asegurar que la interrupcion por Systick sea mayor que la
 prioridad de la interrupcion del evento externo.

 El codigo debe estar debidamente comentado. 

## Especificaciones del Sistema

### Requisitos Funcionales
- Timer SysTick configurado para interrupciones cada 10 milisegundos
- Control de habilitación mediante flanco ascendente en P2.11 (EINT1)
- Cálculo del promedio de 8 valores almacenados en memoria
- Visualización del resultado en puerto P0[7:0]
- Frecuencia del core: 62 MHz

### Requisitos de Prioridad
El sistema debe garantizar que la interrupción externa EINT1 tenga mayor prioridad que SysTick, permitiendo control inmediato del temporizador sin latencia.

## Análisis del Problema

### Cálculo del Registro RELOAD

Para obtener interrupciones cada 10ms con un reloj de 62MHz:

```
Frecuencia del sistema = 62 MHz = 62,000,000 Hz
Período objetivo = 10 ms = 0.01 segundos
Ciclos necesarios = Frecuencia × Período = 62,000,000 × 0.01 = 620,000
Valor RELOAD = Ciclos - 1 = 619,999
```

El valor 619,999 está dentro del rango válido del registro RELOAD (24 bits máximo = 16,777,215).
  

## Estructura

```
main()
  ├── SystemInit()
  ├── configGPIO()
  │     └── Configuración P0[7:0] como salidas
  ├── configEINT()
  │     ├── P2.11 como EINT1
  │     ├── Flanco ascendente
  │     └── Pull-down habilitado
  └── configSysTick()
        ├── RELOAD = 619,999
        └── Clock source = CCLK
```

### Flujo de Interrupciones

#### EINT1_IRQHandler
1. Limpiar flag de interrupción (EXTINT)
2. Alternar estado del flag de control
3. Habilitar o deshabilitar interrupción SysTick según estado

#### SysTick_Handler
1. Calcular promedio del array de datos
2. Escribir resultado en puerto P0[7:0]
3. Retornar (automático clear del flag)

## Configuración de Hardware

### Registro SysTick->CTRL

| Bit | Campo | Valor | Descripción |
|-----|-------|-------|-------------|
| 0 | ENABLE | 1 | Habilita el contador SysTick |
| 1 | TICKINT | Variable | Control dinámico por EINT1 |
| 2 | CLKSOURCE | 1 | Usa reloj del procesador (62MHz) |

 

## Implementación de Funciones

### Cálculo de Promedio

La función debe procesar 8 valores de 8 bits y retornar el promedio como entero sin signo:

```c
uint8_t calcularPromedio(uint8_t values[8])
{
    uint16_t suma = 0;  // Usar 16 bits para evitar overflow
    for(int i = 0; i < 8; i++) {
        suma += values[i];
    }
    return (uint8_t)(suma / 8);  // División entera
}
```

### Control de Estado del SysTick

El control se realiza modificando únicamente el bit TICKINT del registro CTRL:

```c
// Deshabilitar interrupciones SysTick
SysTick->CTRL &= ~(1 << 1);

// Habilitar interrupciones SysTick  
SysTick->CTRL |= (1 << 1);
```

## Consideraciones

### Escritura en GPIO

Para preservar el estado de otros pines del puerto, se debe usar máscara al escribir:

```c
LPC_GPIO0->FIOPIN = (LPC_GPIO0->FIOPIN & ~0xFF) | (promedio & 0xFF);
```

### Limpieza de Flags de Interrupción

Es obligatorio limpiar el flag EXTINT después de atender la interrupción externa:

```c
LPC_SC->EXTINT = (1 << 1);  // Clear EINT1 flag
```

El flag de SysTick se limpia automáticamente al leer el registro CTRL.

### Variables Volátiles

Las variables compartidas entre el contexto principal y las ISR deben declararse como `volatile`:

```c
volatile uint8_t systick_habilitado = 1;
```
 