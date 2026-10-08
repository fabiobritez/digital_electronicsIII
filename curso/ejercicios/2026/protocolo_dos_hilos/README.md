# Trabajo práctico: protocolo digital de dos hilos

## Objetivo

Diseñar e implementar un enlace digital síncrono entre dos placas LPC1769:

- un **controlador**, que inicia todas las transferencias;
- un **periférico remoto**, que recibe comandos y actúa sobre sus salidas.

No se permite usar un periférico de comunicación serie del microcontrolador. El protocolo debe
implementarse manualmente con GPIO, timers, interrupciones y una máquina de estados.

## Conexión

Las placas se conectan con dos señales y masa común:

| Señal | Controlador | Periférico | Función |
|---|---|---|---|
| `CLK` | P2.11, salida | P2.11, entrada | Reloj generado por el controlador |
| `DATA` | P2.12, bidireccional | P2.12, bidireccional | Datos y confirmación |
| `GND` | GND | GND | Referencia común |

La línea `DATA` debe tener un resistor externo de **4,7 kΩ a 3,3 V**.

```text
                         3,3 V
                           |
                         4,7 kΩ
                           |
Controlador P2.12 ----------+---------- P2.12 Periférico
Controlador P2.11 --------------------- P2.11 Periférico
Controlador GND   --------------------- GND   Periférico
```

Ningún equipo puede forzar un nivel alto sobre `DATA`. Cada participante solamente puede:

- llevar `DATA` a cero configurando el pin como salida baja;
- producir un uno liberando el pin, es decir, configurándolo como entrada.

El resistor lleva la línea a uno cuando ambos equipos la liberan. Esta regla evita un cortocircuito
si los dos intentan usar la línea al mismo tiempo.

## Temporización

El controlador debe generar `CLK` con un período de **1 ms**:

```text
                 500 us             500 us
CLK        _______                  _______
                  |________________|

DATA       =======X==============================
                   cambia solamente con CLK bajo
                                      ^
                                      el receptor lee en el flanco ascendente
```

El reloj debe generarse mediante un timer en modo match con interrupción cada **500 µs**. No se
permiten demoras bloqueantes ni lazos usados para medir tiempo.

## Formato de la trama

Una transferencia contiene un comando de 8 bits, un dato de 8 bits y una confirmación:

```text
          INICIO       COMANDO          DATO          CONF.       FIN
                        8 bits          8 bits          1 bit

CLK       ------┐_┌-┐_┌-┐ ... ┌-┐_┌-┐_┌-┐ ... ┌-┐_┌-┐_┌----------
DATA      -----┐│ C7  C6 ... C0 │ D7  D6 ... D0 │ A │└-----------
               └┘
```

Las reglas son:

1. En reposo, `CLK = 1` y `DATA = 1`.
2. Hay **inicio** cuando `DATA` cambia de uno a cero mientras `CLK` permanece en uno.
3. El controlador envía primero el comando y después el dato.
4. Cada byte se transmite desde el bit más significativo al menos significativo.
5. El transmisor prepara cada bit mientras `CLK` está en cero.
6. El receptor toma cada bit en el flanco ascendente de `CLK`.
7. Después del último bit, el controlador libera `DATA` durante el siguiente nivel bajo de `CLK`.
8. El periférico responde durante el ciclo siguiente:
   - `DATA = 0`: comando aceptado;
   - `DATA = 1`: comando rechazado.
9. Después de leer la confirmación, el controlador recupera la línea y genera el fin.
10. Hay **fin** cuando `DATA` cambia de cero a uno mientras `CLK` permanece en uno.

Una transición de `DATA` con `CLK` bajo es solamente un cambio de bit: no es inicio ni fin.

## Comandos del periférico

| Comando | Dato válido | Acción |
|---|---|---|
| `0x10` | `0x00` a `0x0F` | Copiar los cuatro bits bajos en P0.18, P0.19, P0.20 y P0.21 |
| `0x20` | `0x01` a `0xC8` | Alternar P0.22 cada `dato × 10 ms` |

Cualquier otro comando o dato fuera de rango debe ser rechazado. Una trama rechazada no puede
modificar las salidas.

## Timeout y recuperación

Si transcurren más de **30 ms** entre el inicio y el siguiente evento válido del protocolo, el
periférico debe:

- descartar la trama incompleta;
- liberar `DATA`;
- volver al estado de reposo;
- quedar preparado para recibir una transferencia nueva.

No se acepta como recuperación reiniciar la placa.

## Requisitos de implementación

### Controlador

- Generar las dos fases del reloj con Timer0 e interrupciones.
- Transmitir sin `delay`, esperas activas ni ciclos temporizados por software.
- Cambiar la dirección de `DATA` en el momento correspondiente.
- Leer y guardar la confirmación del periférico.
- Dejar `main()` disponible para iniciar nuevas transferencias o realizar otras tareas.

### Periférico

- Detectar los flancos de `CLK` mediante interrupciones GPIO.
- Detectar inicio y fin observando los flancos de `DATA` y el nivel de `CLK`.
- Reconstruir comando y dato con registros de desplazamiento.
- Usar Timer1 para implementar una base de tiempo de 1 ms.
- Resolver el timeout sin esperas bloqueantes.
- Ejecutar el comando solamente después de recibir una trama completa y un fin válido.

### Para ambos programas

- Las variables compartidas entre `main()` y una ISR deben declararse `volatile`.
- Toda bandera de interrupción debe limpiarse correctamente.
- Las ISR deben ser breves y no pueden contener demoras.
- La lógica debe expresarse como una máquina de estados explícita.
- Se programará a nivel de registros usando los nombres definidos en `LPC17xx.h`.

## Entregables

Antes del código se debe entregar, hecho a mano:

1. Un diagrama temporal de una trama completa.
2. La máquina de estados del controlador.
3. La máquina de estados del periférico.
4. Una tabla que indique quién controla `DATA` en cada parte de la trama.
5. El cálculo de `PR` y `MR0` para ambos timers.
6. La expresión utilizada para insertar un bit en un byte en recepción.
7. La estrategia de recuperación frente a timeout.

Después se entregan los dos programas y una captura de analizador lógico u osciloscopio que muestre:

- inicio;
- los 16 bits;
- confirmación;
- fin;
- al menos una trama aceptada y una rechazada.

## Criterios de evaluación

| Aspecto | Peso |
|---|---:|
| Diagrama temporal y máquinas de estados | 25 % |
| Configuración correcta de GPIO, timer y NVIC | 20 % |
| Transmisión y recepción de los 16 bits | 20 % |
| Cambio de dueño de `DATA` y confirmación | 15 % |
| Timeout y recuperación | 10 % |
| Claridad, modularidad y prueba experimental | 10 % |

## Material de trabajo

- [Cómo pensarlo](./COMO_PENSARLO.md): guía para diseñar la solución sin comenzar por el código.
- La carpeta `solucion/` debería consultarse solamente después de intentar resolver el ejercicio.

