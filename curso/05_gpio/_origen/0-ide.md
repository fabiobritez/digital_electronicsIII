# Instalación y uso de MCUXpresso IDE para LPC1769

## MCUXpresso IDE: ¿Qué es y por qué usarlo?

MCUXpresso IDE es el entorno oficial de NXP para el desarrollo con sus microcontroladores.
Está diseñado para **simplificar y centralizar** todo el flujo de trabajo de programación en un solo lugar.

Incluye:

* **Editor de código**: con resaltado de sintaxis, autocompletado y detección de errores (de sintaxis y warnings).
* **Herramientas de compilación** (basados en GCC ARM Embedded): no es necesario instalar toolchains por separado.
* **Depurador gráfico**: permite ejecutar el código paso a paso, inspeccionar variables, establecer *breakpoints* y visualizar registros.
* **Integración con CMSIS**: soporte estándar para ARM Cortex-M.
* **Gestión de SDKs**: descarga e integración automática de librerías y drivers específicos de la placa.
* **Vista de periféricos y registros**: útil para depuración.

---

## Alternativa: Hacer *build* y *run* manualmente

Podes optar por **no usar un IDE**, haciendo todo desde cero con herramientas independientes.
Esto requiere más pasos y conocimientos técnicos:

**Necesitarías:**

1. **Editor de texto** (VS Code, nvim, etc.).
2. **Toolchain ARM GCC**: `arm-none-eabi-gcc`, `arm-none-eabi-gdb`, `binutils-arm-none-eabi`.
3. **CMSIS**: descargar las librerías manualmente desde ARM o NXP.
4. **Makefile**: escribir reglas para compilar, enlazar y generar el binario (`.bin` o `.elf`).
5. **Herramienta de carga**: como `CMSIS-DAP`,`openocd` o algún otro probe que use un protocolo compatible para programar la placa.  

Si bien tienen un control total sobre la configuración, es más propenso a errores de integración.
Para iniciar, no se recomienda.

---

## 3. Instalación de MCUXpresso IDE
 
### Descarga e instalación

1. Ir a la [página oficial de MCUXpresso IDE](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE).
2. Registrarse y aceptar términos.
3. Descargar el instalador según el sistema operativo.

**En Ubuntu 22.04/24.04:**

```bash
# Nos ubicamos en la carpeta de descargas
cd ~/Descargas

# Hacemos el archivo ejecutable
chmod +x mcuxpressoide-11.8.0_1165.x86_64.deb.bin

# Ejecutamos el instalador
sudo ./mcuxpressoide-11.8.0_1165.x86_64.deb.bin
```

**En Windows:** ejecutar el `.exe` y seguir las instrucciones.

---

## 4. Creación de un nuevo proyecto

1. Abrir MCUXpresso IDE.
2. Seleccionar un **workspace** (ruta sin espacios recomendada).

Este espacio de trabajo es el directorio principal en el que se almacenarán y organizarán los proyectos y archivos relacionados con el desarrollo.   
    Esta ruta puede dejarse por defecto o se puede colocar otra.

Al dar click en lanzar, se nos abrirá la ventana principal de la IDE.

<img src="./img/ide.png" width="80%">


3. Ir a `File > New > Create a new C/C++ project`.
4. En **SDK Wizard**, seleccionar la placa: *LPC1769*.

<img src="./img/ide-1.png" width="80%">

5. Elegir `C project`, asignar nombre y ubicación.
6. Importar CMSIS:

   * `Import > Project archive (zip) > Browse`
   * Ruta: `Legacy > NXP > LPC1000 > LPC17xx > LPCXpresso176x_cmsis2.zip`
   * Seleccionar **CMSISV2p00** (mínimo requerido).

7. Omitir selección en librerías DSP.
8. Configurar estructura de directorios (opcional `inc` para *headers*).
9. Finalizar.

---

## 5. Verificar instalación y compilación

* Compilar con el ícono del martillo.
* Salida esperada:

```bash
Build Finished. 0 errors, 0 warnings.
```

* Opcional: cargar un ejemplo “blinky” para probar la placa y depurador.

---

## 6. Buenas prácticas

* Usar nombres cortos y sin espacios en proyectos. 
* Usar control de versiones (Git).
 

