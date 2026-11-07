# AGENTS.md

Instructions for AI coding agents working in this repository. Humans:
read [README.md](README.md) ([Português](README.pt-BR.md)) first.

Every rule below is checkable. When a rule and a request conflict, stop
and ask instead of picking one silently.

## Project

EP1 for MAC0122 (USP), phase 1 due 2026-11-12. A C89 program that reads
course-content TXT files (written in Portuguese by the professor) and
generates a static HTML site: `index.html`, one index page per index
and sort order, and one content page per TXT, with C code blocks
syntax-highlighted.

The architecture (diagrams, data model, highlighter state machine)
lives in `README.md`. It is the source of truth: read the section for
the module you are changing before you change it. This file holds only
the rules that follow from it.

## Commands

Run from the repository root.

Build the program:

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror src/*.c -o ep1
```

Generate the site into `output/` (`template.txt` is skipped, never a page):

```
./ep1 input/*.txt
```

Build one program from `algorithms/` (each is standalone):

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror algorithms/<file>.c -o <file>
```

Build and run one test under AddressSanitizer. Tests and tools link the
`src/` modules they use, never `src/main.c` (it has its own `main`):

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror -fsanitize=address -g \
    tests/test_<module>.c src/<module>.c [other src modules] -o test_<module>
./test_<module>
```

A test program exits 0 when every case passes and prints the failing
cases otherwise.

Compare the highlighter with the expected output of `tests/test.c`
(this one prints a report and exits 0 when the output matches):

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror tools/c_filter.c \
    src/c_highlighter.c src/c_keywords.c src/text_utils.c -o c_filter
gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/comparator.c -o comparator
./c_filter tests/test.c | ./comparator tests/expected.html -
```

The comparator has its own test, which includes `comparator.c`:

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror -fsanitize=address -g \
    tests/test_comparator.c -o test_comparator
./test_comparator
```

Definition of done: every command above exits with status 0 and, except
for the comparator's report, prints nothing. Code that compiles only with
warnings is not done.

## Layout

```
README.md, README.pt-BR.md  architecture, in English and Portuguese
PRODUCT.md                  product facts: audience, purpose, constraints
DESIGN.md                   visual design of the generated site
AGENTS.md                   this file
src/
  main.c                    runs pass 1, builds the indexes, runs pass 2
  page.h / page.c           header struct (one per TXT) and body block structs
  parser.h / parser.c       reads a TXT header into a header struct; reads a body
  text_utils.h / .c         trim, normalize (lowercase, strip accents), split on commas,
                            HTML escape
  sorted_list.h / .c        sorted linked list of pointers to header structs
  bst.h / bst.c             binary search tree of index keys
  indexes.h / indexes.c     builds the six indexes from the header structs
  graph.h / graph.c         graph of pages and keys: global and local layouts
  html.h / html.c           writes index.html, index pages, content pages, graph
                            pages and style.css
  c_highlighter.h / .c      state machine: C source in, highlighted HTML out
  c_keywords.h / .c         sorted array of (word, kind) pairs + binary search
input/                      course TXT files; template.txt shows the format
output/                     generated site; never edit by hand
algorithms/                 standalone C programs + data files (separate deliverable)
tests/                      one test program per module: test_<module>.c; comparator.c,
                            with test.c and expected.html as its input, and
                            test_comparator.c
tools/                      standalone programs built on src/ modules (c_filter.c)
```

Each module talks to the others only through its `.h` file. Do not
change a public header (function signature, struct field, constant) as
a side effect of another task; propose the interface change first.

## TXT format

```
Tecnica: Método guloso
Algoritmo: Kruskal (árvore espalhada mínima)
Autores: Joseph Kruskal
Data: 1956
Conceitos: Grafos, Árvores, árvore espalhada mínima, union-find
Problema: Dado um grafo não dirigido com pesos nas arestas, ...
---
# Section title
Paragraph text.
[code]
...C code, passed verbatim to the highlighter...
[/code]
[ref] Book or source reference
```

- The TXTs are written by the professor. Do not change the format; if a
  file does not fit it, report the file instead of adapting the format.
- Header keys are in Portuguese because they are part of the content
  format; keep them as they are. An empty value means "ignore the field".
- `---` ends the header.
- `Autores` and `Conceitos` hold one or more comma-separated values.
  `Tecnica`, `Algoritmo` and `Data` hold exactly one value.
- `Data` is `dd/MM/YYYY` or `YYYY`.
- Strip `\r` from every line: files may have Windows line endings.
- Files are UTF-8. One line is one paragraph; `[ref]` lines are collected
  at the end of the page.
- A file that does not fit (cannot be opened, no `---` line, invalid
  `Data`) is reported, and the program stops with status 1 before writing
  any page.

## Architecture rules

Each rule summarizes a decision recorded in `README.md`.

Passes and memory:
- Pass 1 reads only the header of each TXT, into one header struct per
  TXT. The struct keeps its TXT's file name.
- Pass 2 re-opens each TXT, skips the header without parsing it again,
  reads the body, writes the content page, and frees the body before
  the next TXT. A header is parsed exactly once.
- `main` owns every header struct, from pass 1 to the end of the run.
  Indexes store pointers to header structs and never copy them.

Indexes:
- Each index is a BST of keys. Each node holds a sorted list of pointers
  to header structs; add pages with `sorted_list_insert_unique`, so a
  page appears once per key.
- Each value of `Autores` and `Conceitos` is its own key.
- Keys are normalized (lowercase, accents stripped) before comparison and
  in anchor ids. Index pages show the spelling of the first TXT, by file
  name, that uses the key, with the first letter capitalized by CSS.
- The problem index's key is the whole normalized problem sentence.
- Dates compare as dates, not as text. A year-only date sorts before
  every full date of the same year (`1956` before `12/03/1956`).

Index pages:
- The reader sorts the pages under each key by a single-valued
  attribute: algorithm, technique, date or problem. An index does not
  offer its own attribute. Authors and concepts are never sort orders.
- Each sort order is its own pre-generated file,
  `<index>_by_<attribute>.html` (for example `concepts_by_date.html`).
- A page with no value for the attribute comes after those that have one.
- Ties are broken by TXT file name, and only by it.
- A node's list is sorted by TXT file name; each sort order is a temporary
  list that lasts while its table is written.
- The generated site is static HTML and CSS. No JavaScript.
- The visual design follows `DESIGN.md`: system fonts, one navy accent,
  white pages, no images. Update `DESIGN.md` in the same commit as a change
  to `SITE_CSS_LINES` or to the page anatomy in `html.c`.
- The site has one shared `style.css`. Every page links it with
  `c_highlighter_write_head_styles`. A style present on a page overrides
  the shared sheet, so any `<style>` block comes after that link.

Highlighter:
- It is a finite state machine with the states drawn in `README.md`.
  Two-character constructs (`/*`, `*/`, `//`, escapes) use in-between
  states; do not peek ahead with `ungetc` or a second buffer.
- It streams: one character at a time, keeping only the current state,
  the word buffer (the current word and the blanks after it) and the
  small variables listed in `README.md`. Holding those blanks is the one
  allowed delay: a word's style can depend on the next significant
  character.
- It writes 13 styles and wraps every line in `<span class="l">`, which
  the CSS counter numbers.
- Keywords are looked up when a word ends, by binary search in the
  sorted array of (word, kind) pairs in `c_keywords`. Keep that array
  sorted.
- Every character written goes through the HTML escape (`<`, `>`, `&`),
  inside strings and comments too.
- At end of input, close any open span.
- Highlight `//` comments in the input even though our own code is C89:
  the code blocks are the professor's.

Graph view:
- The graph is built from the headers and indexes only, and its layout
  is deterministic: a fixed start, a fixed number of steps, integer
  coordinates. Do not include `<math.h>`; use graph.c's square root,
  sine and cosine, so the build needs no `-lm`.
- Algorithm keys are not nodes. Pages and keys are linked only through
  the index lists.
- Text from a TXT goes into the SVG only as element content (`<title>`,
  `<text>`), never into an attribute; hrefs and ids use the text_utils
  writers.
- Local graph pages are built, written and freed one at a time.

Out of scope for phase 1; do not add: search bar, automatic links inside
`Problema`, plurals and synonyms.

## Decisions made

- Highlighter interface:
  `void c_highlighter_write_html(FILE *source_file, FILE *html_file);`
  `void c_highlighter_write_html_from_text(const char *source_text, FILE *html_file);`
  `void c_highlighter_write_css(FILE *css_file);`
  `void c_highlighter_write_head_styles(FILE *html_file, int has_page_style);`
- The parser keeps a code block as `char *`, so `html.c` passes it to
  `c_highlighter_write_html_from_text`.
- The HTML escape shared by `html.c` and the highlighter lives in
  `text_utils`, so the standalone filter in `tools/` does not depend on
  `html.c`.
- One shared `style.css`, written by the program. A style present on a
  page takes priority over it.
- TXTs are UTF-8, one line is one paragraph, and `[ref]` lines are
  collected at the end of the page.

## Open decisions

Ask before implementing any of these; do not pick one silently.

None at the moment.

## Code style

Language: all identifiers, file names, comments and commit messages are
in English. Portuguese appears only in TXT content, in HTML text, in
string literals that end up in the generated pages, and in
`README.pt-BR.md`.

Naming (why: full-word, descriptive identifiers measurably improve
comprehension and defect finding; see References):
- `snake_case` for functions, variables, struct tags, typedefs and files.
- `UPPER_SNAKE_CASE` for macros, constants and enum values (including
  highlighter states: `LINE_START`, `AFTER_SLASH`).
- Prefix public functions with their module: `bst_insert`,
  `sorted_list_insert_unique`, `parser_read_header`,
  `html_write_index_page`.
- Functions start with a verb and say what they do:
  `normalize_index_key`, not `norm` or `process`.
- Booleans read as a question: `is_header_finished`, `has_code_block`.
- No single-letter names and no ad-hoc abbreviations, including loop
  counters: `keyword_index`, `line_number`, not `i`, `ln`, `cnt`.
- Allowed abbreviations (well known, so as readable as the full word):
  `bst`, `html`, `css`, `txt`, `mst`, `str`, `len`, `max`, `min`, `src`, `dst`.
  Add to this list in a separate commit; do not invent others inline.
- No Hungarian notation or type prefixes (`p_page`, `iCount`).

C89 rules (why: the course compiles with `-std=c89`; each item below
is an error under the build flags):
- Comments use `/* */` only; no `//`.
- Declare variables at the start of a block, before any statement;
  no declarations inside `for (...)`.
- No `<stdbool.h>`, `<stdint.h>`, `inline`, `restrict`, `snprintf`,
  `%zu` or designated initializers. Booleans are `int` (0 or 1) with a
  question-style name.
- Aggregate initializers use constant expressions only.
- No string literal longer than 509 characters; split long text into
  several literals or several writes.

General C rules:
- Check every `malloc`, `realloc` and `fopen` result before use.
- Every header documents ownership: who allocates, who frees. Free BST
  nodes in post-order.
- Use `const` for pointers that are not written through.
- Comments explain why, not what the next line does.

## Documentation

- `README.md` and `README.pt-BR.md` describe the same architecture.
  Change both in the same commit; identifiers and file names stay in
  English in both.
- A change that alters the architecture (a pass, a data structure, a
  state, an output file) updates the README in the same pull request.
- Decisions still open go in "Open decisions" above, not in code
  comments.

## Git workflow

- Never commit or push to `main`. Work on a branch named
  `<type>/<short-kebab-description>`, e.g. `feat/bst-insert`,
  `fix/parser-crlf-handling`, and open a pull request.
- Never force-push a branch someone else has pulled; never rewrite `main`.
- Never commit the `ep1` executable, test executables or any other
  binary.

## Commits: Conventional Commits 1.0.0

Format:

```
<type>(<scope>): <description>

<body>

<footer>
```

Types: `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `style`,
`build`, `chore`, `revert`.
Scopes: `highlighter`, `parser`, `index`, `html`, `content`, `docs`, `build`.

Header (first line):
- Imperative mood, lowercase after the colon, no trailing period.
- At most 72 characters in total; aim for a description of 50 or fewer.
- Specific: name the thing changed. Not "fix bug" or "update code".

Body (required for every commit except trivial typo fixes):
- Blank line after the header; wrap at 72 characters.
- Say what changed in behavior and why it was needed; the diff already
  shows how. Mention side effects and limits.

Footer: `BREAKING CHANGE: <what breaks>` when a public header changes
incompatibly (also add `!` before the colon), and `Refs: #<issue>` when
there is one.

Atomic commits (why: tangled commits hide which change caused a bug):
- One logical change per commit; the build passes at every commit.
- If the description needs "and", split the commit.
- Never mix formatting or renames with behavior changes.
- Stage hunks selectively (`git add -p`) when one file holds two changes.

Good:

```
fix(parser): strip carriage return from header values

Files saved on Windows end lines with \r\n. The \r stayed attached to
every header value, so "Grafos\r" and "Grafos" became two different
keys in the concepts index.

Trim \r together with the other trailing whitespace in
text_utils_trim().
```

Bad: `fixed stuff`, `feat: bst and parser and html`, `Update bst.c.`

## Before every commit

Re-read this checklist each time; do not rely on having read it at the
start of the session.
1. Every command in "Commands" prints nothing and exits 0.
2. New identifiers follow the naming rules above.
3. The change follows "Architecture rules"; if it changes the
   architecture, both READMEs are updated.
4. The commit contains one logical change and no binaries or `output/`
   edits unless generating output is the task.
5. The message follows the format, with a body that explains why.

## Boundaries

Always: build with the flags above, run the module's test under
AddressSanitizer, escape text written to HTML, document ownership in
headers.

Ask first: changing a public header, changing the TXT format, renaming
or deleting files, adding a dependency (including JavaScript or a
diagram tool), settling an item in "Open decisions", adding anything
listed as out of scope.

Never: commit to `main`, commit binaries, edit `output/` by hand, guess
the professor's requirements. When something is unknown, say what is
unknown.

## References

- AGENTS.md format: https://agents.md
- Conventional Commits 1.0.0: https://www.conventionalcommits.org/en/v1.0.0/
- Beams, "How to Write a Git Commit Message": https://cbea.ms/git-commit/
- Tian et al., "What Makes a Good Commit Message?", ICSE 2022. doi:10.1145/3510003.3510205
- Herzig & Zeller, "The Impact of Tangled Code Changes", MSR 2013
- Lawrie et al., "What's in a Name? A Study of Identifiers", ICPC 2006. doi:10.1109/ICPC.2006.51
- Schankin et al., "Descriptive Compound Identifier Names Improve Source Code Comprehension", ICPC 2018. doi:10.1145/3196321.3196332
- Linux kernel coding style, ch. 4 "Naming": https://www.kernel.org/doc/html/latest/process/coding-style.html
