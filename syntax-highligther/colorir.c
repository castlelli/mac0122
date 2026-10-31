/*
 * colorir.c - Filtro que recebe codigo C e devolve HTML colorido.
 *
 * Compilar:  gcc -Wall -o colorir colorir.c
 * Usar:      ./colorir programa.c > programa.html     (pagina completa)
 *            ./colorir -f programa.c > trecho.html    (so o <pre>, sem <html>)
 *            ./colorir -c > estilo.css                (so o CSS, para as paginas
 *                                                      do site usarem em comum)
 *            ./colorir < programa.c                   (le da entrada padrao)
 *
 * Classes lexicas reconhecidas:
 *   pragma   : linhas que comecam com # (#include, #define, ...)
 *   tipo     : int, char, float, double, void, struct, ...
 *   comando  : if, else, for, while, return, ...
 *   coment   : comentario de linha (barra dupla) e de bloco (barra-asterisco)
 *   string   : "texto\n"  e  'c'
 *   numero   : 10, 3.14, 0xFF, 1e-3
 *
 * Cores: padrao do Pygments (o mesmo usado nos blocos de codigo da Wikipedia).
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
 
typedef enum { C_PRAGMA, C_TIPO, C_COMANDO, C_COMENT, C_STRING, C_NUMERO } Classe;
 
static const char *NOME_CLASSE[] = {
    "pragma", "tipo", "comando", "coment", "string", "numero"
};
 
static const char *TIPOS[] = {
    "int", "char", "float", "double", "void", "long", "short", "signed",
    "unsigned", "struct", "union", "enum", "typedef", "const", "static",
    "extern", "volatile", "register", "size_t", "FILE", NULL
};
 
static const char *COMANDOS[] = {
    "if", "else", "for", "while", "do", "switch", "case", "default",
    "break", "continue", "return", "goto", "sizeof", NULL
};
 
static int esta_em(const char *palavra, size_t n, const char **lista)
{
    for (int k = 0; lista[k] != NULL; k++)
        if (strlen(lista[k]) == n && strncmp(lista[k], palavra, n) == 0)
            return 1;
    return 0;
}
 
/* Cada linha do codigo vai dentro de <span class="l">...</span>.
   O CSS usa um contador sobre essa classe para desenhar o numero da linha
   (o numero nao faz parte do texto, entao nao e' copiado junto). */
static int linha_aberta = 0;
 
static void abre_linha(FILE *out)
{
    if (!linha_aberta) { fputs("<span class=\"l\">", out); linha_aberta = 1; }
}
 
static void fim_linha(FILE *out)      /* trata um '\n' do codigo-fonte */
{
    abre_linha(out);                  /* linha vazia tambem recebe numero */
    fputs("</span>\n", out);
    linha_aberta = 0;
}
 
/* Escreve s[0..n) escapando os caracteres especiais do HTML. */
static void escreve(FILE *out, const char *s, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\n') { fim_linha(out); continue; }
        abre_linha(out);
        switch (s[i]) {
            case '<': fputs("&lt;", out);  break;
            case '>': fputs("&gt;", out);  break;
            case '&': fputs("&amp;", out); break;
            default:  fputc(s[i], out);
        }
    }
}
 
/* Escreve s[0..n) dentro de <span class="..."> ... </span>.
   Se o trecho tiver varias linhas, fecha/reabre o span a cada '\n'
   (evita problemas se alguem quiser numerar linhas depois). */
static void escreve_span(FILE *out, Classe c, const char *s, size_t n)
{
    size_t ini = 0;
    for (size_t i = 0; i <= n; i++) {
        if (i == n || s[i] == '\n') {
            if (i > ini) {
                abre_linha(out);
                fprintf(out, "<span class=\"%s\">", NOME_CLASSE[c]);
                escreve(out, s + ini, i - ini);
                fputs("</span>", out);
            }
            if (i < n) fim_linha(out);
            ini = i + 1;
        }
    }
}
 
/* Cada ler_*() recebe a posicao i do inicio do item e devolve a posicao
   logo depois do fim do item. */
 
static size_t ler_pragma(const char *s, size_t n, size_t i)
{
    size_t j = i;
    while (j < n && s[j] != '\n') {
        if (s[j] == '\\' && j + 1 < n && s[j + 1] == '\n') { j += 2; continue; }
        /* comentario no fim da linha de #include nao e' pragma */
        if (s[j] == '/' && j + 1 < n && (s[j + 1] == '/' || s[j + 1] == '*'))
            break;
        j++;
    }
    return j;
}
 
static size_t ler_coment_linha(const char *s, size_t n, size_t i)
{
    size_t j = i + 2;
    while (j < n && s[j] != '\n') j++;
    return j;
}
 
static size_t ler_coment_bloco(const char *s, size_t n, size_t i)
{
    size_t j = i + 2;
    while (j < n && !(s[j] == '*' && j + 1 < n && s[j + 1] == '/')) j++;
    return (j < n) ? j + 2 : n;       /* se nao fechou, vai ate o fim */
}
 
static size_t ler_string(const char *s, size_t n, size_t i)
{
    char aspa = s[i];                 /* '"' ou '\'' */
    size_t j = i + 1;
    while (j < n && s[j] != aspa && s[j] != '\n') {
        if (s[j] == '\\' && j + 1 < n) j++;   /* pula o caractere escapado */
        j++;
    }
    return (j < n && s[j] == aspa) ? j + 1 : j;
}
 
static size_t ler_numero(const char *s, size_t n, size_t i)
{
    size_t j = i;
    while (j < n) {
        if (isalnum((unsigned char)s[j]) || s[j] == '.') j++;
        else if ((s[j] == '+' || s[j] == '-') && j > i &&
                 (s[j-1] == 'e' || s[j-1] == 'E')) j++;   /* 1e-3 */
        else break;
    }
    return j;
}
 
static size_t ler_identificador(const char *s, size_t n, size_t i)
{
    size_t j = i;
    while (j < n && (isalnum((unsigned char)s[j]) || s[j] == '_')) j++;
    return j;
}
 
/* Funcao principal do modulo: pode ser chamada pelo gerador de paginas
   dos outros membros do grupo. Escreve apenas o conteudo (sem <pre>). */
void colorir_c(const char *s, size_t n, FILE *out)
{
    size_t i = 0, j;
    int inicio_linha = 1;      /* so espacos ate aqui nesta linha? */
 
    linha_aberta = 0;
 
    while (i < n) {
        char c = s[i];
 
        if (c == '\n') { fim_linha(out); i++; inicio_linha = 1; continue; }
        if (isspace((unsigned char)c)) { escreve(out, &c, 1); i++; continue; }
 
        if (inicio_linha && c == '#') {
            j = ler_pragma(s, n, i);
            escreve_span(out, C_PRAGMA, s + i, j - i);
        }
        else if (c == '/' && i + 1 < n && s[i + 1] == '/') {
            j = ler_coment_linha(s, n, i);
            escreve_span(out, C_COMENT, s + i, j - i);
        }
        else if (c == '/' && i + 1 < n && s[i + 1] == '*') {
            j = ler_coment_bloco(s, n, i);
            escreve_span(out, C_COMENT, s + i, j - i);
        }
        else if (c == '"' || c == '\'') {
            j = ler_string(s, n, i);
            escreve_span(out, C_STRING, s + i, j - i);
        }
        else if (isdigit((unsigned char)c) ||
                 (c == '.' && i + 1 < n && isdigit((unsigned char)s[i + 1]))) {
            j = ler_numero(s, n, i);
            escreve_span(out, C_NUMERO, s + i, j - i);
        }
        else if (isalpha((unsigned char)c) || c == '_') {
            j = ler_identificador(s, n, i);
            if (esta_em(s + i, j - i, TIPOS))
                escreve_span(out, C_TIPO, s + i, j - i);
            else if (esta_em(s + i, j - i, COMANDOS))
                escreve_span(out, C_COMANDO, s + i, j - i);
            else
                escreve(out, s + i, j - i);          /* identificador comum */
        }
        else {                                       /* operador, pontuacao */
            j = i + 1;
            escreve(out, s + i, 1);
        }
 
        inicio_linha = 0;
        i = j;
    }
 
    if (linha_aberta) {                /* ultima linha sem '\n' no final */
        fputs("</span>", out);
        linha_aberta = 0;
    }
}
 
static const char *CSS =
    "/* caixa cinza com numeracao de linhas, estilo listings do LaTeX */\n"
    "pre.codigo { background:#f2f2f2; border:1px solid #c8c8c8;\n"
    "             border-radius:4px; padding:10px 12px; overflow:auto;\n"
    "             font-family:Menlo,Consolas,'DejaVu Sans Mono',monospace;\n"
    "             font-size:14px; line-height:1.45; tab-size:4;\n"
    "             counter-reset:linha; }\n"
    ".l::before { counter-increment:linha; content:counter(linha);\n"
    "              display:inline-block; width:2.5em; margin-right:1em;\n"
    "              padding-right:0.8em; text-align:right; color:#888;\n"
    "              border-right:1px solid #bbb; user-select:none; }\n"
    ".pragma  { color:#BC7A00; }\n"
    ".tipo    { color:#B00040; }\n"
    ".comando { color:#008000; font-weight:bold; }\n"
    ".coment  { color:#408080; font-style:italic; }\n"
    ".string  { color:#BA2121; }\n"
    ".numero  { color:#666666; }\n";
 
static char *le_tudo(FILE *f, size_t *tam)
{
    size_t cap = 4096, n = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;
    size_t lidos;
    while ((lidos = fread(buf + n, 1, cap - n, f)) > 0) {
        n += lidos;
        if (n == cap) {
            cap *= 2;
            char *novo = realloc(buf, cap);
            if (!novo) { free(buf); return NULL; }
            buf = novo;
        }
    }
    *tam = n;
    return buf;
}
 
#ifndef SEM_MAIN   /* compile com -DSEM_MAIN para usar so a funcao colorir_c */
int main(int argc, char *argv[])
{
    int fragmento = 0;
    const char *nome = NULL;
 
    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "-f") == 0) fragmento = 1;
        else if (strcmp(argv[k], "-c") == 0) { fputs(CSS, stdout); return 0; }
        else nome = argv[k];
    }
 
    FILE *in = stdin;
    if (nome != NULL && (in = fopen(nome, "r")) == NULL) {
        fprintf(stderr, "Erro: nao consegui abrir '%s'\n", nome);
        return 1;
    }
 
    size_t n;
    char *texto = le_tudo(in, &n);
    if (texto == NULL) { fprintf(stderr, "Erro de memoria\n"); return 1; }
    if (in != stdin) fclose(in);
 
    if (!fragmento)
        printf("<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n"
               "<title>%s</title>\n<style>\n%s</style></head>\n<body>\n",
               nome ? nome : "codigo C", CSS);
 
    printf("<pre class=\"codigo\">");
    colorir_c(texto, n, stdout);
    printf("</pre>\n");
 
    if (!fragmento) printf("</body></html>\n");
 
    free(texto);
    return 0;
}
#endif
