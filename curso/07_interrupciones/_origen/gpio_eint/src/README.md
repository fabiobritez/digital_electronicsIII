
En este ejemplo simple, se configuraron tres pines del **puerto 0** como entradas con interrupciones habilitadas.
En la ISR, se puede ver que en función del pin que genera la interrupción **se enciende un LED de cada color**. Los pines que interrumpen son los siguientes:

* **P0.9**
  Entrada digital con **pull-up**, **interrumpe por flanco descendente**.
  Al colocar el pin a **3.3V** no interrumpe; al poner el pin a **GND** se produce la interrupción y **enciende el LED rojo**.

* **P0.10**
  Entrada digital con **pull-down**, **interrumpe por flanco ascendente**.
  Al colocar el pin a **GND** no interrumpe; al poner el pin a **3.3V** se produce la interrupción y **enciende el LED verde**.

* **P0.11**
  Entrada digital con **pull-up**, **interrumpe por ambos flancos**.
  Inicialmente al colocar el pin a **3.3V** no interrumpe debido a que ya se encontraba en estado alto por la pull-up.
  Al colocar el pin a **GND** se genera la interrupción por **flanco descendente**.
  Si se saca el pin de GND se producirá una nueva interrupción, ahora por **flanco ascendente** debido a la pull-up. **Se enciende el LED azul**.

> Una buena forma de analizar este código es mediante un proceso de *debug* colocando un *breakpoint* en la ISR.
