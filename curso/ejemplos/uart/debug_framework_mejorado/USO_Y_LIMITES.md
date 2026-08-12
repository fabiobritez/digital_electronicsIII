# Cómo usar el Debug Framework mejorado

Este documento explica cuándo conviene usar el framework, qué costo tiene y qué pasa cuando la
aplicación produce mensajes más rápido de lo que la UART puede transmitir. Los números corresponden
a una LPCXpresso LPC1769 a 100 MHz, compilada con GCC 13.2 y `-Og`.

La configuración recomendada para obtener el mayor rendimiento de esta implementación es:

```bash
make -B USE_CMSIS=1 \
  EXTRA_CFLAGS="-DDEBUG_BACKEND=DEBUG_BACKEND_DMA -DDEBUG_BAUD=921600" \
  flash
```

## Fortalezas y debilidades

| Fortalezas | Debilidades |
|---|---|
| Devuelve rápido el control con IRQ o DMA. | La cola es finita y descarta cuando se llena. |
| Funciona sin una sesión de debugger. | Usa UART0, un pin y, con DMA, el canal 0. |
| Alcanza 92 104 B/s a 921600. | La llamada rápida no reduce el tiempo físico del cable. |
| Cada llamada entra completa o se descarta completa. | Una línea formada con varias llamadas puede quedar incompleta. |
| Cuenta los bytes descartados internamente. | No sabe si la PC recibió correctamente. |
| Evita el costo general de `printf`. | Ofrece menos flexibilidad de formato. |

Para una consola habitual con el debugger conectado, RTT sigue siendo más cómodo. Este framework
queda mejor posicionado cuando necesitás independencia del debugger, una terminal serie común o
mayor caudal. Para eventos de muy alta frecuencia, conviene registrar datos binarios en RAM y
volcarlos después.

## La idea que hay que tener presente

En el backend DMA, una llamada no espera que los bytes lleguen a la PC:

```text
aplicación -> copia a la cola -> DMA -> FIFO de UART0 -> cable -> CP2102 -> PC
              la llamada        la transmisión continúa después
              termina acá
```

Por eso hay tres tiempos diferentes:

1. El tiempo que tarda la llamada en copiar el mensaje a la cola.
2. El tiempo durante el cual el CPU atiende las interrupciones de finalización.
3. El tiempo físico que necesita la UART para sacar todos los bits.

Para un mensaje de 48 bytes, `DBG_MSG()` devuelve el control en 5,84 µs. Sin embargo, esos bytes
necesitan aproximadamente 521 µs para salir por una UART 8N1 a 921600.

DMA evita que el CPU espere esos 521 µs. No hace que el cable sea más rápido.

## Cuánto se puede transmitir

En 8N1 cada byte ocupa diez bits: uno de inicio, ocho de datos y uno de parada. El límite nominal se
calcula así:

```text
bytes por segundo = baudrate / 10
```

En esta placa, el baudrate real configurado es 921053. El límite queda en aproximadamente 92 105
B/s. El benchmark midió 92 104 B/s.

Para líneas de 48 bytes:

```text
92105 B/s / 48 B = 1918,8 líneas/s
```

Ese es el límite físico. La cola puede absorber un exceso durante un rato, pero no puede sostenerlo.

## Prueba alrededor del límite

Cada ritmo se mantuvo durante dos segundos, sin esperar espacio antes de escribir:

| Ritmo solicitado | Producción | Intentadas | Aceptadas | Descartadas |
|---:|---:|---:|---:|---:|
| 1800 líneas/s | 86 400 B/s | 3600 | 3600 | 0 |
| 1900 líneas/s | 91 200 B/s | 3800 | 3800 | 0 |
| 1950 líneas/s | 93 600 B/s | 3900 | 3858 | 42 |
| 2000 líneas/s | 96 000 B/s | 4000 | 3859 | 141 |
| 2500 líneas/s | 120 000 B/s | 5000 | 3865 | 1135 |

A 1950 líneas/s la aplicación excede el enlace por unos 1496 B/s. Con una cola de 2048 bytes, el
descarte no empieza de inmediato:

```text
tiempo hasta llenar la cola = 2047 / (93600 - 92104) = 1,37 s
```

Una prueba corta podría dar la impresión de que 1950 líneas/s funciona. Al mantenerla, la cola se
llena y aparecen las pérdidas. Para uso normal conviene dejar margen y no diseñar al 99 % del
caudal. Con mensajes de 48 bytes, 1500 líneas/s deja un margen razonable.

## Qué pasa dentro de un `while (1)`

Esto es válido como prueba de saturación, pero no como estrategia de diagnóstico:

```c
while (1) {
    DBG_LINE("sigo vivo");
}
```

En la prueba extrema se hicieron 100 000 llamadas de 48 bytes sin esperar:

| Resultado | Valor |
|---|---:|
| Datos que la aplicación intentó producir | 4 800 000 B |
| Tiempo empleado en hacer las llamadas | 117 685 µs |
| Ritmo de producción solicitado | 40,8 MB/s |
| Líneas aceptadas | 256 |
| Líneas descartadas | 99 744 |
| Porcentaje descartado | 99,744 % |
| Tiempo hasta terminar de transmitir lo aceptado | 133 418 µs |

La función no se bloqueó. Cuando no había lugar, descartó la llamada completa y volvió. Por eso el
programa pudo intentar unas 850 000 llamadas por segundo, aunque la UART solo podía transportar unas
1919 líneas por segundo.

Si el objetivo es ver cada iteración, una UART no alcanza. Hay que reducir la cantidad de eventos,
guardar datos binarios en RAM o usar una interfaz de mayor velocidad.

## Qué pasa si se fuerza backpressure

Se puede esperar lugar antes de cada mensaje:

```c
while (debug_mejorado_libres() < sizeof linea - 1u) {
}
debug_mejorado_write(linea, sizeof linea - 1u);
```

En el banco se enviaron 5000 líneas, 240 000 bytes:

| Resultado | Valor |
|---|---:|
| Líneas descartadas | 0 |
| Tiempo total | 2 605 720 µs |
| Tiempo esperando lugar | 2 579 972 µs |
| Parte del tiempo ocupada esperando | 99,01 % |
| Caudal | 92 105 B/s |

No se perdió nada, pero el programa convirtió una salida asíncrona en una espera activa. El CPU no
estaba disponible para trabajo útil durante casi todo el ensayo.

El backpressure puede servir en una prueba controlada o durante un volcado final. No conviene en el
lazo principal de una aplicación con requisitos temporales.

## Tiempo de cada llamada

`debug_mejorado_write()` evita recorrer el mensaje con `strlen()` porque recibe el largo. Con la
cola vacía y DMA a 921600 se midió:

| Bytes entregados en una llamada | Tiempo hasta volver |
|---:|---:|
| 1 | 4,11 µs |
| 16 | 3,45 µs |
| 48 | 3,99 µs |
| 128 | 5,21 µs |
| 512 | 11,34 µs |
| 1024 | 20,34 µs |
| 2000 | 35,72 µs |

Los 5,84 µs medidos para `DBG_MSG()` con 48 bytes incluyen `strlen()` y la administración de esa
API. Si el largo ya es conocido, `debug_mejorado_write()` es más eficiente.

El tiempo de copia aumenta con el tamaño del mensaje. Además, el framework deshabilita brevemente
las interrupciones mientras reserva espacio y copia a la cola. Para 48 bytes la sección crítica es
corta. Copiar 2000 bytes introduce una pausa mucho más visible, aunque la transmisión posterior use
DMA.

Una llamada individual debe ser menor que `DEBUG_COLA_SIZE`. Con la cola predeterminada, el máximo
teórico es 2047 bytes y solo entra si hay suficiente espacio libre. Un mensaje de 2048 bytes o más se
descarta siempre.

## Dónde puede bloquear

| Situación | Comportamiento |
|---|---|
| Backend por bloques | Cada escritura espera a que la UART acepte los bloques. El CPU queda ocupado. |
| Backend IRQ o DMA, con lugar | Copia a la cola y vuelve. Deshabilita interrupciones brevemente durante la copia. |
| Backend IRQ o DMA, sin lugar | No espera. Descarta la llamada completa y vuelve. |
| `debug_mejorado_flush()` | Espera activamente hasta que salga el último bit. |
| Lazo esperando `debug_mejorado_libres()` | El framework no bloquea, pero la aplicación sí lo hace. |

Con 48 bytes, el backend por bloques tardó 2720 µs a 115200 y 350 µs a 921600. En una transmisión
sostenida mantiene al CPU esperando prácticamente al ritmo del cable.

`flush()` tiene un costo variable. Con la cola llena de 2047 bytes, solamente vaciarla necesita
cerca de 22 ms a 921600 o 178 ms a 115200. No hay un timeout interno: si la interrupción necesaria
no puede ejecutarse, `flush()` no termina.

Los tiempos de retorno no incluyen el trabajo posterior de las ISR. Con mensajes espaciados de 48
bytes, DMA puede completar un tramo por mensaje, cerca de 1919 interrupciones por segundo al máximo
del enlace. Si hay varios mensajes contiguos en la cola, puede transferirlos juntos y generar menos
interrupciones. El backend IRQ carga hasta 16 bytes por atención, por lo que ronda 5757
interrupciones por segundo a caudal máximo.

No se midió un único porcentaje de CPU para las ISR porque depende de cómo se agrupen los mensajes y
de las prioridades de la aplicación. En un sistema con plazos estrictos hay que medir ese caso real,
no deducirlo solamente del tiempo que tarda `DBG_MSG()` en volver.

## RAM, Flash y stack

### Memoria estática

Con el mismo benchmark básico:

| Backend | Flash | RAM informada por el linker | Ubicación de la cola |
|---|---:|---:|---|
| Bloques | 3604 B | 2152 B en RAM principal | no tiene cola |
| IRQ | 4024 B | 4208 B en RAM principal | 2048 B en RAM principal |
| DMA | 4132 B | 2168 B en RAM principal + 2048 B en AHB SRAM | AHB SRAM |

La cifra de RAM incluye la reserva de aproximadamente 2 KiB para stack y heap de la plantilla. La
cola no está en el stack. Es un arreglo estático y permanece asignado durante toda la ejecución.

Una cola más grande reduce la probabilidad de perder una ráfaga, pero consume RAM y no modifica los
92 104 B/s del enlace.

### Uso de stack

Compilando con `-fstack-usage`, el backend DMA dio estos máximos estáticos para las funciones del
framework:

| Camino | Stack aproximado del framework |
|---|---:|
| `DBG_MSG()` o `debug_mejorado_write()` durante la copia | 72 B |
| `DBG_DEC32()` o `DBG_HEX32()` durante la copia | 96 B |
| Handler de DMA o UART | 8 B de C + 32 B apilados por el Cortex-M3 |

Como estimación conservadora, una salida numérica interrumpida por el handler puede agregar cerca de
112 B sobre el stack que ya estuviera usando la aplicación. Los valores dependen del compilador, la
optimización y cualquier cambio del código. Conviene volver a generar los archivos `.su` en el
proyecto final.

El tamaño del mensaje no aumenta el stack del framework porque se copia desde el buffer recibido.
El problema aparece si la aplicación declara ese buffer como variable local:

```c
void informar(void)
{
    char linea[1024];  /* Estos 1024 bytes sí están en el stack. */
    /* ... */
}
```

Para buffers grandes o permanentes, usá memoria estática y revisá que su uso sea compatible con el
resto de la aplicación.

## Uso recomendado

### Inicializar una vez

Inicializá el framework después de configurar el clock y antes de producir mensajes:

```c
int main(void)
{
    debug_mejorado_init();
    DBG_LINE("inicio");

    while (1) {
        /* aplicación */
    }
}
```

La implementación configura TXD0 en P0.2 y deja P0.3 sin modificar. DMA usa el canal 0 y la
solicitud 8, compartida con MAT0.0.

### Imprimir cambios, no vueltas del lazo

Un mensaje suele aportar información cuando cambia algo:

```c
if (estado != estado_anterior) {
    DBG_MSG("estado=");
    DBG_DEC32(estado);
    DBG_MSG("\r\n");
    estado_anterior = estado;
}
```

También se puede diezmar una medición:

```c
if (++muestras_hasta_log >= 100u) {
    muestras_hasta_log = 0u;
    DBG_MSG("adc=");
    DBG_DEC32(valor_adc);
    DBG_MSG("\r\n");
}
```

Primero calculá el presupuesto:

```text
bytes por segundo = frecuencia de mensajes * bytes por mensaje
```

Intentá quedar por debajo del 70 al 80 % del enlace si además esperás ráfagas.

Como referencia para líneas de 48 bytes:

| Baudrate | Máximo físico aproximado | Zona de trabajo con margen |
|---:|---:|---:|
| 115200 | 240 líneas/s | hasta unas 180 líneas/s |
| 921600 | 1919 líneas/s | hasta unas 1500 líneas/s |

### Entregar una línea completa

La cola garantiza atomicidad por llamada, no por secuencia de macros. En este ejemplo hay tres
llamadas independientes:

```c
DBG_MSG("valor=");
DBG_DEC32(valor);
DBG_MSG("\r\n");
```

Si queda poco espacio, podría entrar una parte y descartarse otra. Cuando la línea debe aparecer
completa, armala en un buffer y entregala una sola vez:

```c
static char linea[64];
uint32_t largo = construir_linea(linea, sizeof linea, valor);

if (largo != 0u) {
    debug_mejorado_write(linea, largo);
}
```

`construir_linea()` representa el formateador que use la aplicación. Puede ser una rutina propia o
`snprintf()`, teniendo presente el costo de Flash, stack y tiempo de `printf`.

Si el mensaje ya está en un arreglo y su largo se conoce al compilar, evitá `strlen()`:

```c
static const char mensaje[] = "ADC fuera de rango\r\n";
debug_mejorado_write(mensaje, sizeof mensaje - 1u);
```

### Vigilar las pérdidas

Consultá el contador periódicamente:

```c
uint32_t perdidos_anteriores;

void revisar_debug(void)
{
    uint32_t actuales = debug_mejorado_perdidos();
    if (actuales != perdidos_anteriores) {
        perdidos_anteriores = actuales;
        /* Encender un LED, guardar el valor o informarlo a baja frecuencia. */
    }
}
```

El contador está expresado en bytes descartados, no en cantidad de mensajes. Que sea cero confirma
que la cola interna no descartó datos. No confirma que el adaptador, el driver USB o la terminal de
la PC hayan recibido todo.

Para una captura que deba ser verificable, agregá número de secuencia y, si hace falta, checksum.

### Poder desactivarlo

Usá una macro propia para retirar los logs de la versión final:

```c
#ifndef LOG_ACTIVO
  #define LOG_ACTIVO 0
#endif

#if LOG_ACTIVO
  #define LOG_LINE(s)  DBG_LINE(s)
  #define LOG_DEC(n)   DBG_DEC32(n)
#else
  #define LOG_LINE(s)  ((void) 0)
  #define LOG_DEC(n)   ((void) 0)
#endif
```

No pases expresiones con efectos secundarios a un log:

```c
LOG_DEC(contador);          /* Solo observa. */
contador++;                 /* Ocurre con logs activos o desactivados. */
```

Si el incremento estuviera dentro de una macro que desaparece al compilar, el programa cambiaría
su comportamiento al desactivar la depuración.

### Usar `flush()` solamente en límites controlados

`debug_mejorado_flush()` espera que se vacíen la cola, el DMA, la FIFO y el registro de
desplazamiento. Usalo antes de:

- Reiniciar la placa de manera voluntaria.
- Entrar en un modo de bajo consumo que apague la UART o el DMA.
- Terminar un benchmark donde importa medir hasta el último bit.

No lo llames después de cada mensaje. Tampoco lo llames desde una ISR ni con interrupciones
deshabilitadas. Los backends IRQ y DMA necesitan sus interrupciones para completar la transmisión.

## Cómo no usarlo

Evitá estas situaciones:

1. **Imprimir en cada vuelta de un lazo sin límite.** La cola se llena y se descarta casi todo.
2. **Usar `flush()` después de cada `DBG_MSG()`.** Convierte la salida en bloqueante.
3. **Esperar espacio en un lazo cerrado.** Evita pérdidas, pero puede ocupar prácticamente todo el
   tiempo del CPU.
4. **Confundir retorno con entrega.** Que la llamada haya vuelto no significa que la PC ya recibió
   el mensaje.
5. **Formar una línea crítica con varias llamadas.** Cerca de la saturación puede quedar
   incompleta.
6. **Mandar un mensaje tan grande como la cola.** Con 2048 bytes, una llamada de 2048 bytes se
   descarta siempre.
7. **Imprimir desde interrupciones rápidas.** Aumenta latencia, jitter y uso de stack. Guardá el
   evento y procesalo desde el `main`.
8. **Usarlo para informar un HardFault y asumir que DMA seguirá funcionando.** En un fault puede no
   ejecutarse la interrupción del DMA. Para datos críticos conviene guardar un registro de crash en
   RAM retenida o usar una salida de emergencia por polling.
9. **Agrandar la cola para corregir un exceso permanente.** Solo posterga la pérdida.
10. **Compartir recursos sin integrarlos.** El backend DMA reserva GPDMA canal 0, `DMA_IRQHandler`,
    UART0 y su solicitud. IRQ reserva `UART0_IRQHandler`.
11. **Confiar únicamente en `debug_mejorado_perdidos()`.** Ese contador no ve pérdidas en el cable,
    el CP2102 o la PC.

## Qué mostró la captura con CP2102

La prueba de estrés produjo 1 182 069 bytes por ejecución. Dos capturas consecutivas recibieron
exactamente:

- 24 238 líneas normales de 48 bytes.
- Cinco mensajes de cada tamaño: 1, 16, 48, 128, 512, 1024 y 2000 bytes.
- Cero líneas corruptas o truncadas.

Durante la puesta a punto apareció un prefijo inesperado de 1611 bytes. Al repetir la prueba se vio
que no provenía de la cola: algunas secuencias `reset run` de OpenOCD dejaban ejecutar brevemente el
programa durante el manejo del reset y después lo iniciaban de nuevo. El prefijo contenía mensajes
válidos del comienzo y una línea cortada con `0xFF`, coherente con reiniciar la UART a mitad de una
transmisión.

La secuencia determinista fue mantener una sola sesión de OpenOCD, ejecutar `reset halt`, abrir y
vaciar la captura serie, y finalmente usar `resume`. Así se recibieron exactamente 1 182 069 bytes,
sin duplicados ni líneas inválidas. Este detalle no cambia el caudal del framework, pero sí importa
al verificar el arranque de una placa.

UART sin control de flujo no ofrece confirmación de recepción. El micro sabe qué entregó al
periférico, pero no sabe qué terminó guardando la PC.

## Checklist antes de usarlo

- ¿El mensaje tiene valor diagnóstico o solo repite que el lazo sigue ejecutándose?
- ¿Calculaste cuántos bytes por segundo vas a producir?
- ¿Dejaste margen respecto del caudal máximo?
- ¿Una línea importante se entrega en una sola llamada?
- ¿Revisás `debug_mejorado_perdidos()`?
- ¿Evitaste `flush()` en el camino normal?
- ¿La aplicación tiene libres UART0, P0.2, GPDMA canal 0 y la solicitud 8?
- ¿El stack tiene margen para la aplicación, el framework y las interrupciones anidadas?
- ¿Necesitás solamente logs o en realidad te convienen breakpoints, watchpoints, RTT o una captura
  binaria en RAM?

El framework es una herramienta de observación, no un sistema de adquisición de datos. Funciona
muy bien para eventos, estados, errores y mediciones diezmadas. Deja de ser la herramienta adecuada
cuando se intenta registrar cada evento de alta frecuencia sin reducir ni estructurar la
información.

## Reproducir la prueba de estrés

El programa usado está en [`stress.c`](./stress.c). Para compilarlo desde `plantilla/`:

```bash
make -B USE_CMSIS=1 \
  BUILD_DIR=/tmp/lpc1769-debug-mejorado-stress \
  SRC="../curso/ejemplos/uart/debug_framework_mejorado/stress.c \
       ../curso/ejemplos/uart/debug_framework_mejorado/debug_frmwrk_mejorado.c \
       src/syscalls.c startup/startup_lpc1769.c" \
  EXTRA_CFLAGS="-DDEBUG_BACKEND=DEBUG_BACKEND_DMA \
                -DDEBUG_BAUD=921600 -fstack-usage" \
  flash
```

Los resultados quedan en `resultados_stress`. `terminado == 0x51AE55ED` confirma que el ensayo
finalizó. Los archivos `.su` generados por `-fstack-usage` muestran el stack estático de cada
función.

Para verificar la salida con una terminal o un script, abrí `/dev/ttyUSB0` a 921600 antes de dejar
correr el programa. `make flash` reinicia la placa inmediatamente, por lo que un receptor abierto
después puede perder los primeros bytes.

Para capturar desde el primer byte, mantené la misma sesión de OpenOCD durante toda la secuencia:

1. Ejecutá `reset halt`.
2. Abrí el CP2102 y descartá cualquier byte anterior.
3. Ejecutá `resume`, sin agregar otro reset.

Separar estas acciones en distintas instancias de OpenOCD o usar `reset run` puede introducir una
ejecución parcial durante la secuencia de reset de esta placa.
