# Como escrever os TXTs do EP1

Cada TXT vira uma página do site. O programa lê o arquivo, separa as
informações e monta a página e os índices (Técnica, Algoritmo, Autores, Data,
Conceitos, Problema).

O jeito mais simples de começar é **copiar o `input/template.txt` e trocar o
conteúdo**.

---

## Visão geral

O arquivo tem duas partes, separadas por uma linha que contém só `---`:

```
Tecnica: Método guloso                    ┐
Algoritmo: Kruskal (árvore espalhada ...) │
Autores: Joseph Kruskal                   │  CABEÇALHO
Data: 1956                                │  (vai para os índices)
Conceitos: grafo, árvore, union-find      │
Problema: Dado um grafo não dirigido ...  ┘
---                                          SEPARADOR
# Ideia do algoritmo                      ┐
Ordena as arestas por peso, ...           │
                                          │  CORPO
[code]                                    │  (vira o texto da página)
int find(int parent[], int vertex)        │
...                                       │
[/code]                                   │
[ref] Cormen et al., ...                  ┘
```

---

## Parte 1: o cabeçalho

Cada linha tem o formato `Chave: valor`.

| Chave | O que colocar | Quantos valores | Exemplo |
|---|---|---|---|
| `Tecnica` | a técnica de projeto do algoritmo | 1 | `Tecnica: Método guloso` |
| `Algoritmo` | o nome do algoritmo | 1 | `Algoritmo: Kruskal (árvore espalhada mínima)` |
| `Autores` | quem criou o algoritmo | 1 ou mais, separados por vírgula | `Autores: Joseph Kruskal` |
| `Data` | quando foi publicado | 1 | `Data: 1956` ou `Data: 12/03/1956` |
| `Conceitos` | os conceitos que o texto usa | 1 ou mais, separados por vírgula | `Conceitos: grafo, árvore, union-find` |
| `Problema` | o problema que o algoritmo resolve, numa frase | 1 | `Problema: Dado um grafo não dirigido ...` |

### Regras do cabeçalho

1. **Escreva as chaves exatamente como na tabela**, com `Tecnica` **sem
   acento** e com a primeira letra maiúscula.
2. **Cada chave aparece uma vez.** Se houver vários autores, coloque todos na
   mesma linha, separados por vírgula.
3. **Vários valores vão separados por vírgula.** Os espaços em volta das
   vírgulas não fazem diferença.
4. **A data é `dd/MM/AAAA` ou só o ano `AAAA`**, com o dia e o mês em dois
   dígitos (`05/03/1956`).
5. **Sem a informação, deixe vazio** (`Data:`) ou apague a linha.
6. **Use sempre o mesmo texto para o mesmo conceito** em todos os TXTs (combine
   com o grupo se vai ser `grafo` ou `grafos`, por exemplo). No `Problema`,
   use a mesma frase nos TXTs que resolvem o mesmo problema.

Depois do cabeçalho, uma linha **só com `---`** (três hífens).

---

## Parte 2: o corpo

Tudo depois do `---` é o texto da página. O começo de cada linha diz o que
ela é:

| A linha começa com | Vira | Exemplo |
|---|---|---|
| `# ` (cerquilha **e espaço**) | título de seção | `# Ideia do algoritmo` |
| `[ref]` | referência bibliográfica | `[ref] Cormen et al., Introduction to Algorithms, cap. 23` |
| qualquer outra coisa | parágrafo | `Ordena as arestas por peso, ...` |
| (linha em branco) | separa os parágrafos | |

E o código C:

```
[code]
int find(int parent[], int vertex)
{
    while (parent[vertex] != vertex)
        vertex = parent[vertex];
    return vertex;
}
[/code]
```

### Regras do corpo

1. **Um parágrafo = uma linha.** Escreva o parágrafo inteiro numa linha só e
   separe os parágrafos com uma linha em branco.
2. **`[code]` e `[/code]` ficam sozinhos na linha**, sem espaço na frente.
3. **Dentro do código, cole o código como ele é.** A indentação e as linhas em
   branco são mantidas.
4. **Feche todo `[code]` com `[/code]`.**
5. **Seção é `# ` com espaço** depois da cerquilha.
6. **Uma referência por linha**, cada uma começando com `[ref]`.

---

## Ao salvar

- Salve em **UTF-8**. É o padrão do VS Code. No Bloco de Notas, escolha
  "UTF-8" ao salvar.

---

## Checklist antes de entregar

- [ ] Comecei de uma cópia do `input/template.txt`.
- [ ] As 6 chaves estão escritas igual à tabela (`Tecnica` sem acento).
- [ ] Cada chave aparece uma vez.
- [ ] A data está como `AAAA` ou `dd/MM/AAAA`.
- [ ] Os conceitos estão escritos do jeito combinado com o grupo.
- [ ] Tem uma linha só com `---` depois do cabeçalho.
- [ ] Cada parágrafo está numa linha só.
- [ ] Todo `[code]` tem seu `[/code]`, os dois sozinhos na linha.
- [ ] O arquivo está salvo em UTF-8.
