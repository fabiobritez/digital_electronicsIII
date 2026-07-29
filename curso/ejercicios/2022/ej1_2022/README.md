## Descripción General
Este programa captura y almacena los últimos 16 valores leídos desde los pines P2.0 a P2.7 del microcontrolador LPC17xx. Implementa un sistema robusto de debounce por software usando SysTick para eliminar falsos cambios causados por rebotes mecánicos o ruido eléctrico.

## Características
- **Captura por interrupción**: Los datos se capturan automáticamente cuando cambia cualquier pin
- **Buffer de 16 posiciones**: Mantiene un historial de los últimos 16 valores capturados
- **Debounce configurable**: Elimina rebotes y ruido con validación temporal
- **No bloqueante**: Todo el proceso se maneja por interrupciones


## Diagrama de Flujo

### Inicialización
1. Configura GPIO con interrupciones en ambos flancos
2. Configura SysTick
3. El sistema lee el valor inicial del puerto para establecer una referencia
4. El sistema queda listo para capturar cambios

### Durante la Operación
1. Cualquier cambio en P2.0-P2.7 genera una interrupción
2. Se inicia el proceso de debounce
3. Si el valor permanece estable 50ms, se valida
4. El dato se guarda en el buffer circular
5. El sistema queda listo para el siguiente cambio

## Implementación del Buffer Circular

### ¿Por qué un buffer circular?

El buffer circular es una estructura de datos para mantener un historial de tamaño fijo. En este caso, necesitamos almacenar los últimos 16 valores capturados.

### Opción 1: Buffer Circular con Índice (RECOMENDADA)

- El índice `index_write` apunta a la siguiente posición libre
- Al usar `& 0x0F` (AND con 15), el índice automáticamente vuelve a 0 después de 15
- Los datos más antiguos se sobrescriben automáticamente
- Para leer: el dato más reciente está en `(index_write - 1) & 0x0F`

```c
data[index_write] = new_data;
index_write = (index_write + 1) & 0x0F;  // Incremento circular (0-15)
```

**Ventajas:**
- **Baja complejidad temporal**: La inserción siempre toma tiempo constante (no depende de la cantidad de datos a guardar).
- **Eficiente en memoria**: No requiere mover datos, solo actualizar un índice
- **Menor consumo de energía**: Una sola escritura en memoria por dato nuevo
 

### Opción 2: Desplazamiento de Array
```c
for(int i = 15; i > 0; i--) {
    data[i] = data[i-1];
}
data[0] = new_data;
```

**Ventajas:**
- **Orden temporal claro**: `data[0]` siempre es el más reciente, `data[15]` el más antiguo
- **Fácil visualización**: Ideal para debugging o display secuencial
- **Simplicidad conceptual**: Más intuitivo para principiantes

**Desventajas:**
- **Complejidad temporal**: Debe mover 15 elementos cada vez. Si son más datos, crece.
- **Ineficiente**: 15 operaciones de lectura/escritura por cada dato nuevo
- **Mayor consumo de energía**: Más accesos a memoria = más consumo


## Sistema de Debounce con SysTick

### ¿Por qué es necesario el debounce?

Los switches mecánicos y muchos sensores no producen transiciones limpias. Cuando un switch se presiona o suelta, los contactos mecánicos "rebotan" múltiples veces antes de estabilizarse, generando múltiples interrupciones en microsegundos.

```
Señal real sin debounce:
Presión de botón: ‾‾‾\__/‾‾\_/‾\_____  (múltiples transiciones)
Lo que queremos:  ‾‾‾‾‾‾‾‾‾‾\________  (una transición limpia)
```

### Implementación del Algoritmo de Debounce

#### Fase 1: Detección del Cambio (GPIO Interrupt)
```c
void EINT3_IRQHandler(void) {
    if(gpio_interrupt_flag == 0) {  // Solo si no hay debounce activo
        gpio_interrupt_flag = 1;     // Activar proceso de validación
        debounce_counter = 0;         // Reiniciar contador de tiempo
        current_reading = LPC_GPIO2->FIOPIN & 0xFF;  // Capturar valor
    }
}
```

**¿Por qué así?**
- **Una sola instancia**: Evita múltiples procesos de debounce simultáneos
- **Captura inmediata**: Guarda el valor en el momento del cambio
- **No bloquea**: Sale rápidamente de la interrupción

#### Fase 2: Validación Temporal (SysTick cada 1ms)
```c
void SysTick_Handler(void) {
    if(gpio_interrupt_flag) {
        uint8_t new_reading = LPC_GPIO2->FIOPIN & 0xFF;
        
        if(new_reading == current_reading) {
            debounce_counter++;  // Señal estable, incrementar contador
            
            if(debounce_counter >= DEBOUNCE_TIME_MS) {
                // Señal estable por 50ms, es válida
                if(new_reading != last_stable_value) {
                    save_data(new_reading);  // Guardar solo si cambió
                    last_stable_value = new_reading;
                }
                gpio_interrupt_flag = 0;  // Fin del debounce
            }
        } else {
            // Señal cambió, reiniciar validación
            current_reading = new_reading;
            debounce_counter = 0;
        }
    }
}
```

**¿Por qué 50ms?**
La mayoría de switches mecánicos se estabilizan en 10-50ms
- **Balance**: Suficiente para eliminar rebotes, pero rápido para el usuario.

### Diagrama Temporal del Debounce

```
Tiempo (ms):     0    10   20   30   40   50   60
GPIO IRQ:        ↑
Lectura:         [A]  [A]  [A]  [B]  [B]  [B]  [B]
Counter:         0    10   20   0    10   20   30
Reinicio:                       ↑              
                                      
Lectura:         [B]  [B]  [B]  [B]  [B]  [B]
Counter:         0    10   20   30   40   50 → ¡VÁLIDO!
Guardado:                                   ↑
``` 

## Ajuste del Tiempo de Debounce

Se puede ajustar de acuerdo a la aplicación específica. Por ejemplo:
```c
#define DEBOUNCE_TIME_MS    50  // Modificar según necesidad
```

| Tipo de Entrada | Tiempo Recomendado | Justificación |
|-----------------|-------------------|---------------|
| Switches mecánicos | 30-50ms | Rebotes típicos duran 10-30ms |
| Botones de membrana | 20-30ms | Menos rebote que mecánicos |
| Señales digitales limpias | 0-2ms | Solo filtrar ruido |



### Acceso a los Datos

Para leer los datos almacenados:

**Con Buffer Circular:**
```c
// Dato más reciente
uint8_t newest = data[(index_write - 1) & 0x0F];

// Dato más antiguo
uint8_t oldest = data[index_write];

// Recorrer del más nuevo al más viejo
for(int i = 1; i <= 16; i++) {
    uint8_t valor = data[(index_write - i) & 0x0F];
    // Procesar valor...
}
```

**Con Desplazamiento:**
```c
// Dato más reciente
uint8_t newest = data[0];

// Dato más antiguo  
uint8_t oldest = data[15];

// Recorrer del más nuevo al más viejo
for(int i = 0; i < 16; i++) {
    uint8_t valor = data[i];
    // Procesar valor...
}
```
