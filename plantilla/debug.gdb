# =============================================================================
# debug.gdb - Guion de arranque de gdb para depurar en la placa
# =============================================================================
#
# gdb no sabe nada de microcontroladores ni de SWD. Lo unico que sabe hacer es
# hablar el "protocolo remoto de gdb" por TCP con un servidor. Ese servidor es
# openocd (o pyocd), que del otro lado maneja la sonda.
#
#     gdb  <--- TCP 3333 --->  openocd  <--- SWD --->  sonda  --->  LPC1769
#
# Este archivo automatiza los cuatro comandos que hay que tipear siempre.
# Lo usa "make debug".
# =============================================================================

# Conectarse al servidor. "extended-remote" (en vez de "remote") permite
# reiniciar el programa sin cerrar la sesion.
target extended-remote :3333

# Resetear el micro y dejarlo frenado antes de la primera instruccion.
# "monitor" manda el comando al servidor (openocd), no a gdb.
monitor reset halt

# Grabar el firmware en la FLASH. Esto es lo mismo que hace "make flash",
# pero desde adentro de la sesion de debug.
load

# Volver a resetear, para arrancar limpio desde el codigo recien grabado.
monitor reset halt

# Frenar al entrar en main y arrancar. Sin esto el programa correria libre y
# nunca verias el arranque.
break main
continue

# -----------------------------------------------------------------------------
# A partir de aca la sesion es interactiva. Los comandos que mas se usan:
#
#   next / n        siguiente linea (sin entrar en las funciones)
#   step / s        siguiente linea (entrando en las funciones)
#   continue / c    seguir hasta el proximo breakpoint
#   finish          terminar la funcion actual y volver
#   break archivo.c:42     poner un breakpoint en una linea
#   break UART0_IRQHandler poner un breakpoint en una funcion
#   info breakpoints       ver los breakpoints puestos
#   delete 2               borrar el breakpoint numero 2
#
#   print variable         ver una variable
#   print/x variable       verla en hexadecimal
#   print *(uint32_t*)0x2009C000    leer un registro del periferico por direccion
#   display contador       mostrarla automaticamente en cada parada
#   watch contador         frenar cuando esa variable cambie (muy util)
#
#   info registers         los registros del CPU (r0-r15, xPSR)
#   backtrace / bt         la pila de llamadas: como llegaste hasta aca
#   monitor reset halt     resetear el micro
#
#   layout src             abre una vista con el codigo fuente (modo TUI)
#   Ctrl-x a               entra y sale del modo TUI
#   quit                   salir
#
# Para depurar un hard fault: cuando el programa se cuelgue, frena con Ctrl-C
# y mira "backtrace" e "info registers". El registro IPSR te dice que
# excepcion se disparo. (Modulo 12 del curso.)
# -----------------------------------------------------------------------------
