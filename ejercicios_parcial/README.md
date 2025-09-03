# Ejercicios de Parcial - Electrónica Digital III

Esta contiene ejercicios prácticos de parciales anteriores y ejercicios.

## Resumen de Ejercicios

### 1. ej_2025_deserializer - Sistema Deserializador Serial con Distribución Par/Impar

**Descripción**: Implementa un deserializador que recibe una señal serial y distribuye los bits según su paridad de índice (pares a P0.2, impares a P0.1). Incluye detección de patrones especiales para modos de funcionamiento (normal, cero, repetición).

**Periféricos utilizados**:
- GPIO: P0.0 (entrada serial), P0.1 (salida bits impares), P0.2 (salida bits pares)
- SysTick Timer: Para temporización de muestreo cada 10ms

### 2. ej1_2022 - Captura de Datos GPIO con Debounce por Software

**Descripción**: Sistema que captura cambios en los pines P2.0-P2.7 y los almacena en un buffer circular de 16 posiciones. Implementa debounce por software usando SysTick para eliminar rebotes mecánicos o ruido.

**Periféricos utilizados**:
- GPIO: P2.0-P2.7 (entradas con pull-up)
- GPIOINT: Interrupciones por flanco ascendente y descendente
- SysTick Timer: Para temporización del debounce (50ms)
- NVIC: Control de interrupciones

### 3. ej2_2022 - Control de Temporizador SysTick con Interrupciones Externas

**Descripción**: Sistema que permite habilitar/deshabilitar un temporizador SysTick mediante flanco ascendente en P2.11. El temporizador desborda cada 10ms (con core a 62MHz) y muestra el promedio de 8 valores predefinidos en el puerto P0.

**Periféricos utilizados**:
- GPIO: P0[7:0] (salidas para mostrar promedio), P2.11 (entrada EINT1)
- SysTick Timer: Temporizador configurable con RELOAD = 619,999
- EINT1: Interrupción externa por flanco ascendente
- NVIC: Gestión de prioridades de interrupción (EINT1 > SysTick)

### 4. ej3_2023 - Recepción de Datos con Modulación PWM

**Descripción**: Sistema que recibe una secuencia de 10 bits con formato específico (preámbulo 30ms + datos 50ms + postámbulo 30ms) y genera una señal PWM proporcional al valor recibido (0-1023 mapeado a 0-50ms de ancho de pulso).

**Periféricos utilizados**:
- GPIO: P0.0 (entrada de datos serial), P0.2 (salida PWM)
- SysTick Timer: Para temporización precisa de muestreo (1ms) y generación PWM

## Estructura Común de los Ejercicios

Todos los ejercicios siguen una estructura similar:
- `main.c`: Código principal con lógica de aplicación
- `REAMDE.md` o `README.md`: Documentación detallada del ejercicio

