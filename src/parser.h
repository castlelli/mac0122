#ifndef PARSER_H
#define PARSER_H
/* Lê um TXT e separa o cabeçalho em pares chave e valor. */

/* Tira espaços, tabs, \r e \n das duas pontas de line.
   Altera line (escreve '\0' no fim), então não aceita string literal.
   Devolve um ponteiro para dentro de line, não uma cópia: não dê free nele. */
char *parser_trim(char *line);

/* Lê o cabeçalho de file_path até a linha "---" e imprime cada par
   [chave] = [valor]. Ignora linhas sem ':'.
   Devolve 1 se deu certo e -1 se não conseguiu abrir o arquivo. */
int parser_read_file(const char *file_path);


#endif 