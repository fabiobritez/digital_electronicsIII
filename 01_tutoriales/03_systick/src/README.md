# Control de LED con SysTick y Interrupciones Externas

## Descripción del programa

Este programa para **LPC1769** demuestra el uso del timer SysTick en conjunto con interrupciones externas para controlar un LED. El sistema genera un patrón de parpadeo específico que puede ser habilitado o deshabilitado mediante un botón externo.

- **Control por interrupción externa**: Botón en P2.12 (EINT2) para activar/desactivar el sistema
- **Timer SysTick**: Genera interrupciones cada 10ms para control temporal
- **Patrón de LED**: Reproduce una forma de onda predefinida (10101010) en el LED
- **Estado del sistema**: Alterna entre encendido y apagado con cada pulsación del botón

## Hardware Requerido
- **LED**: Conectado al pin P0.22
- **Botón/Switch**: Conectado al pin P2.12
- **Resistencias**: Pull-up para el botón (opcional, usa pull-up interno)


## Funcionamiento

### Estado Inicial
- El sistema inicia con el LED apagado
- El timer SysTick está configurado pero las interrupciones están deshabilitadas

### Operación Normal
1. **Primera pulsación del botón**: Habilita las interrupciones del SysTick
2. **Timer SysTick**: Genera interrupciones cada 10ms
3. **Patrón de LED**: Cada 100ms (10 interrupciones) cambia el estado del LED según el patrón `0xAA` (10101010)
4. **Segunda pulsación del botón**: Apaga el LED y deshabilita las interrupciones del SysTick

### Patrón de Onda
```
Patrón: 0xAA = 10101010 (binario)
Frecuencia: 10 Hz (100ms por bit)
Secuencia: ON-OFF-ON-OFF-ON-OFF-ON-OFF (se repite)
```

## Configuración del Sistema

### Timer SysTick
- **Frecuencia**: 100 Hz (10ms por interrupción)
- **Clock source**: SystemCoreClock
- **Modo**: Interrupción habilitada

### Interrupción Externa (EINT2)
- **Pin**: P2.12
- **Modo**: Edge-triggered (flanco de bajada)
- **Pull-up**: Habilitado internamente

### GPIO
- **P0.22**: Configurado como salida digital para el LED

## Estructura del Código

### Funciones Principales

#### `main()`
- Inicializa el hardware
- Entra en bucle infinito esperando interrupciones

#### `ledConfig()`
- Configura P0.22 como GPIO de salida
- Prepara el pin para controlar el LED

#### `intConfig()`
- Configura P2.12 como EINT2 con pull-up
- Configura flanco de bajada para la interrupción
- Inicializa el SysTick con CMSIS
- Habilita las interrupciones en el NVIC

#### `EINT2_IRQHandler()`
- Manejador de la interrupción externa
- Alterna entre habilitar/deshabilitar el SysTick
- Controla el estado ON/OFF del sistema

#### `SysTick_Handler()`
- Manejador del timer SysTick
- Implementa divisor de frecuencia (10:1)
- Genera el patrón de onda en el LED

