# Cómo pensar el protocolo de dos hilos

Este ejercicio se vuelve difícil si se empieza escribiendo registros. Primero hay que convertir el
diagrama temporal en una secuencia de decisiones pequeñas. El código aparece al final de ese proceso.

## 1. Separar las responsabilidades

Hay tres problemas distintos:

1. **Tiempo:** determinar cuándo ocurre cada mitad del ciclo de reloj.
2. **Movimiento de bits:** sacar o incorporar un bit sin perder su posición.
3. **Secuencia:** recordar en qué parte de la trama se encuentra cada placa.

El timer resuelve el primero, los desplazamientos resuelven el segundo y una máquina de estados
resuelve el tercero.

## 2. Pensar en medios períodos

Cada bit tiene una fase baja y una fase alta. Conviene que la interrupción del controlador ocurra
cada medio período:

| Fase de `CLK` | Trabajo del controlador | Trabajo del periférico |
|---|---|---|
| Bajo | Colocar o liberar `DATA` | No tomar la muestra |
| Flanco ascendente | Mantener `DATA` estable | Leer `DATA` |
| Alto | No modificar el dato | Procesar la muestra |
| Flanco descendente | Preparar el siguiente ciclo | Preparar o retirar la confirmación |

Preguntas que deberían poder responderse antes de programar:

- ¿Qué valor tiene `DATA` antes del primer flanco ascendente?
- ¿En qué flanco se incrementa el contador de bits?
- ¿Cuándo deja el controlador de manejar `DATA`?
- ¿Cuándo empieza y cuándo termina de manejarla el periférico?

## 3. Calcular el timer

Con `CCLK = 100 MHz` y `PCLK_Timer0 = CCLK/4`, el timer recibe 25 MHz. Se puede elegir un prescaler
para que el contador avance una vez por microsegundo:

```text
PR = 25 - 1 = 24
```

Para interrumpir cada 500 µs con reset por match:

```text
T = (MR0 + 1) × (PR + 1) / PCLK
MR0 = 500 - 1 = 499
```

El periférico puede configurar Timer1 de la misma manera, pero con un match cada 1000 µs para obtener
una base de tiempo de 1 ms.

No hay que copiar los números sin comprobarlos: ¿qué ocurriría si `PCLK` estuviera configurado como
`CCLK` en lugar de `CCLK/4`?

## 4. Representar los bits

Una forma sencilla de transmitir consiste en juntar comando y dato en una palabra de 16 bits:

```text
trama = (comando << 8) | dato
```

Si se transmite primero el bit más significativo, el bit de posición `i` puede obtenerse pensando en
qué desplazamiento hace falta para llevarlo hasta la posición cero.

Para recibir, no hacen falta dieciséis variables. Un registro se desplaza y el bit nuevo entra por el
extremo derecho:

```text
nuevo_valor = (valor_anterior << 1) | bit_recibido
```

Conviene simular esta expresión a mano con cuatro bits antes de usarla con ocho.

## 5. Diseñar el controlador

Escribir en una hoja una fila por cada acción que debe ocurrir:

```text
reposo
generar inicio
bajar CLK y preparar un bit
subir CLK
bajar CLK
...
liberar DATA
subir CLK y leer confirmación
recuperar DATA
generar fin
reposo
```

Después se agrupan acciones repetidas. Cada grupo que necesita recordar que ocurrió se transforma en
un estado. El contador de bits permite usar los mismos estados para los dieciséis bits.

La función llamada desde `main()` no debería enviar la trama. Solo debería:

- comprobar que el enlace esté libre;
- guardar comando y dato;
- generar el inicio;
- colocar la máquina en su primer estado activo.

La interrupción periódica hace avanzar una sola fase cada vez.

## 6. Diseñar el periférico

El periférico no genera el reloj. Reacciona a tres clases de evento:

- flanco de `DATA` con `CLK` alto: posible inicio o fin;
- flanco ascendente de `CLK`: momento de muestrear;
- flanco descendente de `CLK`: momento seguro para cambiar la confirmación.

Los GPIO de los puertos 0 y 2 comparten el vector `EINT3_IRQHandler`. Dentro del handler hay que leer
las banderas para distinguir:

- qué pin produjo la interrupción;
- si fue un flanco ascendente o descendente.

El nombre del handler no significa que se esté usando el pin dedicado `EINT3`.

Una forma de dividir la recepción es:

- esperar un inicio;
- acumular ocho bits de comando;
- acumular ocho bits de dato;
- calcular la respuesta;
- colocar la respuesta durante `CLK` bajo;
- retirarla durante otro nivel bajo;
- esperar el fin.

Los nombres exactos y la cantidad de estados son una decisión de diseño. Lo importante es que para
cada estado esté definido qué eventos se aceptan y qué transición producen.

## 7. Resolver el cambio de dueño de DATA

No se debe escribir un uno en la salida. Las únicas primitivas necesarias son:

```text
DATA a cero  -> escribir cero y configurar el pin como salida
DATA a uno   -> configurar el pin como entrada y dejar actuar al pull-up
```

Antes de implementar la confirmación, completar esta tabla:

| Parte | Controlador | Periférico |
|---|---|---|
| Inicio | ¿maneja o libera? | ¿maneja o libera? |
| Comando y dato | ¿maneja o libera? | ¿maneja o libera? |
| Confirmación | ¿maneja o libera? | ¿maneja o libera? |
| Fin | ¿maneja o libera? | ¿maneja o libera? |

Si en alguna fila ambos pueden intentar imponer niveles opuestos, el diseño todavía no está listo.

## 8. Tratar una trama como candidata

Recibir dieciséis bits no debería modificar inmediatamente las salidas. Todavía faltan dos cosas:

- comprobar que el comando y el dato sean válidos;
- recibir un fin correcto.

Por eso conviene guardar el resultado como una **trama candidata**. El periférico comunica aceptación
o rechazo, pero solo publica la orden a `main()` cuando detecta el fin esperado.

Así una trama interrumpida o mal terminada no produce una acción parcial.

## 9. Timeout sin bloquear

El timer del periférico incrementa un contador de milisegundos. Cada evento válido guarda el instante:

```text
ultima_actividad = milisegundos
```

El programa principal puede comprobar:

```text
milisegundos - ultima_actividad >= TIMEOUT
```

La resta sin signo continúa funcionando aunque el contador de tiempo dé la vuelta. Ante timeout hay
que descartar los bytes parciales, liberar `DATA` y volver a esperar un inicio.

## 10. Implementar por etapas

No conviene probar todo junto:

1. Verificar con osciloscopio que `CLK` tenga 500 µs bajo y 500 µs alto.
2. Transmitir continuamente `0xAAAA` y comprobar sus bits.
3. Detectar inicio en el periférico.
4. Recibir un solo byte y observarlo con el debugger.
5. Recibir los dos bytes.
6. Agregar la confirmación.
7. Agregar el fin.
8. Agregar los comandos.
9. Cortar un cable a mitad de trama y verificar el timeout.

En cada etapa debe existir una observación concreta que permita decidir si funciona. “El programa
parece correcto” no reemplaza una medición.

## Errores para buscar deliberadamente

- Usar `MR0 = 500` sin considerar que el período contiene `MR0 + 1` ticks.
- Olvidar limpiar `IR` o `IO2IntClr`.
- Cambiar `DATA` mientras `CLK` está alto durante un bit normal.
- Transmitir LSB primero cuando el receptor espera MSB primero.
- No declarar `volatile` una variable compartida con una ISR.
- Mantener `DATA` como salida durante la confirmación.
- Ejecutar el comando antes de recibir el fin.
- Reiniciar solo contadores ante timeout, pero olvidar liberar físicamente `DATA`.
- Confundir P2.11 configurado como GPIO con su función alternativa `EINT1`.

