/*
 * comparator.c - Compara o HTML gerado pelo colorir com o HTML esperado.
 *
 * COMPILACÃO:
 *   gcc -Wall -o comparator comparator.c
 *
 * COMO USAR (na raiz do projeto):
 *   ./colorir tests/test.c | ./tests/comparator tests/expected.html -
 *
 * O QUE FAZ:
 *   - Lê o conteúdo dentro de <pre class="codigo">...</pre> em ambos os arquivos.
 *   - Compara se cada caractere do código possui a mesma classe de destaque HTML.
 *   - Ignora espaços, tabulações e números de linha (span class="l").
 *
 * CÓDIGOS DE SAÍDA:
 *   0 = Identicos (sucesso)
 *   1 = Há divergências de cores/classes
 *   2 = Erro de uso ou divergência estrutural de texto
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CLASSES 64
#define MAX_PILHA   64
#define NORMAL      ""              /* classe de um caractere sem <span> */

typedef struct {
    char ch;                        /* caractere (ja sem entidade HTML) */
    const char *cls;                /* classe interna (ponteiro internado) */
    int linha, col;                 /* posicao no codigo-fonte, base 1 */
} Cel;

typedef struct {
    Cel *v;
    size_t n, cap;
} Lista;

/* ---------- nomes de classe internados: comparar ponteiros basta ---------- */

static char *nomes[MAX_CLASSES];
static int n_nomes = 0;

static const char *internar(const char *s, size_t len)
{
    for (int i = 0; i < n_nomes; i++)
        if (strlen(nomes[i]) == len && strncmp(nomes[i], s, len) == 0)
            return nomes[i];
    if (n_nomes == MAX_CLASSES) { fprintf(stderr, "classes demais\n"); exit(2); }
    nomes[n_nomes] = malloc(len + 1);
    memcpy(nomes[n_nomes], s, len);
    nomes[n_nomes][len] = '\0';
    return nomes[n_nomes++];
}

/* ---------- leitura de arquivo ---------- */

static char *le_arquivo(const char *nome, size_t *tam)
{
    FILE *f = (strcmp(nome, "-") == 0) ? stdin : fopen(nome, "rb");
    if (!f) { fprintf(stderr, "Erro: nao consegui abrir '%s'\n", nome); exit(2); }
    size_t cap = 8192, n = 0, lidos;
    char *buf = malloc(cap + 1);
    while ((lidos = fread(buf + n, 1, cap - n, f)) > 0) {
        n += lidos;
        if (n == cap) { cap *= 2; buf = realloc(buf, cap + 1); }
    }
    buf[n] = '\0';
    if (f != stdin) fclose(f);
    *tam = n;
    return buf;
}

/* ---------- HTML -> lista de (caractere, classe) ---------- */

static void empilha_cel(Lista *L, char ch, const char *cls, int linha, int col)
{
    if (L->n == L->cap) {
        L->cap = L->cap ? L->cap * 2 : 4096;
        L->v = realloc(L->v, L->cap * sizeof(Cel));
    }
    L->v[L->n++] = (Cel){ ch, cls, linha, col };
}

static const char *acha(const char *s, const char *agulha)
{
    return strstr(s, agulha);
}

static void analisa(const char *html, const char *nome, Lista *L)
{
    const char *ini = acha(html, "<pre class=\"codigo\">");
    if (!ini) {
        fprintf(stderr, "Erro: '%s' nao tem <pre class=\"codigo\">\n", nome);
        exit(2);
    }
    ini += strlen("<pre class=\"codigo\">");
    const char *fim = acha(ini, "</pre>");
    if (!fim) fim = ini + strlen(ini);

    const char *pilha[MAX_PILHA];   /* classe de cada <span> aberto */
    int topo = 0;
    int linha = 1, col = 1;
    const char *p = ini;

    while (p < fim) {
        if (*p == '<') {
            const char *gt = memchr(p, '>', (size_t)(fim - p));
            if (!gt) break;
            if (strncmp(p, "<span", 5) == 0) {
                const char *c = strstr(p, "class=\"");
                const char *cls = NORMAL;
                if (c && c < gt) {
                    c += 7;
                    const char *q = memchr(c, '"', (size_t)(gt - c));
                    if (q) cls = internar(c, (size_t)(q - c));
                }
                if (topo < MAX_PILHA) pilha[topo++] = cls;
            } else if (strncmp(p, "</span", 6) == 0) {
                if (topo > 0) topo--;
            }
            p = gt + 1;
            continue;
        }

        char ch = *p;
        size_t consumido = 1;
        if (ch == '&') {
            if      (strncmp(p, "&lt;", 4) == 0)  { ch = '<'; consumido = 4; }
            else if (strncmp(p, "&gt;", 4) == 0)  { ch = '>'; consumido = 4; }
            else if (strncmp(p, "&amp;", 5) == 0) { ch = '&'; consumido = 5; }
        }

        /* classe = span mais interno que nao seja o "l" de numero de linha */
        const char *cls = NORMAL;
        for (int k = topo - 1; k >= 0; k--)
            if (strcmp(pilha[k], "l") != 0) { cls = pilha[k]; break; }

        empilha_cel(L, ch, cls, linha, col);
        if (ch == '\n') { linha++; col = 1; } else col++;
        p += consumido;
    }
}

/* ---------- comparacao ---------- */

static int e_branco(char c) { return c == ' ' || c == '\t'; }

static const char *mostra_classe(const char *c)
{
    return (c[0] == '\0') ? "(sem cor)" : c;
}

typedef struct {
    const char *cls;
    long total, certos;
} Resumo;

static Resumo resumo[MAX_CLASSES + 1];   /* por classe esperada */
static int n_resumo = 0;

static Resumo *resumo_de(const char *cls)
{
    for (int i = 0; i < n_resumo; i++)
        if (resumo[i].cls == cls) return &resumo[i];
    resumo[n_resumo].cls = cls;
    return &resumo[n_resumo++];
}

/* Imprime uma divergencia: o trecho [a, b) das listas, de uma so linha. */
static void imprime_divergencia(const Lista *E, size_t a, size_t b,
                                const char *obtida)
{
    char texto[48];
    size_t k = 0, usados = 0;
    for (size_t i = a; i < b; i++) {
        usados++;
        if (k < sizeof texto - 4) texto[k++] = (E->v[i].ch == '\t') ? ' ' : E->v[i].ch;
    }
    texto[k] = '\0';
    if (usados > k) strcat(texto, "...");
    printf("  linha %3d, col %3d  esperado %-10s obtido %-10s  \"%s\"\n",
           E->v[a].linha, E->v[a].col,
           mostra_classe(E->v[a].cls), mostra_classe(obtida), texto);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s esperado.html obtido.html   (ou '-' para stdin)\n",
                argv[0]);
        return 2;
    }

    size_t tam_e, tam_o;
    char *html_e = le_arquivo(argv[1], &tam_e);
    char *html_o = le_arquivo(argv[2], &tam_o);

    Lista E = {0}, O = {0};
    analisa(html_e, argv[1], &E);
    analisa(html_o, argv[2], &O);

    /* 1) o texto tem que ser o mesmo, senao nao ha o que comparar */
    size_t n = (E.n < O.n) ? E.n : O.n;
    for (size_t i = 0; i < n; i++) {
        if (E.v[i].ch != O.v[i].ch) {
            printf("Os textos diferem (os dois HTML nao vem do mesmo .c).\n"
                   "  primeira diferenca: linha %d, col %d: esperado '%c', obtido '%c'\n",
                   E.v[i].linha, E.v[i].col, E.v[i].ch, O.v[i].ch);
            return 2;
        }
    }
    if (E.n != O.n) {
        printf("Os textos diferem no tamanho: esperado %zu caracteres, obtido %zu.\n"
               "  (o %s termina antes)\n", E.n, O.n, (E.n < O.n) ? "esperado" : "obtido");
        return 2;
    }

    /* 2) classe de cada caractere visivel */
    long total = 0, certos = 0;
    int n_div = 0;
    printf("Divergencias (esperado x obtido):\n");

    size_t i = 0;
    while (i < E.n) {
        char ch = E.v[i].ch;
        if (ch == '\n' || e_branco(ch)) { i++; continue; }

        Resumo *r = resumo_de(E.v[i].cls);
        r->total++;
        total++;

        if (E.v[i].cls == O.v[i].cls) { r->certos++; certos++; i++; continue; }

        /* divergencia: estende o trecho enquanto o par (esperada, obtida) se
           mantem e na mesma linha; brancos no meio nao interrompem */
        const char *ce = E.v[i].cls, *co = O.v[i].cls;
        size_t fim = i + 1;      /* fim (exclusivo) do ultimo caractere visivel */
        size_t j = i + 1;
        while (j < E.n && E.v[j].ch != '\n') {
            if (e_branco(E.v[j].ch)) { j++; continue; }
            if (E.v[j].cls == ce && O.v[j].cls == co) {
                r->total++; total++;
                j++; fim = j; continue;
            }
            break;
        }
        imprime_divergencia(&E, i, fim, co);
        n_div++;
        i = fim;
    }
    if (n_div == 0) printf("  (nenhuma)\n");

    /* 3) resumo por classe esperada */
    printf("\nResumo por classe esperada (caracteres visiveis):\n");
    for (int k = 0; k < n_resumo; k++)
        printf("  %-10s %5ld de %5ld corretos%s\n",
               mostra_classe(resumo[k].cls), resumo[k].certos, resumo[k].total,
               (resumo[k].certos == resumo[k].total) ? "" : "   <-- verificar");

    printf("\nTotal: %ld de %ld caracteres corretos (%.1f%%), %d divergencia(s).\n",
           certos, total, total ? 100.0 * (double)certos / (double)total : 100.0, n_div);
    printf("Resultado: %s\n", n_div == 0 ? "OK, identico ao esperado" : "DIFERENTE do esperado");

    free(E.v); free(O.v); free(html_e); free(html_o);
    return n_div == 0 ? 0 : 1;
}