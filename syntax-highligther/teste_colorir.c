#include <stdio.h>
#define MAX 100 /* tamanho maximo */

// soma dos n primeiros inteiros
int soma(int n)
{
    int i, s = 0;
    for (i = 1; i <= n; i++)
        s += i;
    return s;
}

/* comentario
   de varias linhas */
int main(void)
{
    float x = 3.14, y = 1e-3;
    char c = '\n';
    if (x < y && MAX > 0)
        printf("Soma = %d \"ok\"\n", soma(10));
    return 0;
}
