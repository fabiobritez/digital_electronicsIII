# Solución de referencia

La solución usa dos placas y dos programas independientes:

- [`controlador.c`](./controlador.c): genera la trama con Timer0.
- [`periferico.c`](./periferico.c): recibe por interrupciones GPIO y usa Timer1 como base de tiempo.

Los programas usan `LPC17xx.h` para nombrar registros, pero no usan los drivers de comunicación del
fabricante.

## Cableado

1. Unir P2.11 del controlador con P2.11 del periférico.
2. Unir P2.12 del controlador con P2.12 del periférico.
3. Unir las masas.
4. Colocar un resistor de 4,7 kΩ entre `DATA` y 3,3 V.

No conectar dos placas alimentadas con tensiones diferentes.

## Máquina del controlador

`protocolo_iniciar()` arma la palabra de 16 bits y genera la condición de inicio. Desde ese momento,
cada interrupción de Timer0 avanza medio período:

```text
REPOSO
  -> INICIO
  -> BIT_BAJO <-> BIT_ALTO       (16 veces)
  -> CONFIRMACION_BAJO
  -> CONFIRMACION_ALTO
  -> RECUPERAR_DATA
  -> FIN_CLK_ALTO
  -> REPOSO
```

En `BIT_BAJO` se sube el reloj. En `BIT_ALTO` se baja y se prepara el bit siguiente. Los nombres
describen el nivel que ya estaba establecido al entrar al estado.

El `main()` de demostración envía una operación cada 1,5 segundos. Alterna escrituras en las cuatro
salidas y cambios en el período del LED del periférico. P0.22 del controlador se enciende cuando la
última trama fue aceptada.

## Máquina del periférico

Los flancos de P2.11 y P2.12 entran por `EINT3_IRQHandler`, porque las interrupciones GPIO de los
puertos 0 y 2 comparten ese vector.

```text
ESPERAR_INICIO
  -> RECIBIR_COMANDO              (8 flancos ascendentes)
  -> RECIBIR_DATO                 (8 flancos ascendentes)
  -> ESPERAR_BAJO_CONFIRMACION
  -> CONFIRMACION_ACTIVA
  -> ESPERAR_RETIRO_CONFIRMACION
  -> ESPERAR_FIN
  -> ESPERAR_INICIO
```

El periférico solo maneja `DATA` durante la confirmación. Para confirmar lleva la línea a cero; para
rechazar deja que el resistor la mantenga alta.

El comando se publica a `main()` al recibir el fin, no al recibir el último bit. Timer1 genera un tic
de 1 ms para el timeout y para temporizar P0.22.

## Compilación

Cada archivo contiene su propio `main()`: no deben compilarse juntos. Copiar uno por vez como
`src/main.c` en la plantilla del repositorio y compilar con:

```bash
make USE_CMSIS=1
```

Se asume `CCLK = 100 MHz` y se configura explícitamente `PCLK_Timer0` o `PCLK_Timer1` como
`CCLK/4 = 25 MHz`.

## Pruebas esperadas

Con el programa de demostración deberían observarse:

- `CLK`: pulsos de 1 ms solamente durante las transferencias;
- `DATA`: inicio, 16 bits MSB primero, confirmación y fin;
- P0.18 a P0.21 del periférico: patrón binario que avanza;
- P0.22 del periférico: alternancia primero cada 250 ms y luego cada 1 s;
- P0.22 del controlador: encendido después de una confirmación válida.

Para probar el rechazo se puede agregar temporalmente `{0x99, 0x55}` a la tabla de demostración del
controlador. El periférico debe dejar `DATA` alta durante la confirmación y no modificar sus salidas.

