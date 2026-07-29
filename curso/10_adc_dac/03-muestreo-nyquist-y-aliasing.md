# Muestreo, Nyquist y aliasing

Las páginas anteriores te enseñaron a **leer** el ADC. Esta responde una pregunta distinta y más
profunda: **¿cada cuánto** conviene leerlo? La respuesta no es "lo más rápido posible" ni "lo que me
quede cómodo": hay una física detrás. Ignorarla hace que midas una señal y obtengas otra, sin ningún
error en el código.

## Muestrear es tomar fotos de una señal

Una señal analógica (la salida de un micrófono, un sensor de presión, un potenciómetro que alguien
mueve) es **continua**: existe en todo instante. El ADC no puede guardar infinitos valores, así que
toma **muestras**: una foto del valor cada cierto tiempo. La **frecuencia de muestreo** `fs` es cuántas
fotos por segundo tomás (en Hz o muestras/s).

```
señal real  ─╮   ╭─╮   ╭─╮   ╭─        muestras (•): el ADC solo "ve" estos puntos
             ╰─╯   ╰─╯   ╰─╯
              •  •  •  •  •  •  •       y entre medio, asume una línea recta
```

Si tomás muchas muestras por ciclo de la señal, las reconstruís bien. Si tomás pocas, te perdés lo que
pasó entre foto y foto, y ahí empieza el problema.

## El teorema de Nyquist

> Para capturar sin ambigüedad una señal cuya frecuencia máxima es `f_max`, hay que muestrear a más del
> **doble**: `fs > 2 · f_max`.

Ese límite, `fs / 2`, se llama **frecuencia de Nyquist**. Es la frecuencia más alta que tu sistema de
muestreo puede representar. Todo lo que en la señal real esté **por encima** de `fs/2` no solo se
pierde: se **disfraza** de otra frecuencia más baja y te ensucia la medición. Eso es el aliasing.

Ejemplos concretos:
- Querés medir **audio de voz** (hasta ~3.4 kHz en telefonía): necesitás `fs > 6.8 kHz`. Por eso el
  teléfono muestrea a 8 kHz.
- Querés medir una señal que **no pasa de 100 Hz** (un sensor de temperatura, la posición de un
  potenciómetro movido a mano): con `fs = 1 kHz` (una muestra por ms) te sobra.

## Aliasing: la frecuencia que no estaba

El aliasing es el fenómeno más contraintuitivo y por eso el que más cuesta detectar. Una señal de
frecuencia **mayor** a `fs/2`, al muestrearla, aparece como una señal de frecuencia **menor** que nunca
existió. El ejemplo clásico es visual: en las películas, las ruedas de un auto a veces parecen girar al
revés. La cámara (24 fotogramas/s) está submuestreando el giro de la rueda; el "giro hacia atrás" es un
alias.

```
Señal real de 900 Hz, muestreada a fs = 1000 Hz (Nyquist = 500 Hz):
las muestras caen de tal forma que "dibujan" una señal de 100 Hz que no existe.

real 900 Hz:  /\  /\  /\  /\  /\        (sube y baja rapidísimo)
muestras:     •      •      •      •     (caen casi en el mismo punto del ciclo)
lo que ves:   ╲___________╱             (una onda lenta, fantasma, de 100 Hz)
```

Una vez que el alias entró en tus muestras, **no hay software que lo saque**: es indistinguible de una
señal real de esa frecuencia. Por eso el aliasing se combate **antes** del ADC, no después.

## Cómo evitarlo

1. **Muestreá lo suficientemente rápido.** Si conocés `f_max` de tu señal, poné `fs > 2·f_max`. En la
   práctica se usa un margen: `fs = 3` a `5 · f_max` para reconstruir cómodo y dejar lugar al filtro.

2. **Filtro antialiasing analógico.** Si tu señal **puede** contener frecuencias altas o ruido por
   encima de `fs/2` (casi siempre las contiene: ruido eléctrico, interferencia de 50 Hz de la red y sus
   armónicos), poné un **filtro pasabajos analógico antes del pin de ADC** que las corte. Un simple RC
   (una resistencia y un capacitor) ya ayuda muchísimo. La clave: tiene que ser **analógico**, físico,
   porque tiene que actuar *antes* de muestrear. Un filtro digital llega tarde, el alias ya está adentro.

3. **Sobremuestrear y promediar.** Tomar varias muestras y promediarlas reduce el ruido aleatorio y
   mejora la resolución efectiva. No reemplaza al filtro antialiasing, pero es gratis y ayuda.

## Aplicado al LPC1769

El ADC del LPC1769 hace hasta ~200 ksps (200.000 muestras por segundo) repartidas entre los canales
que uses. Para controlar el tiempo entre muestras tenés tres caminos, de menos a más prolijo:

- **Por software con SysTick:** disparás una conversión cada N ms en el `SysTick_Handler`. Simple, pero
  el *jitter* (variación del instante exacto) depende de qué más esté haciendo el CPU.
- **Disparada por Timer (match):** configurás un Timer para que dispare el ADC con período exacto
  (registro `ADCR`, campo `START`). El muestreo queda **uniforme**, que es justo lo que Nyquist asume.
- **Timer + ADC + DMA (módulo 11):** el Timer dispara, el ADC convierte y el DMA guarda en RAM, todo
  sin CPU. Es la forma profesional de capturar una señal a frecuencia fija. Combina los módulos 8, 10
  y 11.

> Para una medición lenta (temperatura, un potenciómetro) nada de esto importa: leés cuando querés. El
> tema aparece cuando muestreás algo que **se mueve rápido** (audio, vibración, una señal de un sensor
> con contenido de alta frecuencia). Ahí, elegir `fs` y poner el filtro antialiasing es la diferencia
> entre medir la señal y medir un fantasma.

## Lo que te llevás
- **Muestrear** es tomar fotos espaciadas de una señal continua; `fs` es cuántas por segundo.
- **Nyquist:** `fs` tiene que ser **más del doble** de la frecuencia máxima de la señal.
- **Aliasing:** lo que supera `fs/2` se disfraza de una frecuencia baja que no existe, y no se puede
  quitar después. Se previene con un **filtro pasabajos analógico antes del ADC** y muestreando rápido.
- Para muestreo **uniforme** en el LPC1769, dispará el ADC por **Timer** (idealmente con DMA), no a ojo.

---

**Anterior:** [02 - ADC/DAC con el driver](./02-adc-dac-con-driver.md) ·
**Módulo:** [ADC / DAC](./README.md) · **Siguiente módulo:** [11 - DMA](../11_dma/)
