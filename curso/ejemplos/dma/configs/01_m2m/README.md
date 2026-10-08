# DMA memoria a memoria

Este ejemplo compara tres formas de mover datos entre posiciones de memoria. Se usa el canal 7, de
menor prioridad, para que una copia M2M no demore periféricos que podrían perder datos.

## Copia simple

`config_dma_m2m_words()` copia un bloque completo. El origen y el destino incrementan después de
cada word, por lo que ambos buffers se recorren en el mismo orden. `linkedList = 0` indica que la
transferencia termina al copiar la cantidad configurada.

## Llenado de memoria

`config_dma_m2m_fill()` repite una misma word en todo el destino. El origen no incrementa (`SI = 0`),
pero el destino sí lo hace (`DI = 1`). Este patrón permite inicializar un buffer con un valor fijo.

## Cadena finita con LLI

`config_dma_m2m_tres_bloques()` reúne tres bloques en un destino continuo:

```text
origen_a ──→ destino[0..15]
origen_b ──→ destino[16..31]
origen_c ──→ destino[32..47] ──→ fin
```

El bloque A se configura en los registros del canal. Una primera LLI describe B y apunta a la LLI
de C. La última usa `nextLLI = 0`, por eso la cadena es finita; si apuntara otra vez al primer
descriptor sería circular. Solo la última LLI solicita una interrupción.

Para lograrlo sin modificar el driver, `intTC` queda deshabilitado durante la configuración para que
A no interrumpa. La LLI de B no incluye el bit `I`, la de C sí lo incluye y, después de configurar el
canal, se habilita únicamente la máscara `ITC` de `DMACCConfig`. Así el TC final puede llegar a la ISR
sin agregar una interrupción al primer bloque.

Este caso es *gather* porque reúne varios orígenes en un destino continuo. *Scatter* realiza la
operación inversa: distribuye un origen entre varios destinos.

Verificá desde el debugger que `destino_demo` contiene A, B y C en orden, y que la interrupción
ocurre después de copiar el último bloque.

## Compilar

```bash
cp curso/ejemplos/dma/configs/01_m2m/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
