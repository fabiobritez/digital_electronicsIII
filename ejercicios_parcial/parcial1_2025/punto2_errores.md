# Análisis del Sistema de Parpadeo LED con Medición de Período

## Errores Identificados en el Código Original

### 1. Error en la configuración de SysTick

**Código Original (INCORRECTO):**
```c
SysTick->CTRL = (1<<0) & (1<<1) & (1<<2);  // Resultado: 0x00000000
```

**Problema:** Usa AND (&) en lugar de OR (|), resultando en 0 (SysTick deshabilitado)

**Código Corregido:**
```c
SysTick->CTRL = (1<<0) | (1<<1) | (1<<2);  // Resultado: 0x00000007
```

### 2. Configuración Incorrecta de EINT1

**Código Original (INCORRECTO):**
```c
void eint1_config_reg(void){
    LPC_GPIOINT->IO2IntEnR |= (1<<11);  // Esto es GPIO interrupt, no EINT1
    NVIC_EnableIRQ(EINT3_IRQn);         // IRQ incorrecta
}
```

- Mezclaba configuración de GPIO interrupt con EINT1
- Usaba EINT3_IRQn en lugar de EINT1_IRQn
- No configuraba EXTMODE y EXTPOLAR

**Código Corregido:**
```c
void config_EINT1(void) {
    // Configurar P2.11 como EINT1
    LPC_PINCON->PINSEL4 |= (1 << 22);
    
    // Configurar para flanco ascendente
    LPC_SC->EXTMODE |= (1 << 1);   // Modo flanco
    LPC_SC->EXTPOLAR |= (1 << 1);  // Flanco ascendente
    
    NVIC_EnableIRQ(EINT1_IRQn);    // IRQ correcta
}
```

### 3. Medición de Período Incorrecta

**Código Original (INCORRECTO):**
```c
uint32_t now = SysTick->VAL;  // VAL cuenta hacia atrás
periodo = now - t_prev;       // Cálculo incorrecto
```

**Problema:** SysTick->VAL cuenta hacia atrás desde LOAD hasta 0

**Código Corregido:**
```c
// Usar contador de milisegundos que incrementa
uint32_t current_time = t;
measured_period = t - t_prev;
```

### 4. Falta de Control del LED

**Código Original:**
```c
while(1) {
    // TODO: parpadear LED con periodo medido
}
```

**Código Corregido:**
```c
while(1) {
    if ((t - last_led_toggle) >= (led_period / 2)) {
        toggle_led();
        last_led_toggle = t;
    }
}
```

## Configuración de Prioridades

### Análisis de Prioridades Relativas

| Interrupción | Prioridad | Justificación |
|--------------|-----------|---------------|
| EINT1 | 1 (Alta) | Debe capturar flancos con precisión temporal |
| SysTick | 3 (Baja) | Solo incrementa contador, puede tolerar pequeños retrasos |

**Justificación detallada:**

1. **EINT1 con mayor prioridad:**
   - Necesita timestamp preciso del flanco
   - Pérdida de flanco = medición incorrecta
   - Duración corta de ISR

2. **SysTick con menor prioridad:**
   - Solo incrementa un contador
   - Retraso de microsegundos no afecta
   - Se ejecuta 1000 veces/segundo

### Escenario de Conflicto

```
Tiempo ─────────────────────────────────────►
        ↑                    ↑
     SysTick              EINT1
     (empieza)           (llega)
 
Con prioridades correctas: EINT1 interrumpe → medición precisa
```

## Justificación de la Base de Tiempo (1ms)


1. Error <1% en todo el rango
2. 1000 interrupciones/segundo
3. **Simplicidad:** Fácil trabajar con milisegundos 

### Cálculo del RELOAD para 1ms:

```
Asumiendo CCLK = 100MHz 
RELOAD = (100,000,000 / 1,000) - 1 = 99,999
```

## Modo de Interrupción EINT1

### Configuración Elegida

| Parámetro | Valor | Registro | Justificación |
|-----------|-------|----------|---------------|
| Modo | Flanco | EXTMODE[1] = 1 | Detecta transiciones, no niveles |
| Polaridad | Ascendente | EXTPOLAR[1] = 1 | Punto consistente de medición |

### Comparación de Modos

```
Señal cuadrada:
     ┌────┐    ┌────┐    ┌────┐
─────┘    └────┘    └────┘    └─────
     ↑         ↑         ↑
   Flanco   Flanco   Flanco
   ascend.  ascend.  ascend.
   
   └─Período─┘└─Período─┘
```

**Por qué flanco ascendente:**
1. Punto único y definido por período
2. Evita mediciones dobles
3. Inmune a duty cycle variable
 