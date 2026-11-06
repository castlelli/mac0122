#ifndef C_HIGHLIGHTER_H
#define C_HIGHLIGHTER_H

#include <stdio.h>

/*
 * C syntax highlighter: C source in, highlighted HTML out.
 *
 * Every source line becomes <span class="l">...</span> (the CSS draws the
 * line number with a counter, so the number is not copied with the code).
 * Tokens are wrapped in <span class="NAME"> where NAME is one of: pragma,
 * arquivo, coment, tipo, comando, builtin, funcao, estrutura, rotulo,
 * string, escape, numero, operador. The class names are Portuguese because
 * they are part of the generated pages.
 *
 * Only the content is written: the caller wraps it in
 * <pre class="codigo">...</pre>.
 *
 * Ownership: nothing is allocated and neither file is closed; both stay
 * owned by the caller. The function keeps no global state, so it can be
 * called once per code block.
 */

/* Reads source_file until end of input and writes the highlighted HTML.
   Carriage returns are dropped, so CRLF and LF sources give the same HTML. */
void c_highlighter_write_html(FILE *source_file, FILE *html_file);

/* Writes the CSS that styles the classes above: the content of the site's
   shared style.css, or of a page's own <style> block. */
void c_highlighter_write_css(FILE *css_file);

/* Writes the styles of a page's <head>: a link to the shared style.css and,
   when has_page_style is 1, a <style> block with the same CSS. The cascade
   lets the later of two equal rules win, so a page's own styles override the
   shared sheet only if they come after the link. A caller that adds <style>
   blocks of its own must write them after this call. */
void c_highlighter_write_head_styles(FILE *html_file, int has_page_style);

#endif
