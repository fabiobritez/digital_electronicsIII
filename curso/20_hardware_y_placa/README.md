# Módulo 20: Hardware y placa

Todo el curso habla de software, pero el micro vive en una **placa**, conectado a componentes reales,
con limitaciones físicas. Este módulo cubre lo que en general se da por sabido y casi nunca lo está: de
qué está hecha tu placa, cómo no quemar el micro, y cómo medir lo que pasa de verdad en los pines.

Es de lo más práctico del curso: la mayoría de las frustraciones iniciales ("conecté un motor y se
reseteó", "el botón hace cualquier cosa", "no sé qué pin es P0.22") se evitan con lo de acá.

## Recorrido

1. [01 - Tu placa por dentro](./01-tu-placa-por-dentro.md)
   Del nombre `P0.22` al pin físico: cómo leer el esquemático y la disposición de pines, y cómo está
   cableado un LED o un botón.
2. [02 - Electrónica mínima para no romper el micro](./02-electronica-minima.md)
   Niveles de 3.3 V, corriente máxima por pin, resistencias de LED, pull-up/down, capacitores de
   desacople.
3. [03 - Instrumentos de medición](./03-instrumentos-de-medicion.md)
   Multímetro, osciloscopio y analizador lógico: cómo ver qué pasa realmente en tus señales.

## Cuándo leerlo
Cuanto antes mejor: idealmente junto con el [módulo 5 (GPIO)](../05_gpio/), la primera vez que
conectás algo físico. La parte de instrumentos complementa el [módulo 12 (debug)](../12_debug/).

## Nota
Los detalles exactos (qué pin tiene el LED, cuáles son tolerantes a 5 V) dependen de **tu placa**.
Acá van los conceptos y los valores típicos del LPC1769; siempre confirmá con el **esquemático de tu
placa** y el **datasheet del LPC1769**.

---

**Anterior:** [19 - PWM](../19_pwm/) · **Volver al** [índice del curso](../README.md)
