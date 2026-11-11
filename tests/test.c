// Hello World
#include <stdio.h>

/* ==========================================================================
 * test.c - Cobre os elementos lexicos de C que o colorizador precisa
 * distinguir. Compila (gcc -Wall -std=gnu11), mas nao faz nada util: serve
 * de entrada para o colorizador e para o comparar.
 *
 * Especiais de HTML em comentario: < > & "aspas" 'apostrofo'
 * Acentuacao em comentario: ação, coração, é
 * ========================================================================== */

/* ---- 1. Diretivas de pre-processador ---------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <wchar.h>
#include <sys/types.h>      /* comentario depois do include */
#include <math.h>           // comentario de linha depois do include
#ifdef USAR_ARQUIVO_LOCAL
#include "arquivo_local.h"
#endif

#define MAX 100             /* tamanho maximo */
#define PI 3.14159          // comentario depois do define
#define NOME "texto em macro"
#define QUADRADO(x) ((x) * (x))
#define MAXIMO(a, b) ((a) > (b) ? (a) : (b))
#define BLOCO(x)    \
    do {            \
        (void)(x);  \
    } while (0)
#define STR(x) #x
#define COLA(a, b) a ## b
  #  define COM_ESPACOS 1
#undef COM_ESPACOS
#if defined(MAX) && MAX > 10
#  define GRANDE 1
#elif MAX > 5
#  define GRANDE 2
#else
#  define GRANDE 0
#endif
#ifndef NAO_DEFINIDO
#endif
#ifdef NUNCA_DEFINIDO
#error "isto nunca deve compilar"
#endif
#pragma GCC diagnostic ignored "-Wunused-variable"

/* ---- 2. Comentarios --------------------------------------------------- */
// comentario de linha
//comentario colado
/* comentario de bloco */
/**/
/* varias
   linhas
   de comentario */
/* // linha dentro de bloco */
// /* bloco dentro de linha */
/* < > & dentro de comentario */
// if (x) return 0; "string" 'c' 123 dentro de comentario
int com_comentario = 1; // fim de linha
int comentario_no_meio = /* no meio */ 2;

/* ---- 3. Tipos, modificadores e declaracoes ---------------------------- */
typedef unsigned char byte;
typedef struct ponto { int x; int y; } Ponto;
union valor { int i; float f; char c[4]; };
typedef enum { VERMELHO, VERDE = 5, AZUL } Cor;
struct campos { unsigned int a : 3; unsigned int b : 5; };
typedef int (*Operacao)(int, int);

extern int variavel_externa;
const volatile int constante_volatil = 7;
signed short curto = -1;
unsigned long long enorme = 1;
long double precisao = 1;
float real = 1;
double dupla = 1;
char caractere = 'x';
bool booleano = true;
bool outro_booleano = false;
void *ponteiro_nulo = NULL;
size_t tamanho = 0;
ptrdiff_t diferenca = 0;
wchar_t largo_c = L'x';
FILE *arquivo = NULL;
uint8_t octeto = 0;
int32_t i32 = 0;
uint64_t u64 = 0;

/* identificadores que contem palavras reservadas (nao devem ser coloridos) */
int integer = 1, formato = 2, if_ok = 3, returned = 4, intensidade = 5;
int _privado = 6, x1 = 7, valor_2 = 8, forca = 9, doce = 10, estatico = 11;

/* ---- 4. Numeros ------------------------------------------------------- */
double numeros[] = {
    0, 7, 42, 123456789,
    42u, 42U, 42l, 42L, 42ul, 42UL, 42ull,
    0x1F, 0XFF, 0xdeadBEEFu, 0x7fffffffL,
    017, 0777,
    3.14, .5, 5., 0.0,
    1e10, 1E10, 1e-3, 1E+3, 2.5e-2, 6.02e23,
    3.14f, 3.14F, 2.5L, 10.f,
    0x1p3, 0x1.8p1, 0x1p-3
};

/* ---- 5. Strings e caracteres ------------------------------------------ */
const char *strings[] = {
    "",
    "texto simples",
    "escapes: \n \t \r \\ \" \' \0 \a \b \f \v",
    "hex \x41 octal \101 \7 unicode \u00e9",
    "http://nao-e-comentario.com",
    "/* nao e comentario */",
    "int if for return 123 #include",
    "< > & dentro de string",
    "formatos %d %s %5.2f %%",
    "acentuação: olá, coração",
    "concatenacao " "de " "strings",
    u8"utf8",
    "continuacao \
na proxima linha",
    "aspas simples ' dentro",
};
const wchar_t *largo = L"largo";
const void *utf16 = u"utf16";
const void *utf32 = U"utf32";

char caracteres[] = {
    'a', 'Z', '0', ' ', '/', '#', '<', '&',
    '\n', '\t', '\\', '\'', '"', '\0', '\x41', '\101'
};

/* ---- 6. Funcoes: prototipos e definicoes ------------------------------ */
int soma(int n);
void *aloca(size_t n);
int (*ptr_func)(int, int);

int soma(int n)
{
    int i, s = 0;
    for (i = 1; i <= n; i++)
        s += i;
    return s;
}

void *aloca(size_t n) { return malloc(n); }

static inline int triplo(int x) { return x * 3; }

int
duas_linhas(void)
{
    return 0;
}

int variadica(int n, ...)
{
    va_list ap;
    int s = 0;
    va_start(ap, n);
    while (n-- > 0)
        s += va_arg(ap, int);
    va_end(ap);
    return s;
}

/* ---- 7. Operadores e pontuacao ---------------------------------------- */
int operadores(int a, int b)
{
    int r = 0;
    int v[3] = { 1, 2, 3 };
    Ponto p = { .x = 1, .y = 2 }, *q = &p;

    r = a + b - a * b / (b ? b : 1) % 7;
    r += a; r -= b; r *= 2; r /= 2; r %= 5;
    r &= 0xF; r |= 1; r ^= 2; r <<= 1; r >>= 1;
    r = (a << 1) | (b >> 1);
    r = (a & 3) ^ 1;
    r = ~a;
    r = !a || (a && b);
    r = (a == b) + (a != b) + (a < b) + (a > b) + (a <= b) + (a >= b);
    r++; ++r; r--; --r;
    r = -a + +b;
    r = v[0] + q->x + p.y + (*q).x;
    r = (int)3.7 + (int)sizeof(int) + (int)sizeof r;
    p = (Ponto){ .x = 3, .y = 4 };
    return r + p.x;
}

/* ---- 8. Estruturas de controle ---------------------------------------- */
int controle(int n)
{
	int i, total = 0;	/* indentado com TAB */

    if (n < 0) {
        return -1;
    } else if (n == 0) {
        return 0;
    } else {
        total = 1;
    }

    for (i = 0; i < n; i++) {
        if (i % 2 == 0) continue;
        if (i > 50) break;
        total += i;
    }
    while (total > 1000) total /= 2;
    do { total++; } while (total < 10);

    switch (n) {
        case 1:
            total += 1;
            break;
        case 2: case 3:
            total += 2;
            /* continua */
        default:
            total += 3;
    }

    {
        auto int a = 1;
        register int r = 2;
        static int s = 3;
        total += a + r + s;
    }

    #ifdef MAX
    total += MAX;
    #endif

    goto fim;
fim:
    return total;
}

/* ---- 9. Programa principal -------------------------------------------- */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("Hello World\n");
    printf("Soma = %d \"ok\"\n", soma(10));
    return 0;
}