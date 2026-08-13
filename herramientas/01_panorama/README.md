# 01 - Panorama

Antes de profundizar conviene reconocer las capas y el vocabulario. Esta parte es breve y está
pensada para leerse en orden; después podés consultar el resto según el problema que estés
resolviendo.

## Recorrido

1. [01 - El mapa completo: de `main.c` a un LED parpadeando](./01-el-mapa-completo.md)
   Las capas principales entre el código fuente y el hardware en ejecución, primero en general y
   después con ejemplos del LPC1769.
2. [02 - Cómo se graba un micro](./02-como-se-graba-un-micro.md)
   Las dos vías de desarrollo más comunes (sonda de debug y bootloader), sus requisitos y por qué la
   FLASH no se escribe como la RAM.
3. [03 - Glosario](./03-glosario.md)
   Host, target, probe, gdbserver, HAL, SDK, BSP, IDE, toolchain. Los términos que después
   se usan sin aclarar, y las confusiones típicas entre ellos.

## La idea que atraviesa todo

En ambos casos, el código pasa por compilación y enlazado. La diferencia central es que en una
PC el **sistema operativo** carga el ejecutable y presta varios servicios; en un micro sin sistema
operativo, el proyecto debe resolverlos de forma explícita.

| Lo que en una PC hace el sistema operativo | En un micro lo hace |
|---|---|
| Cargar el ejecutable en memoria | en un caso típico, la imagen ya está en FLASH; el startup copia las secciones que deben vivir en RAM |
| Decidir en qué dirección va cada cosa | el **linker script**, escrito por vos o provisto por el SDK |
| Inicializar las variables globales antes de `main` | el **startup** del proyecto o de la plataforma |
| Saber a dónde va un `printf` | la redirección elegida: UART, RTT, semihosting u otra salida |
| Cargar y arrancar el programa desde afuera | un programador, una sonda o un bootloader |
| Dejarte inspeccionar un proceso colgado | el hardware de debug **adentro del chip** |

La unidad recorre esa columna y muestra qué capa resuelve cada problema. En sistemas con RTOS,
bootloader propio o memoria externa, algunas responsabilidades se reparten de otra manera.

---

**Unidad:** [Herramientas](../README.md) ·
**Siguiente parte:** [02 - Protocolos y debug en el chip](../02_protocolos_y_debug_en_el_chip/)
