#include "symtab.h"
#include "diag.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char nome[TS_MAX_NOME]; /*nome completo do escopo, ex.: "proc:main.locals"*/
    int pai;                /*indice do escopo pai; -1 para o global*/
} Escopo;

static Simbolo simbolos[TS_MAX_SIMBOLOS];
static int total_simbolos = 0;

static Escopo escopos[TS_MAX_ESCOPOS];
static int total_escopos = 0;
static int escopo_ativo = 0;

void ts_init(void) {
    total_simbolos = 0;
    total_escopos = 1;
    escopo_ativo = 0;
    snprintf(escopos[0].nome, TS_MAX_NOME, "global");
    escopos[0].pai = -1;
}

void ts_abre_escopo(const char *nome) {
    int novo;

    if (total_escopos >= TS_MAX_ESCOPOS)
        diag_error(0, "limite de escopos da tabela de simbolos excedido");

    novo = total_escopos++;
    /*o nome ja chega pronto (ex.: "proc:main.block#1"); aqui so guardamos a cadeia*/
    snprintf(escopos[novo].nome, TS_MAX_NOME, "%s", nome);
    escopos[novo].pai = escopo_ativo;
    escopo_ativo = novo;
}

void ts_fecha_escopo(void) {
    if (escopos[escopo_ativo].pai >= 0) escopo_ativo = escopos[escopo_ativo].pai;
}

const char *ts_escopo_atual(void) { return escopos[escopo_ativo].nome; }

int ts_lookup_local(const char *lexema) {
    int i;
    for (i = 0; i < total_simbolos; i++)
        if (simbolos[i].escopo == escopo_ativo && strcmp(simbolos[i].lexema, lexema) == 0)
            return i;
    return -1;
}

int ts_lookup(const char *lexema) {
    int e, i;
    /*percorre do escopo mais interno ate o global, como manda a visibilidade*/
    for (e = escopo_ativo; e >= 0; e = escopos[e].pai)
        for (i = total_simbolos - 1; i >= 0; i--)
            if (simbolos[i].escopo == e && strcmp(simbolos[i].lexema, lexema) == 0)
                return i;
    return -1;
}

int ts_insert(const char *lexema, CategoriaSimbolo cat, TipoSimbolo tipo,
              int extra, int linha) {
    int i;

    /*a linguagem nao admite dois identificadores iguais no mesmo escopo*/
    if (ts_lookup_local(lexema) >= 0)
        diag_error(linha, "identificador \"%s\" ja declarado no escopo %s",
                   lexema, escopos[escopo_ativo].nome);

    if (total_simbolos >= TS_MAX_SIMBOLOS)
        diag_error(linha, "limite de simbolos da tabela excedido");

    i = total_simbolos++;
    snprintf(simbolos[i].lexema, TOKEN_LEXEMA_MAX, "%s", lexema);
    simbolos[i].categoria = cat;
    simbolos[i].tipo = tipo;
    simbolos[i].escopo = escopo_ativo;
    simbolos[i].extra = extra;
    return i;
}

const Simbolo *ts_get(int indice) {
    if (indice < 0 || indice >= total_simbolos) return NULL;
    return &simbolos[indice];
}

void ts_set_extra(int indice, int extra) {
    if (indice >= 0 && indice < total_simbolos) simbolos[indice].extra = extra;
}

void ts_set_tipo(int indice, TipoSimbolo tipo) {
    if (indice >= 0 && indice < total_simbolos) simbolos[indice].tipo = tipo;
}

const char *ts_nome_categoria(CategoriaSimbolo cat) {
    switch (cat) {
        case TS_VAR_GLOBAL: return "varglobal";
        case TS_VAR_LOCAL:  return "varlocal";
        case TS_PARAM:      return "param";
        case TS_PARAM_REF:  return "paramref";
        case TS_FUNC:       return "func";
        case TS_PROC:       return "proc";
        default:            return "?";
    }
}

const char *ts_nome_tipo(TipoSimbolo tipo) {
    switch (tipo) {
        case TS_INT:       return "int";
        case TS_LOGIC:     return "logic";
        case TS_CHR:       return "chr";
        case TS_INT_VET:   return "int[]";
        case TS_LOGIC_VET: return "logic[]";
        case TS_CHR_VET:   return "chr[]";
        case TS_VOID:      return "void";
        default:           return "?";
    }
}

void ts_dump(void) {
    int e, i;
    if (!log_symtab_ativo()) return;
    /*escopos na ordem de criacao; dentro de cada um, ordem de insercao*/
    for (e = 0; e < total_escopos; e++)
        for (i = 0; i < total_simbolos; i++)
            if (simbolos[i].escopo == e)
                log_simbolo(escopos[e].nome, simbolos[i].lexema,
                            ts_nome_categoria(simbolos[i].categoria),
                            ts_nome_tipo(simbolos[i].tipo),
                            simbolos[i].extra);
}
