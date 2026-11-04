#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

/* Removes spaces, tabs, '\r' and '\n' from both ends of line.
   Changes line (writes '\0' at the new end), so it does not accept a
   string literal. Returns a pointer into line, not a copy: do not free it. */
char *text_utils_trim(char* line);

/* Returns a new malloc'd copy of str. The caller owns the copy and frees
   it. Returns NULL if memory runs out. */
char *text_utils_copy_str(const char* str);

/* Splits line at the commas, trims each piece and skips the empty ones.
   Each piece is copied with malloc. Changes line (commas become '\0'),
   so it does not accept a string literal. Writes in *count how many
   strings were kept.
   Returns the string vector, or NULL (with *count = 0) when no piece is
   left or memory runs out. The caller owns the vector and the strings
   and frees them all with text_utils_free_str_vector. */
char** text_utils_split_comma(char* line, int* count);

/* Frees the first count strings of str_vector and then the vector.
   Accepts a NULL str_vector with count 0. */
void text_utils_free_str_vector(char **, int);

/* Removes spaces, tabs, '\r' and '\n' from the right end of line only.
   The start is kept, so it fits lines whose indentation matters (code).
   Changes line (writes '\0' at the new end), so it does not accept a
   string literal. Returns line itself, not a copy: do not free it. */
char* text_utils_trim_end(char* line);

#endif
