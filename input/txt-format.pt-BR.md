# Como escrever os TXTs do EP1

Cada TXT vira uma página do site. O programa lê o arquivo, separa as
informações e monta a página e os índices (Técnica, Algoritmo, Autores, Data,
Conceitos, Problema). Ele **não adivinha nada**: se o arquivo sair do formato,
a informação some ou vai parar no lugar errado, muitas vezes sem aviso.

Por isso o jeito mais seguro de começar é **copiar o `input/template.txt` e
trocar o conteúdo**.

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

1. **Escreva as chaves exatamente como na tabela.** `Tecnica` é **sem acento**,
   e maiúsculas e minúsculas contam. `Técnica:`, `tecnica:` ou `Tecnicas:` não
   são reconhecidas, e o campo **some sem aviso**.
2. **Cada chave aparece uma vez só.** Se houver dois `Autores:`, o resultado
   não está definido. Ponha todos os autores na mesma linha, separados por
   vírgula.
3. **Vários valores vão separados por vírgula.** Os espaços em volta das
   vírgulas não importam: `grafo,árvore` e `grafo , árvore` dão o mesmo
   resultado.
4. **A data é `dd/MM/AAAA` ou só o ano `AAAA`.** O programa ordena por data de
   verdade, e um ano sozinho vem antes das datas completas do mesmo ano.
   Escreva o dia e o mês com dois dígitos (`05/03/1956`, não `5/3/1956`).
5. **Sem a informação, deixe vazio** (`Data:`) ou apague a linha. O campo é
   ignorado.
6. **Mesmo texto, mesma chave.** Nos índices, `Grafos` e `grafos` ficam juntos,
   mas `grafo` e `grafos` ficam separados. Combine com o grupo como escrever
   cada conceito. No `Problema`, só ficam juntos os TXTs que usarem
   **exatamente a mesma frase**.

Depois do cabeçalho, uma linha **só com `---`** (três hífens). Espaços
sobrando nas pontas não atrapalham, mas texto na mesma linha sim.

---

## Parte 2: o corpo

Tudo depois do `---` é o texto da página. O começo de cada linha diz o que
ela é:

| A linha começa com | Vira | Exemplo |
|---|---|---|
| `# ` (cerquilha **e espaço**) | título de seção | `# Ideia do algoritmo` |
| `[ref]` | referência bibliográfica | `[ref] Cormen et al., Introduction to Algorithms, cap. 23` |
| qualquer outra coisa | parágrafo | `Ordena as arestas por peso, ...` |
| (linha em branco) | nada, só separa | |

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

1. **Um parágrafo = uma linha.** Escreva o parágrafo inteiro numa linha só,
   sem quebrar no meio, e separe os parágrafos com uma linha em branco. Hoje
   cada linha vira um parágrafo, então um parágrafo quebrado em três linhas
   aparece como três parágrafos.
2. **`[code]` e `[/code]` ficam sozinhos na linha**, sem espaço na frente e
   sem nada depois. `  [code]` (com espaço) ou `[code] int x;` não são
   reconhecidos e viram parágrafo.
3. **Dentro do código vale tudo:** a indentação, as linhas em branco e os
   caracteres como `<`, `>` e `&` são mantidos como estão. Cole o código como
   ele é.
4. **Feche todo `[code]` com `[/code]`.** Um bloco aberto e nunca fechado
   **some** da página.
5. **Seção é `# ` com espaço.** `#Ideia` (sem espaço) vira parágrafo.
6. **Uma referência por linha**, cada uma começando com `[ref]`.

---

## Limites que valem para o arquivo inteiro

- **Nenhuma linha pode passar de 255 caracteres.** Isso vale principalmente
  para o `Problema` e para os parágrafos longos. Uma linha maior é **cortada em
  duas**: o parágrafo vira dois, ou uma linha de código ganha uma quebra no
  meio. Se o parágrafo ficar grande, divida em dois parágrafos.
- **Salve em UTF-8.** É o padrão do VS Code. No Bloco de Notas, escolha
  "UTF-8" ao salvar.
- **Fim de linha Windows ou Linux tanto faz.** O programa aceita os dois.

---

## Erros comuns e o que acontece

| Erro | O que acontece |
|---|---|
| `Técnica:` com acento | a técnica some da página e do índice |
| `Autores` repetido em duas linhas | o resultado não está definido |
| `Data: 5/3/1956` | a data pode ser ordenada errado |
| esquecer o `---` | o corpo inteiro **some** |
| `---` com texto na mesma linha (`--- corpo`) | o separador não é reconhecido |
| parágrafo quebrado em várias linhas | vira vários parágrafos |
| `  [code]` com espaço na frente | o código vira parágrafo |
| `[code]` sem `[/code]` | o código some |
| linha com mais de 255 caracteres | a linha é cortada em duas |

---

## Checklist antes de entregar

- [ ] Comecei de uma cópia do `input/template.txt`.
- [ ] As 6 chaves estão escritas igual à tabela (`Tecnica` sem acento).
- [ ] Nenhuma chave aparece duas vezes.
- [ ] A data está como `AAAA` ou `dd/MM/AAAA`.
- [ ] Os conceitos estão escritos do jeito combinado com o grupo.
- [ ] Tem uma linha só com `---` depois do cabeçalho.
- [ ] Cada parágrafo está numa linha só.
- [ ] Todo `[code]` tem seu `[/code]`, os dois sozinhos na linha.
- [ ] Nenhuma linha passa de 255 caracteres.
- [ ] O arquivo está salvo em UTF-8.

---

## Ainda em aberto no grupo

Estes pontos podem mudar, e quando mudarem este guia muda junto:

- se linhas de texto seguidas devem virar **um** parágrafo (hoje cada linha é
  um parágrafo);
- onde as referências `[ref]` aparecem na página: no fim ou no lugar em que
  estão;
- a codificação oficial (UTF-8 é a que funciona hoje);
- o que o programa faz quando um TXT sai do formato: avisa e segue, ou
  descarta o arquivo.
