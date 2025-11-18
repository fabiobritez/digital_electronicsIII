# Ejercicios de Parciales - Electrónica Digital III

Esta carpeta contiene ejercicios resueltos de exámenes parciales de años anteriores.

## 📚 Ejercicios por año

### 2022
- **[ej1_2022](./2022/ej1_2022/)** - Captura de datos GPIO con debounce por software
- **[ej2_2022](./2022/ej2_2022/)** - Control de temporizador SysTick con interrupciones externas

### 2023
- **[ej3_2023](./2023/ej3_2023/)** - Recepción de datos con modulación PWM

### 2025
- **[ej_2025_deserializer](./2025/ej_2025_deserializer/)** - Sistema deserializador serial con distribución par/impar
- **[parcial1_2025](./2025/parcial1_2025/)** - Ejercicios del primer parcial 2025
  - `punto1.c` - Ejercicio 1
  - `punto1_b.c` - Ejercicio 1 variante B
  - `punto2.c` - Ejercicio 2
  - `punto2_mejorado.c` - Ejercicio 2 versión mejorada
  - `punto2_errores.md` - Análisis detallado de errores comunes

---

## 🎯 Propósito de estos ejercicios

Estos ejercicios te ayudan a:

1. **Prepararte para exámenes** - Familiarizarte con el tipo de problemas que se presentan
2. **Practicar conceptos clave** - Aplicar conocimientos de múltiples periféricos
3. **Aprender de errores comunes** - Evitar los errores típicos documentados
4. **Integrar conocimientos** - Combinar GPIO, timers, interrupciones, DMA, etc.

---

## 📖 Cómo estudiar con estos ejercicios

### Paso 1: Intenta resolver por tu cuenta
- Lee el enunciado en el README.md de cada ejercicio
- Intenta implementar la solución sin ver el código
- Identifica qué periféricos y conceptos necesitas

### Paso 2: Compara con la solución
- Revisa el código proporcionado
- Compara con tu implementación
- Identifica diferencias y mejoras

### Paso 3: Analiza los errores comunes
- Lee los archivos de errores (ej: `punto2_errores.md`)
- Entiende por qué ciertos enfoques son incorrectos
- Aprende las mejores prácticas

### Paso 4: Experimenta
- Modifica el código
- Prueba diferentes configuraciones
- Valida tu comprensión

---

## 🔑 Temas frecuentes en parciales

### Periféricos más utilizados
- ✅ **GPIO** - Entrada/salida digital, LEDs, botones
- ✅ **SysTick** - Base de tiempo, temporizaciones
- ✅ **NVIC** - Prioridades de interrupción
- ✅ **EINT** - Interrupciones externas
- ✅ **Timers** - Match, Capture, PWM
- ✅ **DMA** - Transferencias de datos eficientes

### Conceptos clave
- ⚡ Configuración de registros (PINSEL, PINMODE)
- ⚡ Manejo de interrupciones y prioridades
- ⚡ Sincronización de eventos
- ⚡ Medición de tiempos y períodos
- ⚡ Detección de flancos
- ⚡ Debouncing por software
- ⚡ Máscaras de bits y operaciones bitwise

---

## ⚠️ Errores comunes a evitar

### 1. Operadores lógicos incorrectos
```c
// ❌ INCORRECTO
SysTick->CTRL = (1<<0) & (1<<1) & (1<<2);  // Resultado: 0

// ✅ CORRECTO
SysTick->CTRL = (1<<0) | (1<<1) | (1<<2);  // Resultado: 7
```

### 2. Confusión entre interrupciones
```c
// ❌ INCORRECTO - Mezclar EINT con GPIO interrupt
LPC_GPIOINT->IO2IntEnR |= (1<<11);  // Esto es GPIO interrupt
NVIC_EnableIRQ(EINT3_IRQn);         // Esto es EINT3

// ✅ CORRECTO - Para EINT1
LPC_PINCON->PINSEL4 |= (1 << 22);   // Configurar pin como EINT1
LPC_SC->EXTMODE |= (1 << 1);        // Modo flanco
NVIC_EnableIRQ(EINT1_IRQn);         // IRQ correcta
```

### 3. Uso incorrecto de SysTick->VAL
```c
// ❌ INCORRECTO - VAL cuenta hacia atrás
uint32_t now = SysTick->VAL;
periodo = now - t_prev;

// ✅ CORRECTO - Usar contador que incrementa
uint32_t current_time = millis;
periodo = current_time - t_prev;
```

### 4. Prioridades mal configuradas
- **Alta prioridad**: Interrupciones que requieren precisión temporal (EINT, Capture)
- **Baja prioridad**: Interrupciones periódicas que solo actualizan variables (SysTick)

---

## 🔗 Recursos relacionados

- **Tutoriales**: [/01_tutoriales](../01_tutoriales/) - Aprende los conceptos base
- **Ejemplos**: [/02_ejemplos](../02_ejemplos/) - Código de referencia por periférico
- **Fundamentos de C**: [/00_fundamentos/c_basics](../00_fundamentos/c_basics/) - Repasa C para embebidos

---

## 📝 Consejos para el parcial

1. **Lee el enunciado completo** antes de empezar a codificar
2. **Identifica los periféricos** que necesitas usar
3. **Planifica las prioridades** de interrupción
4. **Dibuja diagramas** de tiempo si es necesario
5. **Revisa la configuración** de PINSEL antes de usar cualquier periférico
6. **Comenta tu código** para explicar tu razonamiento
7. **Prueba casos límite** (flancos, overflows, etc.)

¡Buena suerte en tus parciales! 🚀
