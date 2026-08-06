/* ============================================================================
 * syscalls.c - El piso que la libreria estandar de C espera encontrar
 * ============================================================================
 *
 * printf(), malloc() y compania estan escritas asumiendo que abajo hay un
 * sistema operativo: cuando printf() termina de armar el texto, lo entrega
 * llamando a write(1, buffer, largo), y espera que alguien lo haga aparecer
 * en algun lado. malloc() pide mas memoria con sbrk().
 *
 * En el LPC1769 no hay ningun sistema operativo. Si no definimos esas
 * funciones, el linker corta con "undefined reference to _write" apenas
 * escribas un printf.
 *
 * Este archivo las define. La mayoria son stubs que fallan prolijamente
 * (no hay archivos ni procesos en un micro), y las dos que importan de verdad
 * son _sbrk (para malloc) y _write (para printf).
 *
 * COMO HACER QUE printf() SALGA POR LA UART
 * -----------------------------------------
 * No hay que tocar este archivo. Definí en tu codigo:
 *
 *     int __io_putchar(int ch)
 *     {
 *         while (!(U0LSR & (1 << 5))) { }   // esperar que el THR este libre
 *         U0THR = (uint8_t) ch;
 *         return ch;
 *     }
 *
 * y listo: _write() de mas abajo la va a usar. La version completa, con la
 * inicializacion de la UART y la conversion de LF a CRLF, esta en el modulo 0,
 * capitulo 13 del curso.
 *
 * Y tene presente que printf() es caro: se lleva varios KB de FLASH y es lento.
 * Para depurar de verdad conviene el debugger (modulo 12).
 * ========================================================================= */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;


/* ---------------------------------------------------------------------------
 * Salida de caracteres
 * ---------------------------------------------------------------------------
 * Version por defecto: tira todo a la basura. Esta declarada weak, asi que si
 * definis tu propia __io_putchar() en cualquier otro archivo, gana la tuya sin
 * que haya que configurar nada.
 * ------------------------------------------------------------------------ */
__attribute__((weak)) int __io_putchar(int ch)
{
    (void) ch;
    return ch;
}

__attribute__((weak)) int __io_getchar(void)
{
    return -1;
}


/* ---------------------------------------------------------------------------
 * _write - aca desemboca printf(), puts(), fwrite()...
 * ------------------------------------------------------------------------ */
int _write(int fd, const char *buf, int len)
{
    (void) fd;              /* no distinguimos stdout de stderr */
    for (int i = 0; i < len; i++) {
        __io_putchar(buf[i]);
    }
    return len;
}


/* ---------------------------------------------------------------------------
 * _read - aca desemboca scanf(), getchar()...
 * ------------------------------------------------------------------------ */
int _read(int fd, char *buf, int len)
{
    (void) fd;
    for (int i = 0; i < len; i++) {
        int c = __io_getchar();
        if (c < 0) {
            return i;
        }
        buf[i] = (char) c;
    }
    return len;
}


/* ---------------------------------------------------------------------------
 * _sbrk - de aca saca memoria malloc()
 * ---------------------------------------------------------------------------
 * El heap arranca donde termina .bss (el simbolo "end" lo define el linker
 * script) y crece hacia arriba. El stack arranca en el tope de la RAM y crece
 * hacia abajo. Se vienen de frente.
 *
 * El chequeo contra el stack pointer actual es lo que convierte un desastre
 * silencioso (malloc devuelve memoria que el stack va a pisar, y el programa
 * falla raro media hora despues) en un error honesto: malloc devuelve NULL.
 *
 * Igual, la recomendacion en embebidos sigue siendo no usar malloc: reservá
 * los buffers estaticos y sabé desde el dia uno cuanta RAM usa tu programa.
 * (Modulo 0, capitulo 9.)
 * ------------------------------------------------------------------------ */
void *_sbrk(ptrdiff_t incr)
{
    extern char end;              /* fin de .bss, lo pone el linker script */
    static char *heap_actual = NULL;

    if (heap_actual == NULL) {
        heap_actual = &end;
    }

    char *tope_stack = (char *) __builtin_frame_address(0);
    char *anterior = heap_actual;

    if (heap_actual + incr > tope_stack) {
        errno = ENOMEM;
        return (void *) -1;
    }

    heap_actual += incr;
    return (void *) anterior;
}


/* ---------------------------------------------------------------------------
 * El resto: no existen en un micro, pero el linker los pide igual
 * ---------------------------------------------------------------------------
 * _fstat e _isatty son los unicos con una respuesta interesante: le dicen a
 * la libc que la salida es un terminal de caracteres y no un archivo, para que
 * no intente hacer buffering por bloques.
 * ------------------------------------------------------------------------ */
int _close(int fd)
{
    (void) fd;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void) fd;
    st->st_mode = S_IFCHR;        /* dispositivo de caracteres */
    return 0;
}

int _isatty(int fd)
{
    (void) fd;
    return 1;
}

off_t _lseek(int fd, off_t offset, int whence)
{
    (void) fd; (void) offset; (void) whence;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void) pid; (void) sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void) status;
    /* No hay a donde salir. Si tu programa llega aca, algo termino y no
       deberia haber terminado. Frena y quedate: con el debugger conectado
       vas a poder ver el backtrace. */
    while (1) {
    }
}
