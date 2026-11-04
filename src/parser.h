#ifndef PARSER_H
#define PARSER_H

#include "page.h"
/* Reads the header of the TXT at file_path, up to the "---" line, into a
   new page (pass 1). Lines without ':' and empty values are skipped.
   For now it only stores txt_name and prints each [key] = [value] pair.
   Returns NULL if the file cannot be opened or memory runs out.
   The caller owns the returned page and frees it with page_free. */
page* parser_read_header(const char *file_path);


#endif 