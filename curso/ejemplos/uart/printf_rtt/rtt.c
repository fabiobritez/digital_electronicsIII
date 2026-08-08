/* ============================================================================
 * rtt.c - Consola de depuracion por el cable del debugger (SWD)
 * ============================================================================
 *
 * Ver rtt.h para el uso y la configuracion, y
 * curso/12_debug/03-consola-por-el-debugger-rtt.md para la guia completa.
 *
 * COMO FUNCIONA
 * -------------
 *   printf()  ->  _write()  ->  cola circular en RAM
 *                                     ^
 *                            el debugger la lee por SWD
 *                            MIENTRAS el programa corre
 *
 * No hay periferico de por medio. El acceso a memoria por SWD lo hace la
 * unidad de debug del Cortex-M3, que es hardware aparte del CPU: puede leer
 * RAM sin frenar la ejecucion. Por eso escribir cuesta lo que cuesta un
 * memcpy y nada mas.
 *
 * El formato del bloque de control es el de SEGGER RTT, que es lo que
 * entienden OpenOCD (comandos "rtt setup" / "rtt server"), J-Link y pyOCD.
 * No usamos el codigo de SEGGER: son cien lineas y se leen enteras.
 * ========================================================================= */

#include "rtt.h"

#include <stdio.h>
#include <string.h>


/* ---------------------------------------------------------------------------
 * El bloque de control: un contrato binario con el host
 * ---------------------------------------------------------------------------
 * Los campos y su ORDEN no son negociables: el host los lee por offset. Si
 * cambias algo aca, deja de encontrarlo.
 * ------------------------------------------------------------------------ */
typedef struct {
    const char *sName;          /* nombre del canal, para el host */
    char       *pBuffer;
    unsigned    SizeOfBuffer;
    unsigned    WrOff;          /* lo escribe el MICRO */
    unsigned    RdOff;          /* lo escribe el HOST  */
    unsigned    Flags;
} canal_t;

typedef struct {
    char    acID[16];           /* "SEGGER RTT" + ceros */
    int     MaxNumUpBuffers;
    int     MaxNumDownBuffers;
    canal_t aUp[1];             /* micro -> PC */
    canal_t aDown[1];           /* PC -> micro */
} bloque_control_t;

static char up_buf[RTT_UP_SIZE];
static char down_buf[RTT_DOWN_SIZE];

/* volatile porque el host escribe RdOff (y el WrOff de bajada) por atras,
   sin que el compilador tenga forma de saberlo. */
static volatile bloque_control_t cb __attribute__((aligned(4)));

static uint32_t perdidos;
static char     buf_stdout[128];


/* ---------------------------------------------------------------------------
 * Seccion critica opcional (ver RTT_SEGURO_ISR en rtt.h)
 * ------------------------------------------------------------------------ */
#if RTT_SEGURO_ISR
static inline uint32_t entrar_critica(void)
{
    uint32_t primask;
    __asm__ volatile ("mrs %0, primask" : "=r" (primask));
    __asm__ volatile ("cpsid i" ::: "memory");
    return primask;
}
static inline void salir_critica(uint32_t primask)
{
    __asm__ volatile ("msr primask, %0" :: "r" (primask) : "memory");
}
#else
static inline uint32_t entrar_critica(void)      { return 0u; }
static inline void     salir_critica(uint32_t p) { (void) p; }
#endif


/* ---------------------------------------------------------------------------
 * escribir - mete lo que pueda en la cola de salida
 * ---------------------------------------------------------------------------
 * Devuelve cuantos bytes entraron. Copia con memcpy en uno o dos tramos (el
 * segundo cuando los datos dan la vuelta al final del arreglo), no byte por
 * byte: para una linea de 48 caracteres la diferencia es real.
 *
 * Se deja SIEMPRE un byte sin usar. Es el truco clasico de las colas
 * circulares: sin ese hueco, "cabeza == cola" significaria a la vez vacia y
 * llena, y no habria forma de distinguirlas.
 * ------------------------------------------------------------------------ */
static unsigned escribir(const char *datos, unsigned len)
{
    unsigned wr = cb.aUp[0].WrOff;
    unsigned rd = cb.aUp[0].RdOff;      /* lo mueve el host, se lee cada vez */

    unsigned libre = (rd > wr) ? (rd - wr - 1u)
                               : (RTT_UP_SIZE - wr + rd - 1u);
    if (len > libre) {
        len = libre;
    }
    if (len == 0u) {
        return 0u;
    }

    unsigned hasta_el_final = RTT_UP_SIZE - wr;

    if (len <= hasta_el_final) {
        memcpy(&up_buf[wr], datos, len);
        wr += len;
        if (wr == RTT_UP_SIZE) {
            wr = 0u;
        }
    } else {
        memcpy(&up_buf[wr], datos, hasta_el_final);
        memcpy(&up_buf[0], datos + hasta_el_final, len - hasta_el_final);
        wr = len - hasta_el_final;
    }

    /* Barrera: los datos tienen que estar en memoria ANTES de publicar el
       indice. Si el host viera el WrOff nuevo apuntando a un buffer todavia
       sin escribir, leeria basura. */
    __asm__ volatile ("dmb" ::: "memory");
    cb.aUp[0].WrOff = wr;

    return len;
}


static unsigned espacio_libre(void)
{
    unsigned wr = cb.aUp[0].WrOff;
    unsigned rd = cb.aUp[0].RdOff;
    return (rd > wr) ? (rd - wr - 1u) : (RTT_UP_SIZE - wr + rd - 1u);
}


/* ---------------------------------------------------------------------------
 * entregar - politica de que hacer cuando no entra
 * ---------------------------------------------------------------------------
 * O ENTRA TODO EL MENSAJE O NO ENTRA NADA. Nunca se escribe la mitad.
 *
 * Es la decision mas importante de este archivo, y no es un detalle de estilo.
 * Si al llenarse la cola se recortara el mensaje a la mitad, en la terminal
 * apareceria una linea incompleta que PARECE valida:
 *
 *     adc=2048 temp=25.4 C        <- buena
 *     adc=20                      <- esta recortada, pero no hay como saberlo
 *
 * Depurando, eso es peor que perder la linea entera: te manda a buscar un bug
 * que no existe. Descartando el mensaje completo, la garantia es fuerte y
 * simple: TODO LO QUE VES ESTA COMPLETO Y EN ORDEN. Lo que no entro, no
 * aparece, y queda contado en rtt_perdidos().
 *
 * Como stdout esta en buffering de linea, cada llamada trae una linea entera,
 * asi que "mensaje" y "linea" son lo mismo.
 * ------------------------------------------------------------------------ */
static void entregar(const char *datos, unsigned len)
{
    /* Un mensaje mas largo que la cola no entra nunca, ni esperando. Sin este
       guardia, el modo bloqueante giraria para siempre. Pasa si subis el
       buffer de stdout por encima de RTT_UP_SIZE. */
    if (len >= RTT_UP_SIZE) {
        perdidos += len;
        return;
    }

    uint32_t estado = entrar_critica();

#if RTT_BLOQUEANTE
    /* Esperar a que entre. Si nadie lee, esto no vuelve nunca: es exactamente
       lo que pediste al poner RTT_BLOQUEANTE en 1. */
    while (espacio_libre() < len) { }
    (void) escribir(datos, len);
#else
    if (espacio_libre() >= len) {
        (void) escribir(datos, len);
    } else {
        perdidos += len;            /* el mensaje entero, no un pedazo */
    }
#endif

    salir_critica(estado);
}


/* ---------------------------------------------------------------------------
 * Enganches de la libreria estandar
 * ---------------------------------------------------------------------------
 * Se definen los dos juegos: el de newlib y el de Redlib. En cada proyecto se
 * usa uno solo, y --gc-sections descarta el otro. Asi el mismo archivo sirve
 * para la plantilla del repo y para MCUXpresso sin tocar nada.
 * ------------------------------------------------------------------------ */

/* --- newlib (plantilla del repo, toolchain de linea de comandos) --------- */

int _write(int fd, const char *buf, int len)
{
    (void) fd;
    if (len > 0) {
        entregar(buf, (unsigned) len);
    }
    /* Se devuelve len aunque se haya descartado algo, a proposito: si
       dijeramos "escribi menos", stdio reintentaria en un lazo del que no se
       sale mientras nadie lea. Lo descartado se contabiliza y se consulta con
       rtt_perdidos(). */
    return len;
}

int _read(int fd, char *buf, int len)
{
    (void) fd;
    int n = 0;
    while (n < len) {
        int c = rtt_getchar();
        if (c < 0) {
            break;
        }
        buf[n++] = (char) c;
        if (c == '\n' || c == '\r') {   /* cortar por linea, como una consola */
            break;
        }
    }
    return n;
}

/* --- Redlib (MCUXpresso por defecto) ------------------------------------- */
/* Firma y semantica salen de redlib/include/sys/libconfig-arm.h. __sys_write
   devuelve la cantidad de caracteres que NO pudo escribir (0 = todo bien),
   siguiendo la convencion de semihosting de ARM.

   Definirlas aca NO choca con Redlib: en libcr_nohost.a cada una vive en su
   propio objeto (__sys_write.o), asi que el linker simplemente no lo saca del
   archivo. Verificado linkeando contra la Redlib de MCUXpresso 11.10. */

int __sys_write(int fh, char *buf, int len)
{
    (void) fh;
    if (len > 0) {
        entregar(buf, (unsigned) len);
    }
    return 0;
}

void __sys_write0(char *buf)
{
    entregar(buf, (unsigned) strlen(buf));
}

int __sys_readc(void)
{
    return rtt_getchar();
}


/* ---------------------------------------------------------------------------
 * API publica
 * ------------------------------------------------------------------------ */

void rtt_init(void)
{
    cb.aUp[0].sName        = "Terminal";
    cb.aUp[0].pBuffer      = up_buf;
    cb.aUp[0].SizeOfBuffer = RTT_UP_SIZE;
    cb.aUp[0].WrOff        = 0u;
    cb.aUp[0].RdOff        = 0u;
    cb.aUp[0].Flags        = RTT_BLOQUEANTE ? 2u : 0u;

    cb.aDown[0].sName        = "Terminal";
    cb.aDown[0].pBuffer      = down_buf;
    cb.aDown[0].SizeOfBuffer = RTT_DOWN_SIZE;
    cb.aDown[0].WrOff        = 0u;
    cb.aDown[0].RdOff        = 0u;
    cb.aDown[0].Flags        = 0u;

    cb.MaxNumUpBuffers   = 1;
    cb.MaxNumDownBuffers = 1;

    perdidos = 0u;

    /* El identificador se arma caracter por caracter y AL FINAL, y las dos
     * cosas son a proposito:
     *
     * - Caracter por caracter porque el host encuentra el bloque ESCANEANDO
     *   la RAM en busca de la cadena "SEGGER RTT". Si estuviera como literal,
     *   el compilador dejaria ademas una copia en .rodata y el host podria
     *   toparse con la equivocada.
     * - Al final porque, si el host se conecta justo durante el arranque, no
     *   queremos que encuentre un bloque a medio construir. */
    static const char id[16] = { 'S','E','G','G','E','R',' ','R','T','T',
                                 0,0,0,0,0,0 };
    for (int i = 15; i >= 0; i--) {
        cb.acID[i] = id[i];
    }
    __asm__ volatile ("dsb" ::: "memory");

    /* Buffering de linea, con buffer propio.
     *
     * Sin buffer, newlib llamaria a _write UNA VEZ POR CARACTER, y cada
     * llamada seria un viaje entero a la cola. Con buffering de linea entrega
     * la linea completa de un saque.
     *
     * Y el buffer se lo damos nosotros para que no lo pida al heap: con NULL,
     * el primer printf reserva 1032 bytes con malloc. */
    setvbuf(stdout, buf_stdout, _IOLBF, sizeof buf_stdout);
}


uint32_t rtt_perdidos(void)
{
    return perdidos;
}


uint32_t rtt_pendientes(void)
{
    unsigned wr = cb.aUp[0].WrOff;
    unsigned rd = cb.aUp[0].RdOff;
    return (wr >= rd) ? (wr - rd) : (RTT_UP_SIZE - rd + wr);
}


int rtt_flush(void)
{
    /* Tope de espera para no colgarse si no hay nadie leyendo. No es un
       tiempo exacto —depende del clock y de la optimizacion— sino un limite
       generoso: del orden de decimas de segundo a 100 MHz. */
    for (uint32_t vueltas = 0u; vueltas < 20000000u; vueltas++) {
        if (cb.aUp[0].RdOff == cb.aUp[0].WrOff) {
            return 1;
        }
        __asm__ volatile ("nop");
    }
    return 0;
}


int rtt_getchar(void)
{
    unsigned rd = cb.aDown[0].RdOff;

    if (rd == cb.aDown[0].WrOff) {
        return -1;                      /* no hay nada */
    }

    int c = (unsigned char) down_buf[rd];

    rd++;
    if (rd == RTT_DOWN_SIZE) {
        rd = 0u;
    }
    __asm__ volatile ("dmb" ::: "memory");
    cb.aDown[0].RdOff = rd;

    return c;
}
