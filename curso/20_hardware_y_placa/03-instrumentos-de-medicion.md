# Instrumentos de medición

El debugger (módulo 12) te muestra qué hace el **software**. Pero a veces el problema está en el
**hardware**: ¿la señal sale de verdad? ¿a qué voltaje? ¿con qué forma? Para eso están los
instrumentos. Saber cuál usar y para qué resuelve problemas que por software son invisibles.

## El multímetro (el básico imprescindible)

Mide valores **estáticos** o lentos. Es barato y el primero que tenés que tener. Usos:

- **Tensión (V):** ¿la placa tiene sus 3.3 V? ¿un pin está en alto (≈3.3 V) o bajo (≈0 V)? ¿la batería
  está cargada? Se mide **en paralelo** (las puntas entre el punto y GND). Ojo: una entrada con
  **pull-up interno** en reposo mide **~2.3–2.6 V**, no 3.3 V: es normal (página anterior).
- **Continuidad:** ¿este cable está realmente conectado? ¿hay un cortocircuito entre dos pines? El modo
  continuidad **pita** si hay conexión. Sirve muchísimo para verificar cableado en protoboard.
- **Resistencia (Ω):** medir una resistencia, verificar un pull-up.
- **Corriente (A):** cuánto consume el circuito (se mide **en serie**, cortando el camino, más
  engorroso).

> Primer reflejo cuando "no anda nada": multímetro en VCC. Muchísimos problemas son "no llegaba
> alimentación" o "GND mal conectado". Treinta segundos de medir antes que una hora de revisar código.

**Limitación:** el multímetro muestra un número, no **cómo cambia** la señal en el tiempo. Para una
señal que se mueve rápido (PWM, UART, un pulso), un multímetro solo te da un promedio o un valor que
"baila". Ahí entran los otros dos.

## El osciloscopio (ver la forma de la señal en el tiempo)

Dibuja un **gráfico de tensión vs tiempo**: ves la **forma** real de la señal. Es la herramienta para
señales analógicas y temporización. Usos:

- Ver una señal **PWM** (módulo 19): su período, su duty, si realmente sale.
- Ver una señal **analógica** (la salida de un DAC, un sensor, ruido en la alimentación).
- Medir **tiempos** con precisión: cuánto dura un pulso, la frecuencia de algo, el *jitter*.
- Detectar **glitches** y ruido que un multímetro nunca mostraría.

Conceptos al usarlo: la **base de tiempo** (cuántos µs/ms por división horizontal), la **escala
vertical** (V por división), y el **trigger** (el evento que "congela" la pantalla para ver una señal
repetitiva quieta). Confirmá siempre que la **masa de la sonda** vaya al GND del circuito.

## El analizador lógico (el mejor amigo de la comunicación)

Captura **muchas señales digitales a la vez** y, lo más útil, **decodifica protocolos**. Para depurar
**UART, I2C, SPI** (módulos 9, 13, 14) es la herramienta definitiva. Uno barato (tipo "Saleae clone",
unos pocos dólares) resuelve el **80% de los problemas de comunicación**.

¿Por qué es tan útil? Cuando tu I2C "no anda", el analizador te muestra **exactamente** qué bytes
salieron por SDA/SCL: ¿se mandó la dirección correcta? ¿el esclavo respondió ACK o NACK? ¿el baudrate
es el que creés? Eso, adivinando por software, son horas; con el analizador, segundos.

```
SCL  ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐
SDA  ──┐ START  [ 0x48 + W ] ACK  [ 0x00 ] ACK ...   <- el analizador te decodifica esto
```

- **vs osciloscopio:** el osciloscopio muestra la **forma analógica** (voltajes, ruido, tiempos finos)
  de pocas señales; el analizador lógico muestra **muchos** canales como **unos y ceros** y **decodifica
  el protocolo**. Para comunicación digital, el analizador; para señales analógicas y timing fino, el
  osciloscopio.

## Cuál usar según el síntoma

| Síntoma | Instrumento |
|---------|-------------|
| "No prende nada / se resetea" | **multímetro**: ¿hay 3.3 V? ¿GND conectado? |
| "El cable / la conexión, ¿está bien?" | **multímetro** (continuidad) |
| "El PWM / la señal analógica, ¿sale bien?" | **osciloscopio** |
| "El I2C / SPI / UART no se comunica" | **analizador lógico** (decodifica las tramas) |
| "El programa hace algo raro" | **debugger** (módulo 12): ese es software |

## Sin instrumentos: el LED y la UART

Si no tenés ninguno (es lo habitual al empezar), tus instrumentos "de pobre" son los del módulo 12: un
**LED** para ver si el código llegó a un punto, y la **UART** para imprimir valores. No reemplazan a un
analizador para depurar I2C, pero resuelven mucho. Aun así, un **multímetro** es tan barato y tan útil
que vale la pena tener uno sí o sí.

## Lo que te llevás
- **Multímetro:** lo primero, para alimentación, continuidad y voltajes estáticos.
- **Osciloscopio:** para ver la forma y el timing de señales (PWM, analógicas).
- **Analizador lógico:** para depurar comunicación digital (UART/I2C/SPI), el que más tiempo ahorra.
- Medir **antes** de desesperar: muchos "bugs de software" son en realidad de hardware, y un
  instrumento lo revela en segundos.

---

**Anterior:** [02 - Electrónica mínima](./02-electronica-minima.md) ·
**Volver al** [índice del curso](../README.md)
