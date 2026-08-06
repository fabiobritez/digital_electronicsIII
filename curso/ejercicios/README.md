# Ejercicios de parciales: Electrónica Digital III

Ejercicios resueltos de exámenes parciales de años anteriores. Sirven para practicar la integración
de varios periféricos y para reconocer los errores típicos antes de que te cuesten puntos.

## Ejercicios por año

### 2022
- [ej1_2022](./2022/ej1_2022/): captura de datos GPIO con *debounce* por software
- [ej2_2022](./2022/ej2_2022/): temporizador SysTick con interrupciones externas

### 2023
- [ej3_2023](./2023/ej3_2023/): recepción de datos con modulación PWM

### 2025
- [ej_2025_deserializer](./2025/ej_2025_deserializer/): deserializador serial con distribución par/impar
- [parcial1_2025](./2025/parcial1_2025/): primer parcial 2025
  - `punto1.c`, `punto1_b.c`: ejercicio 1 y variante
  - `punto2.c`, `punto2_mejorado.c`: ejercicio 2 y versión mejorada
  - `punto2_errores.md`: análisis detallado de errores comunes

## Cómo estudiar con estos ejercicios

1. **Resolvé por tu cuenta primero.** Leé el enunciado, identificá qué periféricos necesitás e intentá
   implementarlo sin mirar la solución.
2. **Compará con la solución** provista: diferencias, mejoras, decisiones.
3. **Estudiá los errores comunes** (ej. `punto2_errores.md`): entender por qué algo está mal vale
   tanto como saber lo correcto.
4. **Experimentá:** modificá parámetros y probá casos límite (flancos, overflows, rebote).

## Temas frecuentes en parciales

Periféricos: GPIO, SysTick, NVIC, EINT, Timers (match/capture/PWM), DMA.
Conceptos: configuración de registros (PINSEL/PINMODE), prioridades de interrupción, sincronización de
eventos, medición de tiempos/períodos, detección de flancos, *debounce* por software, máscaras de bits.

## Cuatro errores que cuestan parciales

**1. Confundir `|` (OR) con `&` (AND) al armar un registro**
```c
SysTick->CTRL = (1<<0) & (1<<1) & (1<<2);   // da 0 (no comparten bits)
SysTick->CTRL = (1<<0) | (1<<1) | (1<<2);   // da 7
```

**2. Mezclar interrupción por GPIO con EINT**
```c
// habilitar GPIO interrupt pero pensar que es EINT
LPC_GPIOINT->IO2IntEnR |= (1<<11);   // esto es interrupción por GPIO (vector EINT3)
NVIC_EnableIRQ(EINT3_IRQn);

// para EINT1 de verdad (pin dedicado)
LPC_PINCON->PINSEL4 |= (1 << 22);    // pin como EINT1
LPC_SC->EXTMODE |= (1 << 1);         // por flanco
NVIC_EnableIRQ(EINT1_IRQn);
```
(Ver [módulo 07](../07_interrupciones/02-eint-y-gpio.md) para la diferencia.)

**3. Usar `SysTick->VAL` (cuenta hacia atrás) para medir tiempo**
```c
uint32_t now = SysTick->VAL;          // VAL decrementa; restar da negativo/raro
uint32_t now = millis;                // usar un contador que incrementa en el handler
```

**4. Prioridades al revés**
- Alta prioridad (número bajo): eventos que necesitan precisión temporal (EINT, Capture).
- Baja prioridad: interrupciones periódicas que solo actualizan variables (SysTick).

## Consejos para el parcial

1. Leé el enunciado **completo** antes de codificar.
2. Identificá los periféricos y planificá las prioridades de interrupción.
3. Dibujá diagramas de tiempo si ayuda.
4. **Revisá PINSEL/PCONP** antes de usar cualquier periférico (el "ritual de arranque").
5. Comentá tu razonamiento y probá casos límite (flancos, overflows).

## Enlaces

- Curso: [../README.md](../README.md) · Ejemplos: [../ejemplos/](../ejemplos/) ·
  Lenguaje C: [../00_lenguaje_c/](../00_lenguaje_c/)
