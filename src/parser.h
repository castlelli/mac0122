#ifndef PARSER_H
#define PARSER_H

#include "page.h"
/* Reads the header of the TXT at file_path, up to the "---" line, into a
   new page (pass 1). Lines without ':' and empty values are skipped.
   Fills the header fields, txt_path (a copy of file_path) and html_name
   (the file name with ".html" in place of the extension, no folder).
   Returns NULL if the file cannot be opened or memory runs out.
   The caller owns the returned page and frees it with page_free. */
page* parser_read_header(const char *file_path);


#endif 