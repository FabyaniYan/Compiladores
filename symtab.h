#ifndef SYMTAB_H
#define SYMTAB_H

#include "lex.h"

/*
 * Modulo symtab - tabela de simbolos com controle de escopo.
 *
 * Os escopos espelham a estrutura da SLAC2:
 *   global                  -> secao globvars e nomes de sub-rotinas
 *   proc:main.locals        -> parametros e locvars de uma sub-rotina
 *   proc:main.block#1       -> bloco start...end da sub-rotina
 *
 * A visibilidade segue a cadeia de escopos ativos (do mais interno ao global).
 */

#define TS_MAX_SIMBOLOS 1024
#define TS_MAX_ESCOPOS  256
#define TS_MAX_NOME     512

/*categorias de identificador registradas na TS*/
typedef enum {
    TS_VAR_GLOBAL,
    TS_VAR_LOCAL,
    TS_PARAM,
    TS_PARAM_REF,
    TS_FUNC,
    TS_PROC
} CategoriaSimbolo;

/*tipos de dado da SLAC2; os variantes VET representam vetores*/
typedef enum {
    TS_INT, TS_LOGIC, TS_CHR,
    TS_INT_VET, TS_LOGIC_VET, TS_CHR_VET,
    TS_VOID
} TipoSimbolo;

typedef struct {
    char lexema[TOKEN_LEXEMA_MAX];
    CategoriaSimbolo categoria;
    TipoSimbolo tipo;
    int escopo;  /*indice do escopo em que foi declarado*/
    int extra;   /*tamanho do vetor ou quantidade de parametros da sub-rotina*/
} Simbolo;

/*prepara a tabela e cria o escopo global*/
void ts_init(void);

/*
 * Abre um novo escopo, filho do escopo atual.
 * O nome recebido e o caminho completo que aparecera nos relatorios,
 * por exemplo "fn:SOMA.locals" ou "proc:main.block#1".
 */
void ts_abre_escopo(const char *nome);

/*encerra o escopo atual, voltando ao escopo pai*/
void ts_fecha_escopo(void);

/*nome completo do escopo ativo (ex.: "proc:main.locals")*/
const char *ts_escopo_atual(void);

/*
 * Insere um identificador no escopo atual.
 * Declaracao duplicada no mesmo escopo e reportada via diag_error.
 * Devolve o indice do simbolo inserido.
 */
int ts_insert(const char *lexema, CategoriaSimbolo cat, TipoSimbolo tipo,
              int extra, int linha);

/*busca o identificador na cadeia de escopos visiveis; -1 se nao encontrado*/
int ts_lookup(const char *lexema);

/*busca restrita ao escopo atual; -1 se nao encontrado*/
int ts_lookup_local(const char *lexema);

/*acesso somente leitura a um simbolo pelo indice*/
const Simbolo *ts_get(int indice);

/*ajusta o campo extra de um simbolo ja inserido (usado na contagem de parametros)*/
void ts_set_extra(int indice, int extra);

/*ajusta o tipo de um simbolo ja inserido (tipo de retorno de funcao)*/
void ts_set_tipo(int indice, TipoSimbolo tipo);

/*textos usados nos relatorios*/
const char *ts_nome_categoria(CategoriaSimbolo cat);
const char *ts_nome_tipo(TipoSimbolo tipo);

/*percorre a TS na ordem de escopo e de insercao, enviando tudo ao modulo log*/
void ts_dump(void);

#endif
