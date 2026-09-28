# COMPLAC - Compilador para SLAC²

Projeto desenvolvido para a disciplina de Compiladores.

O objetivo é criar, em linguagem C, um compilador para a linguagem SLAC².
Esta entrega corresponde à **Fase 1**: análise léxica, análise sintática
(ASDR) e construção da tabela de símbolos.

## Integrantes

| Nome | RA |
|---|---:|
| Fabyani Tiva Yan | 10431835 |
| Bruna Amorim Maia | 10431883 |

## Compilação

```bash
make
```

Gera o binário `complac`. As flags usadas são exatamente `-Wall -Wextra -std=c99`,
e o projeto compila sem nenhum warning com o gcc.

Para remover objetos, binário e logs gerados nos testes:

```bash
make clean
```

## Execução

```bash
./complac <arquivo.slac> [--tokens] [--symtab] [--trace]
```

| Opção | Arquivo gerado | Conteúdo |
|---|---|---|
| `--tokens` | `<fonte>.tk` | `<L>  <CAT>  "<LEX>"` — um token por linha |
| `--symtab` | `<fonte>.ts` | `SCOPE=<descr>  id="<lexema>"  cat=<categ>  tipo=<tipo>  extra=<atrib>` |
| `--trace`  | `<fonte>.trc` | entrada (`>`) e saída (`<`) de cada não-terminal, indentadas |

As opções podem ser combinadas. Exemplo:

```bash
./complac testes/exemplo3.slac --tokens --symtab --trace
```

Em caso de sucesso o binário retorna `0`; em caso de erro léxico, sintático ou
de declaração duplicada, imprime a mensagem em `stderr` no formato
`<fonte>:<linha>: erro: <mensagem>` e retorna `1`.

Os programas de teste ficam em `testes/`. O alvo `make teste` executa todos eles
(três válidos e três com erros intencionais).

## Estrutura de módulos

| Arquivo | Papel | Interface pública |
|---|---|---|
| `main.c` | Orquestração e CLI. Não realiza análise. | — |
| `opt.c/.h` | Interpreta a linha de comando. | `opts_parse`, `opts_get`, `opts_uso` |
| `lex.c/.h` | Analisador léxico: um token por chamada. | `lex_init`, `lex_next`, `lex_nome_categoria`, `lex_erro` |
| `parser.c/.h` | ASDR estrito; abre e fecha escopos. | `parse_program` |
| `symtab.c/.h` | Tabela de símbolos com escopos encadeados. | `ts_insert`, `ts_lookup`, `ts_init`, `ts_dump`, … |
| `diag.c/.h` | Centraliza erros e rastreamento. | `diag_error`, `diag_info`, `diag_init`, `diag_ok` |
| `log.c/.h` | Gera os arquivos `.tk`, `.ts` e `.trc`. | `log_abrir`, `log_token`, `log_simbolo`, `log_trace`, `log_fechar` |

A comunicação entre módulos ocorre exclusivamente pelos cabeçalhos públicos.
Nenhum estado interno é compartilhado: `lex`, `parser` e `symtab` mantêm seus
dados privados e nunca escrevem na saída diretamente — todas as mensagens
passam por `diag`.

Fluxo: `main` lê as opções, abre o fonte e inicializa os módulos; `parse_program`
consome tokens de `lex_next` conforme expande as produções, registrando
identificadores na TS; erros interrompem o processo via `diag_error`, que fecha
os logs antes de encerrar; ao final, `main` despeja a TS e finaliza tudo em ordem.

## Particularidades da implementação

**Escopos.** A tabela de símbolos é uma cadeia de escopos com ponteiro para o pai.
A busca (`ts_lookup`) percorre do escopo mais interno até o global, reproduzindo a
visibilidade da SLAC². Os nomes seguem o formato pedido no enunciado:
`global`, `fn:SOMA.locals`, `proc:main.block#1`. Parâmetros e `locvars`
compartilham o mesmo escopo (`.locals`), já que na linguagem eles têm as mesmas
regras de visibilidade; o bloco `start...end` abre um escopo filho.

**Campo `extra`.** Guarda o tamanho declarado, no caso de vetores, e a quantidade
de parâmetros, no caso de sub-rotinas. Como o tipo em SLAC² aparece no fim da
declaração, os identificadores só são inseridos na TS depois de `is <tipo>;`.

**Lookahead.** O ASDR trabalha com um único token de lookahead, suficiente para
toda a gramática. O caso que exigiria decisão antecipada — um comando iniciado
por identificador, que pode ser `<call>` ou `<atr>` — é resolvido consumindo o
identificador e inspecionando o token seguinte (`(`, `[` ou `<<`). Não foi
necessário alterar o analisador léxico.

**`<elem>` e `<fact>`.** A regra `<elem> ::= <litl> | <id> | <vetr> | <call> | <expr>`
é circular em relação a `<fact>`, que já deriva `<elem>`. Na implementação as duas
foram unificadas em `fact()`, que reconhece literal, identificador, elemento de
vetor, chamada de sub-rotina, expressão entre parênteses e os operadores unários
`~` e `-` (associativos à direita). A hierarquia `expr → elogc → erlac → earit →
earip → fact` reproduz exatamente a tabela de precedência da especificação, com
os operadores binários associativos à esquerda.

**Índice de vetor.** Conforme a EBNF (`<vetr> ::= <id> "[" ("sCTEINT" | <id>) "]"`),
o índice aceita apenas uma constante inteira ou um identificador, não uma
expressão completa.

**`main`.** É exigido que o programa defina o procedimento `main`, sem parâmetros,
e que ele seja a última sub-rotina do arquivo, como determina
`<prg> ::= [<gvars>] {<subs>} <princ>`.

**Verificações adiadas.** A checagem de tipos e o uso de identificadores não
declarados pertencem à análise semântica (Fase 2). Nesta entrega a TS já registra
tudo o que será necessário e reporta declarações duplicadas no mesmo escopo.
