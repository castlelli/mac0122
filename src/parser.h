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

/* Re-opens the TXT at page_header->txt_path, skips the header without
   parsing it, and reads the body (pass 2) into blocks: "# " lines are
   sections, "[ref]" lines references, other non-blank lines paragraphs.
   The lines between "[code]" and "[/code]" become one code block, with
   their indentation; an empty [code] block, or one never closed, adds
   nothing.
   Returns NULL if page_header is NULL, the file cannot be opened or
   memory runs out. The caller owns the returned body and frees it with
   page_body_free before reading the next TXT. */
page_body* parser_read_body(const page* page_header);
#endif 
