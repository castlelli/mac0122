#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

/* Tira espaços, tabs, \r e \n das duas pontas de line.
   Altera line (escreve '\0' no fim), então não aceita string literal.
   Devolve um ponteiro para dentro de line, não uma cópia: não dê free nele. */
char *text_utils_trim(char* line);

/* Retorna uma nova string alocada dinamicamente que copia a string original.
Quem chama essa função recebe a cópia e deve liberá-la. Retorna NULL se a alocação
de memória falhar. */
char *text_utils_copy_str(const char* str);

/* Separa line nas vírgulas, tira os espaços das pontas de cada pedaço
   e ignora os pedaços vazios. Cada pedaço é copiado com malloc.
   Altera line (troca as vírgulas por '\0'), então não aceita string literal.
   Escreve em *count quantas strings foram guardadas.
   Devolve o vetor de strings, ou NULL (com *count = 0) se não sobrar
   nenhum pedaço ou se faltar memória. Quem chama é dono do vetor e das
   strings, e deve liberar tudo com text_utils_free_str_vector. */
char** text_utils_split_comma(char* line, int* count);

/* Libera as count primeiras strings de str_vector e depois o próprio vetor.
   Aceita str_vector NULL com count 0. */
void text_utils_free_str_vector(char **, int);

#endif