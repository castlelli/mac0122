# AGENTS.md

Instructions for AI coding agents working in this repository. Humans: see
the group's README and reports. Every rule below is checkable; when a rule
and a request conflict, stop and ask instead of picking one silently.

## Project

EP1 for MAC0122 (USP). A C89 program that reads course-content TXT files
(written in Portuguese) and generates a static HTML site: `index.html`,
six index pages (technique, algorithm, authors, date, concepts, problem),
and one page per TXT, with C code blocks syntax-highlighted.

Pipeline (two passes):
1. Read the header of every TXT and build the six indexes.
2. Generate every HTML page, sending each code block to the highlighter.

## Layout

```
src/
  main.c                  orchestrates pass 1, pass 2 and output
  page.h / page.c         page and block structs; create and free
  parser.h / parser.c     reads one TXT: header fields + body blocks
  text_utils.h / .c       trim, normalize (lowercase, strip accents), split on commas
  sorted_list.h / .c      sorted linked list of page references
  bst.h / bst.c           binary search tree: insert, search, in-order walk, free
  indexes.h / indexes.c   builds the six indexes from the pages
  html.h / html.c         writes content pages and index pages
  c_highlighter.h / .c    filter: C source in, highlighted HTML out
  c_keywords.h / .c       sorted vector of C reserved words + binary search
input/                    course TXT files (content in Portuguese)
output/                   generated HTML; never edit by hand
algorithms/               standalone C programs + their data files (separate deliverable)
tests/                    one test program per module: test_<module>.c
tools/                    standalone programs built on src/ modules (e.g. the C filter)
```

Each module talks to the others only through its `.h` file. Do not change a
public header (function signature, struct field, constant) as a side effect
of another task; propose the interface change first.

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
[codigo]
...C code, passed verbatim to the highlighter...
[/codigo]
[ref] Book or source reference
```

- Header keys are in Portuguese because they are part of the content
  format; keep them as they are. An empty value means "ignore the field".
- `---` ends the header. Multi-value fields are comma-separated.
- Strip `\r` from every line: files may have Windows line endings.

## Build

Run from the repository root:

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror src/*.c -o ep1
```

Programs in `algorithms/` are built one at a time with the same flags:

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror algorithms/<file>.c -o <file>
```

Tests and tools link the `src/` modules they use, never `src/main.c`
(it has its own `main`). Run tests under AddressSanitizer:

```
gcc -std=c89 -Wall -Wextra -pedantic -Werror -fsanitize=address -g \
    tests/test_<module>.c src/<module>.c [other src modules] -o test_<module>
./test_<module>
```

A test program exits 0 when every case passes and prints the failing
cases otherwise.

Definition of done: every build command above exits with status 0 and
prints nothing. Code that compiles only with warnings is not done.
Never commit the `ep1` executable or any other binary.

## Code style

Language: all identifiers, file names, comments and commit messages are in
English. Portuguese appears only in TXT content, in HTML text, and in
string literals that end up in the generated pages.

Naming (why: full-word, descriptive identifiers measurably improve
comprehension and defect finding; see References):
- `snake_case` for functions, variables, struct tags, typedefs and files.
- `UPPER_SNAKE_CASE` for macros, constants and enum values.
- Prefix public functions with their module: `bst_insert`,
  `sorted_list_insert_unique`, `parser_read_header`, `html_write_index_page`.
- Functions start with a verb and say what they do:
  `normalize_index_key`, not `norm` or `process`.
- Booleans read as a question: `is_header_finished`, `has_code_block`.
- No single-letter names and no ad-hoc abbreviations, including loop
  counters: `keyword_index`, `line_number`, not `i`, `ln`, `cnt`.
- Allowed abbreviations (well known, so as readable as the full word):
  `bst`, `html`, `txt`, `mst`, `str`, `len`, `max`, `min`, `src`, `dst`.
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
- Escape `<`, `>` and `&` in every piece of text written to HTML.
- Comments explain why, not what the next line does.

## Git workflow

- Never commit or push to `main`. Work on a branch named
  `<type>/<short-kebab-description>`, e.g. `feat/bst-insert`,
  `fix/parser-crlf-handling`, and open a pull request.
- Never force-push a branch someone else has pulled; never rewrite `main`.

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
1. The build command prints nothing and exits 0.
2. New identifiers follow the naming rules above.
3. The commit contains one logical change and no binaries or `output/` edits
   unless generating output is the task.
4. The message follows the format, with a body that explains why.

## When unsure

Ask before: changing a public header, changing the TXT format, renaming
files, adding a dependency, or deleting anything. Do not guess the
professor's requirements; say what is unknown.

## References

- AGENTS.md format: https://agents.md
- Conventional Commits 1.0.0: https://www.conventionalcommits.org/en/v1.0.0/
- Beams, "How to Write a Git Commit Message": https://cbea.ms/git-commit/
- Tian et al., "What Makes a Good Commit Message?", ICSE 2022. doi:10.1145/3510003.3510205
- Herzig & Zeller, "The Impact of Tangled Code Changes", MSR 2013
- Lawrie et al., "What's in a Name? A Study of Identifiers", ICPC 2006. doi:10.1109/ICPC.2006.51
- Schankin et al., "Descriptive Compound Identifier Names Improve Source Code Comprehension", ICPC 2018. doi:10.1145/3196321.3196332
- Linux kernel coding style, ch. 4 "Naming": https://www.kernel.org/doc/html/latest/process/coding-style.html