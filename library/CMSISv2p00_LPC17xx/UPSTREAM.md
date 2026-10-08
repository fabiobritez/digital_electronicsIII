# Origen de los drivers modernizados

El directorio `Drivers/` se importó desde:

- Repositorio: <https://github.com/David-A-T-M/LPC17xx-CMSIS-Driver-Enhancement>
- Commit: `511893e93413bdde5ef5b1f3d4752c7cdd4f1548`
- Fecha de importación: 2026-10-08

El repositorio de origen indica licencia BSD-3-Clause tanto para la biblioteca original de NXP como
para sus modificaciones. La licencia CMSIS/NXP incluida continúa en `docs/`.

Para comparar una actualización futura sin ensuciar el repositorio principal:

```sh
git clone https://github.com/David-A-T-M/LPC17xx-CMSIS-Driver-Enhancement.git \
  .vendor/LPC17xx-CMSIS-Driver-Enhancement
diff -qr library/CMSISv2p00_LPC17xx \
  .vendor/LPC17xx-CMSIS-Driver-Enhancement/CMSISv2p00_LPC17xx
```

`.vendor/` está en el `.gitignore`; el clon es solamente una referencia local.

## Ajustes locales posteriores a la importación

- Se corrigió `lpc17xx.h` → `LPC17xx.h` en dos includes para compilar en Linux.
- GPDMA ya no desplaza por un valor negativo al tocar `DMAREQSEL` para conexiones 0..7/M2M.
- Los valores `AUTO` de ancho y burst se resuelven desde el extremo periférico; en M2M usan word y
  burst 32.
- El DAC usa word como ancho automático, porque el dato de `DACR` ocupa los bits 15:6.
- `GPDMA_ChannelGracefulStop()` también limpia `E` después de drenar el FIFO, como indica UM10360
  31.6.1.4 y como promete su documentación.
- Se completó la validación de ambos endpoints de `GPDMA_Channel_CFG_T`.
