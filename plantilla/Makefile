# =============================================================================
# Makefile para LPC1769 (ARM Cortex-M3)
# =============================================================================
#
# Esto es lo que MCUXpresso hace por vos cuando apretas "Build". La diferencia
# es que aca podes leerlo entero en cinco minutos.
#
#   make              compila -> build/firmware.elf, .bin, .hex
#   make flash        graba la placa (detecta solo que herramienta tenes)
#   make debug        graba y abre gdb, parado en main
#   make size         cuanta FLASH y cuanta RAM usa tu programa
#   make clean        borra lo generado
#   make help         la lista completa de comandos y opciones
#
# Para ver los comandos completos que se ejecutan:  make V=1
# =============================================================================


# -----------------------------------------------------------------------------
# 1. Opciones del proyecto
# -----------------------------------------------------------------------------
# Todas se pueden pisar desde la linea de comandos, por ejemplo:
#     make USE_CMSIS=1
#     make flash FLASHER=pyocd

# OJO con los comentarios en las asignaciones: en Make, todo lo que va despues
# del valor forma parte del valor, espacios incluidos. Por eso van arriba.

# Nombre del binario que sale.
PROJECT   ?= firmware

# Donde van los archivos intermedios.
BUILD_DIR ?= build

# Optimizacion. -Og optimiza pero deja debuggear. Tambien valen -O0 -O2 -Os.
OPT       ?= -Og

# 0 = bare metal, solo tu codigo (el micro queda a 4 MHz)
# 1 = compila y linkea los drivers CMSIS de NXP (y SystemInit deja el core a 100 MHz)
USE_CMSIS ?= 0

# Donde esta CMSIS. Por defecto, la copia que trae este repo.
CMSIS_DIR ?= ../library/CMSISv2p00_LPC17xx

# Grabador: auto | openocd | pyocd | linkserver | lpc21isp
FLASHER   ?= auto

# Puerto serie para grabar por ISP (solo si FLASHER=lpc21isp).
# ISP_XTAL es la frecuencia del cristal en kHz.
ISP_PORT  ?= /dev/ttyUSB0
ISP_BAUD  ?= 115200
ISP_XTAL  ?= 12000


# -----------------------------------------------------------------------------
# 2. El toolchain
# -----------------------------------------------------------------------------
# Se prefiere el toolchain local del repo (tools/toolchain/, que instala
# tools/install_toolchain.sh). Si no esta, se busca en el PATH del sistema.
# Se puede forzar con:  make CROSS=/ruta/a/arm-none-eabi-

TOOLCHAIN_LOCAL := $(wildcard ../tools/toolchain/bin/arm-none-eabi-gcc)
ifneq ($(TOOLCHAIN_LOCAL),)
  CROSS ?= ../tools/toolchain/bin/arm-none-eabi-
else
  CROSS ?= arm-none-eabi-
endif

CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size

# gdb no viene en todos los toolchains. Se busca el primero que exista.
GDB := $(firstword $(shell command -v $(CROSS)gdb 2>/dev/null) \
                   $(shell command -v gdb-multiarch 2>/dev/null) \
                   $(shell command -v arm-none-eabi-gdb 2>/dev/null))

PYTHON := $(firstword $(shell command -v python3 2>/dev/null) \
                      $(shell command -v python 2>/dev/null))


# -----------------------------------------------------------------------------
# 3. Flags del compilador
# -----------------------------------------------------------------------------

# --- Que CPU es. Esto no es negociable: el LPC1769 es un Cortex-M3 que solo
#     entiende el set de instrucciones Thumb-2.
CPUFLAGS := -mcpu=cortex-m3 -mthumb

# --- Advertencias. Activadas a fondo a proposito: en embebidos, un warning
#     ignorado suele ser un bug que despues aparece en la placa.
WARNINGS := -Wall -Wextra -Wshadow -Wundef -Wdouble-promotion

# --- Generacion de codigo
#     -ffunction-sections / -fdata-sections ponen cada funcion y cada variable
#     en su propia seccion, para que despues el linker pueda descartar con
#     --gc-sections todo lo que no se usa. Sin esto, linkear CMSIS te mete
#     drivers enteros que nunca llamas.
CODEGEN  := -ffunction-sections -fdata-sections -fno-common

# --- Debug: -g3 incluye hasta las macros del preprocesador.
DEBUGF   := -g3 -gdwarf-4

# --- Dependencias automaticas: genera un .d por cada .o con la lista de
#     headers incluidos, para que tocar un .h recompile lo que corresponde.
DEPFLAGS := -MMD -MP

CFLAGS := $(CPUFLAGS) $(OPT) $(WARNINGS) $(CODEGEN) $(DEBUGF) $(DEPFLAGS) \
          -std=gnu11 $(EXTRA_CFLAGS)


# -----------------------------------------------------------------------------
# 4. Que archivos se compilan
# -----------------------------------------------------------------------------

SRC := $(wildcard src/*.c) startup/startup_lpc1769.c
INC := -Iinc -Isrc

CMSIS_SRC :=
ifeq ($(USE_CMSIS),1)
  ifeq ($(wildcard $(CMSIS_DIR)/inc/LPC17xx.h),)
    $(error No encuentro CMSIS en '$(CMSIS_DIR)'. Pasa la ruta correcta con \
            make USE_CMSIS=1 CMSIS_DIR=/ruta/a/CMSISv2p00_LPC17xx)
  endif
  # system_LPC17xx.c trae SystemInit(): configura PLL, clock y wait states.
  CMSIS_SRC := $(CMSIS_DIR)/src/system_LPC17xx.c \
               $(wildcard $(CMSIS_DIR)/Drivers/src/*.c)
  INC += -I$(CMSIS_DIR)/inc -I$(CMSIS_DIR)/Drivers/inc
  CFLAGS += -D__USE_CMSIS -DCORE_M3
  # Los drivers de NXP son de 2010 y no compilan limpios con -Wextra. Se les
  # bajan las advertencias SOLO a ellos: tu codigo se sigue compilando con
  # todas activadas.
  #
  # El ultimo flag tapa un bug real de NXP, no una molestia de estilo: en
  # NVIC_SCBDeInit() (lpc17xx_nvic.c) el lazo recorre SCB->SHP de 0 a 31,
  # pero ese arreglo tiene 12 elementos. A partir del 12 escribe fuera del
  # arreglo, que en C es comportamiento indefinido. No nos afecta porque esa
  # funcion no se usa, pero conviene saber que esta ahi.
  CMSIS_CFLAGS := -Wno-unused-parameter -Wno-sign-compare -Wno-shadow \
                  -Wno-unused-variable -Wno-type-limits \
                  -Wno-implicit-fallthrough -Wno-aggressive-loop-optimizations
endif

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC:.c=.o)) \
       $(addprefix $(BUILD_DIR)/cmsis/,$(notdir $(CMSIS_SRC:.c=.o)))
DEP := $(OBJ:.o=.d)


# -----------------------------------------------------------------------------
# 5. Flags del linker
# -----------------------------------------------------------------------------

LDSCRIPT := linker/lpc1769.ld

# --specs=nano.specs   usa newlib-nano: una libc chica, pensada para micros.
#                      La libc normal se lleva decenas de KB solo en printf.
#                      No se usa nosys.specs: las llamadas al sistema las
#                      define src/syscalls.c, que ademas deja el lugar listo
#                      para enganchar printf a la UART.
# --gc-sections        descarta las secciones que nadie usa. Junto con
#                      -ffunction-sections, es lo que evita que linkear CMSIS
#                      te meta drivers enteros que nunca llamaste.
# -Map                 genera el mapa: que quedo donde y cuanto ocupa cada cosa.
#                      Cuando no entendes por que el binario crecio, se mira ahi.
# --print-memory-usage imprime el resumen de FLASH y RAM al terminar de linkear.
# --no-warn-rwx-segments  silencia un aviso de binutils nuevo. Aparece porque
#                      .data se guarda en FLASH (ejecutable) pero corre en RAM
#                      (escribible), y el linker junta las dos en un segmento
#                      que queda marcado como lectura+escritura+ejecucion. En
#                      un micro sin MMU eso no significa nada: no hay permisos
#                      de memoria que violar.

LDFLAGS := $(CPUFLAGS) -T$(LDSCRIPT) \
           --specs=nano.specs \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref \
           -Wl,--print-memory-usage \
           -Wl,--no-warn-rwx-segments \
           $(EXTRA_LDFLAGS)

ELF := $(BUILD_DIR)/$(PROJECT).elf
BIN := $(BUILD_DIR)/$(PROJECT).bin
HEX := $(BUILD_DIR)/$(PROJECT).hex
LST := $(BUILD_DIR)/$(PROJECT).lst


# -----------------------------------------------------------------------------
# 6. Modo silencioso
# -----------------------------------------------------------------------------
# Por defecto se muestra una linea corta por archivo. Con "make V=1" se ven
# los comandos completos, que es lo que conviene mirar la primera vez para
# entender que se esta ejecutando realmente.

ifeq ($(V),1)
  Q :=
else
  Q := @
endif


# -----------------------------------------------------------------------------
# 7. Reglas de construccion
# -----------------------------------------------------------------------------

.PHONY: all
all: $(ELF) $(BIN) $(HEX) size

# --- Compilar: un .c -> un .o -----------------------------------------------
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Los fuentes de CMSIS viven fuera de este directorio, asi que llevan su
# propia regla. Son dos: los drivers y el system_LPC17xx.c.
$(BUILD_DIR)/cmsis/%.o: $(CMSIS_DIR)/Drivers/src/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $< (cmsis)"
	$(Q)$(CC) $(CFLAGS) $(CMSIS_CFLAGS) $(INC) -c $< -o $@

$(BUILD_DIR)/cmsis/%.o: $(CMSIS_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $< (cmsis)"
	$(Q)$(CC) $(CFLAGS) $(CMSIS_CFLAGS) $(INC) -c $< -o $@

# --- Linkear: todos los .o + el linker script -> el .elf --------------------
$(ELF): $(OBJ) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	$(Q)$(CC) $(OBJ) $(LDFLAGS) -o $@
	$(Q)$(MAKE) --no-print-directory checksum-elf

# --- Convertir el .elf a los formatos que entienden los grabadores ----------
# El .elf tiene simbolos y datos de debug; la FLASH del micro no quiere nada
# de eso, solo los bytes. objcopy los extrae.
$(BIN): $(ELF)
	@echo "  OBJCOPY $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	@echo "  OBJCOPY $@"
	$(Q)$(OBJCOPY) -O ihex $< $@

# --- El checksum de la boot ROM ---------------------------------------------
# Se parchea el .elf, y de ahi salen el .bin y el .hex ya correctos.
# Ver la explicacion completa en tools/lpc_checksum.py.
.PHONY: checksum-elf
checksum-elf:
ifeq ($(PYTHON),)
	@echo "  AVISO   sin python: no se inyecto el checksum de la boot ROM."
	@echo "          Con openocd o lpc21isp anda igual (lo parchean ellos),"
	@echo "          pero con pyocd la placa no va a arrancar."
else
	@echo "  CKSUM   $(ELF)"
	$(Q)$(PYTHON) tools/lpc_checksum.py -q $(ELF)
endif

# --- Informacion util --------------------------------------------------------
.PHONY: size
size: $(ELF)
	@echo ""
	$(Q)$(SIZE) $(ELF)
	@echo ""
	@echo "  text = codigo + constantes (va a FLASH)"
	@echo "  data = globales con valor inicial (ocupa FLASH y RAM)"
	@echo "  bss  = globales en cero (solo RAM)"
	@echo "  FLASH usada = text + data      de 512 KB"
	@echo "  RAM   usada = data + bss       de 32 KB (+ stack en ejecucion)"

# Desensamblado con el codigo C intercalado. Sirve para ver que hizo de verdad
# el compilador con lo que escribiste.
.PHONY: lst
lst: $(LST)
$(LST): $(ELF)
	@echo "  OBJDUMP $@"
	$(Q)$(OBJDUMP) -d -S $< > $@

# Los primeros 8 vectores tal como quedaron en la FLASH.
.PHONY: vectores
vectores: $(ELF)
	@echo "Tabla de vectores (las 8 palabras que mira la boot ROM):"
	$(Q)$(OBJDUMP) -s -j .isr_vector $(ELF) | head -8
	$(Q)$(PYTHON) tools/lpc_checksum.py --check $(ELF)

# Todo lo que se puede verificar del firmware SIN la placa: el checksum de la
# boot ROM, el stack pointer inicial, el bit Thumb del Reset_Handler, la palabra
# de CRP y el tamano. Son las causas tipicas del "grabe y no hace nada", y las
# cinco se detectan aca, en la PC. "make flash" lo corre solo antes de grabar.
.PHONY: preflight
preflight: $(ELF)
ifeq ($(PYTHON),)
	@echo "  AVISO   sin python: no se pudieron correr los chequeos previos."
else
	$(Q)$(PYTHON) tools/preflight.py $(ELF)
endif


# -----------------------------------------------------------------------------
# 8. Grabar la placa
# -----------------------------------------------------------------------------
# Se detecta que herramienta hay instalada, en orden de preferencia. Se puede
# forzar cualquiera con FLASHER=...

HAS_OPENOCD    := $(shell command -v openocd 2>/dev/null)
HAS_PYOCD      := $(shell command -v pyocd 2>/dev/null)
HAS_LINKSERVER := $(shell command -v LinkServer 2>/dev/null)
HAS_LPC21ISP   := $(shell command -v lpc21isp 2>/dev/null)

ifeq ($(FLASHER),auto)
  ifneq ($(HAS_OPENOCD),)
    FLASHER := openocd
  else ifneq ($(HAS_PYOCD),)
    FLASHER := pyocd
  else ifneq ($(HAS_LINKSERVER),)
    FLASHER := linkserver
  else ifneq ($(HAS_LPC21ISP),)
    FLASHER := lpc21isp
  else
    FLASHER := ninguno
  endif
endif

OPENOCD_CFG ?= openocd/lpc1769.cfg

# Target de pyocd. "lpc1768" viene incorporado en pyocd y sirve para el 1769
# (misma familia, misma FLASH y misma RAM). Para el nombre exacto:
#   pyocd pack install LPC1769   y despues  make flash PYOCD_TARGET=lpc1769
PYOCD_TARGET ?= lpc1768

# -W (--no-wait) es importante: sin el, si la placa no esta enchufada pyocd
# se queda esperando en silencio para siempre, sin decir nada. Con -W corta
# enseguida con "No connected debug probes", que es lo que uno quiere leer.
PYOCD_FLAGS ?= -W -t $(PYOCD_TARGET)

.PHONY: flash
flash: $(ELF) $(BIN) $(HEX) preflight
	@echo "  FLASH   con $(FLASHER)"
ifeq ($(FLASHER),openocd)
	openocd -f $(OPENOCD_CFG) -c "program $(ELF) verify reset exit"
else ifeq ($(FLASHER),pyocd)
	pyocd flash $(PYOCD_FLAGS) $(HEX)
else ifeq ($(FLASHER),linkserver)
	LinkServer flash LPC1769 load $(ELF)
else ifeq ($(FLASHER),lpc21isp)
	lpc21isp -control $(HEX) $(ISP_PORT) $(ISP_BAUD) $(ISP_XTAL)
else
	@echo ""
	@echo "ERROR: no encontre ningun grabador instalado."
	@echo ""
	@echo "  Instala alguno de estos (ver docs del modulo 18):"
	@echo "    openocd    el estandar. sudo apt install openocd"
	@echo "    pyocd      en python.   pip install pyocd"
	@echo "    LinkServer el de NXP, si ya tenes MCUXpresso"
	@echo "    lpc21isp   para grabar por el puerto serie, sin sonda"
	@echo ""
	@echo "  O forza uno con:  make flash FLASHER=pyocd"
	@exit 1
endif

.PHONY: erase
erase:
	@echo "  ERASE   con $(FLASHER)"
ifeq ($(FLASHER),openocd)
	openocd -f $(OPENOCD_CFG) -c "init; reset halt; flash erase_sector 0 0 last; exit"
else ifeq ($(FLASHER),pyocd)
	pyocd erase $(PYOCD_FLAGS) --chip
else ifeq ($(FLASHER),linkserver)
	LinkServer flash LPC1769 erase
else
	@echo "ERROR: 'make erase' no esta soportado con FLASHER=$(FLASHER)"
	@exit 1
endif

# Reset de la placa sin volver a grabar.
.PHONY: reset
reset:
ifeq ($(FLASHER),openocd)
	openocd -f $(OPENOCD_CFG) -c "init; reset run; exit"
else ifeq ($(FLASHER),pyocd)
	pyocd reset $(PYOCD_FLAGS)
else
	@echo "ERROR: 'make reset' no esta soportado con FLASHER=$(FLASHER)"
	@exit 1
endif


# -----------------------------------------------------------------------------
# 9. Depurar
# -----------------------------------------------------------------------------
# Depurar un micro son siempre dos procesos:
#
#   1. un GDB SERVER, que habla SWD con la sonda de un lado y el protocolo
#      remoto de gdb del otro (openocd o pyocd hacen de esto)
#   2. gdb, que se conecta al server por TCP (puerto 3333)
#
# Cuando apretas F5 en VSCode, la extension Cortex-Debug lanza exactamente
# estos dos procesos. No hay nada mas.

GDB_PORT ?= 3333

# Deja el servidor corriendo en primer plano. Se usa en una terminal aparte.
.PHONY: gdbserver
gdbserver:
	@echo "  GDBSRV  $(FLASHER), escuchando en el puerto $(GDB_PORT)"
ifeq ($(FLASHER),openocd)
	openocd -f $(OPENOCD_CFG)
else ifeq ($(FLASHER),pyocd)
	pyocd gdbserver $(PYOCD_FLAGS) --port $(GDB_PORT)
else ifeq ($(FLASHER),linkserver)
	LinkServer gdbserver LPC1769
else
	@echo "ERROR: no hay un gdbserver disponible con FLASHER=$(FLASHER)"
	@exit 1
endif

# Levanta el servidor, conecta gdb, graba y para en main. Todo en un comando.
#
# El servidor se lanza en segundo plano y se mata al salir de gdb (el trap).
# Su salida va a build/gdbserver.log para no ensuciar la sesion: si algo falla
# al conectar, ese archivo es lo primero que hay que mirar.
.PHONY: debug
debug: $(ELF)
ifeq ($(GDB),)
	@echo "ERROR: no encontre gdb. Instalalo con: sudo apt install gdb-multiarch"
	@exit 1
else
	@mkdir -p $(BUILD_DIR)
	@echo "  GDBSRV  arrancando $(FLASHER) en segundo plano"
	$(Q)$(MAKE) --no-print-directory gdbserver > $(BUILD_DIR)/gdbserver.log 2>&1 & \
	  SRV=$$!; \
	  trap "kill $$SRV 2>/dev/null" EXIT INT TERM; \
	  sleep 2; \
	  if ! kill -0 $$SRV 2>/dev/null; then \
	    echo ""; echo "ERROR: el servidor gdb no arranco. Log:"; echo ""; \
	    cat $(BUILD_DIR)/gdbserver.log; exit 1; \
	  fi; \
	  $(GDB) $(ELF) -x debug.gdb
endif

# Igual que 'debug' pero sin grabar ni resetear: se engancha a lo que ya esta
# corriendo en la placa. Sirve para ver donde se colgo un programa.
.PHONY: attach
attach: $(ELF)
	@mkdir -p $(BUILD_DIR)
	$(Q)$(MAKE) --no-print-directory gdbserver > $(BUILD_DIR)/gdbserver.log 2>&1 & \
	  SRV=$$!; \
	  trap "kill $$SRV 2>/dev/null" EXIT INT TERM; \
	  sleep 2; \
	  $(GDB) $(ELF) -ex "target extended-remote :$(GDB_PORT)"


# -----------------------------------------------------------------------------
# 10. Soporte para editores
# -----------------------------------------------------------------------------
# compile_commands.json es el archivo que le dice a clangd (y por lo tanto a
# vim, neovim, helix, emacs, Sublime y al modo clangd de VSCode) con que flags
# se compila cada archivo. Sin el, el editor no sabe donde estan los headers
# ni que es un Cortex-M3, y te subraya en rojo codigo que compila perfecto.
#
# Si tenes "bear" instalado lo usa; si no, lo genera a mano con make -n.

.PHONY: compile_commands.json
compile_commands.json:
	@echo "  GEN     compile_commands.json"
	$(Q)if command -v bear >/dev/null 2>&1; then \
	    $(MAKE) clean >/dev/null && bear -- $(MAKE) >/dev/null; \
	else \
	    $(PYTHON) tools/gen_compile_commands.py; \
	fi
	@echo "          listo: clangd ya entiende el proyecto."


# -----------------------------------------------------------------------------
# 11. Limpieza y ayuda
# -----------------------------------------------------------------------------

.PHONY: clean
clean:
	@echo "  CLEAN   $(BUILD_DIR)"
	$(Q)rm -rf $(BUILD_DIR)

.PHONY: distclean
distclean: clean
	$(Q)rm -f compile_commands.json

.PHONY: info
info:
	@echo "Configuracion actual"
	@echo "  compilador  : $(shell $(CC) --version 2>/dev/null | head -1)"
	@echo "  ruta        : $(shell command -v $(CC) 2>/dev/null || echo 'NO ENCONTRADO')"
	@echo "  gdb         : $(if $(GDB),$(GDB),NO ENCONTRADO)"
	@echo "  python      : $(if $(PYTHON),$(PYTHON),NO ENCONTRADO)"
	@echo "  CMSIS       : $(if $(filter 1,$(USE_CMSIS)),si -> $(CMSIS_DIR),no (bare metal))"
	@echo "  grabador    : $(FLASHER)"
	@echo "  detectados  : $(if $(HAS_OPENOCD),openocd )$(if $(HAS_PYOCD),pyocd )$(if $(HAS_LINKSERVER),LinkServer )$(if $(HAS_LPC21ISP),lpc21isp )"
	@echo "  optimizacion: $(OPT)"

.PHONY: help
help:
	@echo "Plantilla LPC1769 - comandos"
	@echo ""
	@echo "  make               compila (.elf, .bin, .hex) y muestra el tamano"
	@echo "  make flash         graba la placa"
	@echo "  make debug         graba y abre gdb parado en main"
	@echo "  make gdbserver     solo el servidor gdb (para VSCode o gdb aparte)"
	@echo "  make erase         borra la FLASH entera"
	@echo "  make reset         resetea la placa"
	@echo "  make size          FLASH y RAM que usa el firmware"
	@echo "  make preflight     chequea que el firmware vaya a arrancar (sin placa)"
	@echo "  make lst           desensamblado con el C intercalado"
	@echo "  make vectores      muestra la tabla de vectores y su checksum"
	@echo "  make info          que herramientas detecto en esta maquina"
	@echo "  make compile_commands.json   para clangd (vim, neovim, helix...)"
	@echo "  make clean         borra build/"
	@echo ""
	@echo "Opciones (van despues del comando)"
	@echo "  V=1                muestra los comandos completos"
	@echo "  USE_CMSIS=1        compila con los drivers CMSIS de NXP"
	@echo "  OPT=-O2            cambia la optimizacion (default -Og)"
	@echo "  FLASHER=pyocd      fuerza el grabador (openocd|pyocd|linkserver|lpc21isp)"
	@echo "  CROSS=...          usa otro toolchain"
	@echo ""
	@echo "Ejemplos"
	@echo "  make USE_CMSIS=1 flash"
	@echo "  make V=1                 (para ver que hace de verdad)"

# Incluye los .d generados con -MMD. El guion inicial evita que falle la
# primera vez, cuando todavia no existen.
-include $(DEP)
