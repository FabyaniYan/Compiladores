#include "parser.h"
#include "diag.h"
#include "log.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IDS_DECL 64 /*identificadores por instrucao de declaracao*/

/*estado interno do parser: nao e exposto a nenhum outro modulo*/
static Lex *lexico = NULL;
static Token atual;       /*token de lookahead (um unico simbolo)*/
static int profundidade;  /*indentacao das mensagens de rastreamento*/
static int viu_main;      /*indica que o procedimento main ja foi analisado*/
static int blocos_sub;    /*contador de blocos start...end da sub-rotina atual*/
static char sub_atual[TOKEN_LEXEMA_MAX + 16]; /*prefixo de escopo, ex.: "proc:main"*/

/*prototipos dos nao-terminais (a gramatica e mutuamente recursiva)*/
static void gvars(void);
static void lvars(void);
static void decls(CategoriaSimbolo categoria);
static TipoSimbolo tipo(int *tamanho);
static void subrotina(void);
static int param(void);
static void bco(void);
static void cmd(void);
static void cmd_echo(void);
static void cmd_get(void);
static void cmd_case(void);
static void cmd_choose(void);
static void cmd_for(void);
static void cmd_while(void);
static void cmd_repeat(void);
static void cmd_return(void);
static void cmd_id(void);
static void args_chamada(void);
static void indice_vetor(void);
static void expr(void);
static void elogc(void);
static void erlac(void);
static void earit(void);
static void earip(void);
static void fact(void);

/* ------------------------------------------------------------------ */
/* Apoio: consumo de tokens e rastreamento                             */
/* ------------------------------------------------------------------ */

static void entrar(const char *nao_terminal) {
    /*registra a entrada em um nao-terminal no arquivo de rastreamento*/
    diag_info("%*s> %s", profundidade * 2, "", nao_terminal);
    profundidade++;
}

static void sair(const char *nao_terminal) {
    if (profundidade > 0) profundidade--;
    diag_info("%*s< %s", profundidade * 2, "", nao_terminal);
}

static void avancar(void) {
    /*pede o proximo token ao lex, registra no log e aborta em erro lexico*/
    atual = lex_next(lexico);
    log_token(atual.linha, lex_nome_categoria(atual.categoria), atual.lexema);
    if (atual.categoria == sERRO) diag_error(atual.linha, "%s", lex_erro(lexico));
}

static int aceitar(CategoriaToken categoria) {
    /*consome o token se ele for da categoria esperada*/
    if (atual.categoria != categoria) return 0;
    avancar();
    return 1;
}

static Token consumir(CategoriaToken categoria) {
    /*exige o token esperado; caso contrario reporta esperado x encontrado*/
    Token t = atual;
    if (atual.categoria != categoria)
        diag_error(atual.linha, "esperado %s, encontrado %s \"%s\"",
                   lex_nome_categoria(categoria),
                   lex_nome_categoria(atual.categoria), atual.lexema);
    avancar();
    return t;
}

static int inicia_comando(CategoriaToken c) {
    /*conjunto First(<cmd>)*/
    return c == sECHO || c == sGET || c == sCASE || c == sCHOOSE ||
           c == sFOR || c == sWHILE || c == sREPEAT || c == sRETURN ||
           c == sIDENTIF;
}

static TipoSimbolo tipo_vetor(TipoSimbolo base) {
    if (base == TS_INT) return TS_INT_VET;
    if (base == TS_LOGIC) return TS_LOGIC_VET;
    return TS_CHR_VET;
}

/* ------------------------------------------------------------------ */
/* <prg> ::= [<gvars>] {<subs>} <princ>                                */
/* ------------------------------------------------------------------ */

void parse_program(Lex *lex) {
    lexico = lex;
    profundidade = 0;
    viu_main = 0;
    sub_atual[0] = '\0';

    ts_init();
    avancar(); /*carrega o primeiro token de lookahead*/

    entrar("prg");
    if (atual.categoria == sGLOBVARS) gvars();

    /*sub-rotinas opcionais seguidas, obrigatoriamente, do proc main*/
    while (atual.categoria == sFUNC || atual.categoria == sPROC) {
        if (viu_main)
            diag_error(atual.linha, "nenhuma sub-rotina pode ser declarada depois de main");
        subrotina();
    }

    if (!viu_main)
        diag_error(atual.linha, "programa nao define o procedimento main");

    consumir(sEOF);
    sair("prg");
}

/* <gvars> ::= "sGLOBVARS" <decls>{<decls>} */
static void gvars(void) {
    entrar("gvars");
    consumir(sGLOBVARS);
    if (atual.categoria != sIDENTIF)
        diag_error(atual.linha, "a secao globvars exige ao menos uma declaracao");
    while (atual.categoria == sIDENTIF) decls(TS_VAR_GLOBAL);
    sair("gvars");
}

/* <lvars> ::= "sLOCVARS" <decls>{<decls>} */
static void lvars(void) {
    entrar("lvars");
    consumir(sLOCVARS);
    if (atual.categoria != sIDENTIF)
        diag_error(atual.linha, "a secao locvars exige ao menos uma declaracao");
    while (atual.categoria == sIDENTIF) decls(TS_VAR_LOCAL);
    sair("lvars");
}

/* <decls> ::= <id> {"," <id>} "sKIND" <type> ";" */
static void decls(CategoriaSimbolo categoria) {
    char nomes[MAX_IDS_DECL][TOKEN_LEXEMA_MAX];
    int linhas[MAX_IDS_DECL];
    int n = 0, i, tamanho;
    TipoSimbolo t;

    entrar("decls");
    do {
        Token id = consumir(sIDENTIF);
        if (n >= MAX_IDS_DECL)
            diag_error(id.linha, "declaracao com identificadores demais");
        snprintf(nomes[n], TOKEN_LEXEMA_MAX, "%s", id.lexema);
        linhas[n] = id.linha;
        n++;
    } while (aceitar(sVIRGULA));

    consumir(sKIND);
    t = tipo(&tamanho);
    consumir(sPONTOVIRGULA);

    /*so registramos na TS depois de conhecer o tipo, que vem no fim da regra*/
    for (i = 0; i < n; i++) ts_insert(nomes[i], categoria, t, tamanho, linhas[i]);
    sair("decls");
}

/* <type> ::= ("sINT" | "sLOGIC" | "sCHR") ["[" "sCTEINT" "]"] */
static TipoSimbolo tipo(int *tamanho) {
    TipoSimbolo base;

    entrar("type");
    if (aceitar(sINT)) base = TS_INT;
    else if (aceitar(sLOGIC)) base = TS_LOGIC;
    else if (aceitar(sCHR)) base = TS_CHR;
    else {
        diag_error(atual.linha, "esperado um tipo (int, logic ou chr), encontrado %s \"%s\"",
                   lex_nome_categoria(atual.categoria), atual.lexema);
        base = TS_INT; /*inalcancavel: diag_error encerra o programa*/
    }

    *tamanho = 0;
    if (aceitar(sABRECOL)) {
        Token n = consumir(sCTEINT);
        *tamanho = atoi(n.lexema);
        if (*tamanho <= 0)
            diag_error(n.linha, "tamanho de vetor deve ser maior que zero");
        consumir(sFECHACOL);
        base = tipo_vetor(base);
    }
    sair("type");
    return base;
}

/*
 * <subs> ::= (<func> | <proc>)
 * <func> ::= "sFUNC" <id> "(" [<param>] ")" ":" <type> [<lvars>] <bco>
 * <proc> ::= "sPROC" <id> "(" [<param>] ")" [<lvars>] <bco>
 */
static void subrotina(void) {
    int e_funcao = (atual.categoria == sFUNC);
    char escopo[TS_MAX_NOME];
    Token id;
    int idx, qtd_param = 0, tamanho;

    entrar(e_funcao ? "func" : "proc");
    avancar(); /*consome sFUNC ou sPROC*/
    id = consumir(sIDENTIF);

    /*o nome da sub-rotina pertence ao escopo global*/
    idx = ts_insert(id.lexema, e_funcao ? TS_FUNC : TS_PROC, TS_VOID, 0, id.linha);

    snprintf(sub_atual, sizeof(sub_atual), "%s:%s", e_funcao ? "fn" : "proc", id.lexema);
    blocos_sub = 0;

    /*parametros e variaveis locais compartilham o mesmo escopo*/
    snprintf(escopo, TS_MAX_NOME, "%s.locals", sub_atual);
    ts_abre_escopo(escopo);

    consumir(sABREPAR);
    if (atual.categoria != sFECHAPAR) qtd_param = param();
    consumir(sFECHAPAR);
    ts_set_extra(idx, qtd_param);

    if (e_funcao) {
        consumir(sDOISPONTOS);
        ts_set_tipo(idx, tipo(&tamanho));
    } else if (strcmp(id.lexema, "main") == 0) {
        /*main deve ser um procedimento sem parametros*/
        if (qtd_param != 0)
            diag_error(id.linha, "o procedimento main nao pode receber parametros");
        viu_main = 1;
    }

    if (atual.categoria == sLOCVARS) lvars();
    bco();

    ts_fecha_escopo();
    sair(e_funcao ? "func" : "proc");
}

/* <param> ::= ["sREF"] <id> ":" <type> {"," ["sREF"] <id> ":" <type>} */
static int param(void) {
    int n = 0, tamanho;
    TipoSimbolo t;

    entrar("param");
    do {
        int por_referencia = aceitar(sREF);
        Token id = consumir(sIDENTIF);
        consumir(sDOISPONTOS);
        t = tipo(&tamanho);
        /*vetores sao sempre passados por referencia, conforme a especificacao*/
        if (t == TS_INT_VET || t == TS_LOGIC_VET || t == TS_CHR_VET) por_referencia = 1;
        ts_insert(id.lexema, por_referencia ? TS_PARAM_REF : TS_PARAM, t, tamanho, id.linha);
        n++;
    } while (aceitar(sVIRGULA));
    sair("param");
    return n;
}

/* <bco> ::= "sSTART" {<cmd> ";"} "sEND" */
static void bco(void) {
    char escopo[TS_MAX_NOME];

    entrar("bco");
    blocos_sub++;
    snprintf(escopo, TS_MAX_NOME, "%s.block#%d", sub_atual, blocos_sub);
    ts_abre_escopo(escopo);

    consumir(sSTART);
    while (atual.categoria != sEND) {
        if (!inicia_comando(atual.categoria))
            diag_error(atual.linha, "esperado sEND ou um comando, encontrado %s \"%s\"",
                       lex_nome_categoria(atual.categoria), atual.lexema);
        cmd();
        consumir(sPONTOVIRGULA);
    }
    consumir(sEND);

    ts_fecha_escopo();
    sair("bco");
}

/*
 * <cmd> := <echo> | <get> | <case> | <chse> | <for> | <whle> |
 *          <rept> | <call> | <ret> | <atr>
 */
static void cmd(void) {
    entrar("cmd");
    switch (atual.categoria) {
        case sECHO:    cmd_echo();   break;
        case sGET:     cmd_get();    break;
        case sCASE:    cmd_case();   break;
        case sCHOOSE:  cmd_choose(); break;
        case sFOR:     cmd_for();    break;
        case sWHILE:   cmd_while();  break;
        case sREPEAT:  cmd_repeat(); break;
        case sRETURN:  cmd_return(); break;
        case sIDENTIF: cmd_id();     break;
        default:
            diag_error(atual.linha, "esperado um comando, encontrado %s \"%s\"",
                       lex_nome_categoria(atual.categoria), atual.lexema);
    }
    sair("cmd");
}

/* <echo> ::= "sECHO" "(" <elem> {"," <elem>} ")" */
static void cmd_echo(void) {
    entrar("echo");
    consumir(sECHO);
    consumir(sABREPAR);
    do { expr(); } while (aceitar(sVIRGULA));
    consumir(sFECHAPAR);
    sair("echo");
}

/* <get> ::= "sGET" "(" (<id> | <vetr>) ")" */
static void cmd_get(void) {
    entrar("get");
    consumir(sGET);
    consumir(sABREPAR);
    consumir(sIDENTIF);
    if (atual.categoria == sABRECOL) indice_vetor();
    consumir(sFECHAPAR);
    sair("get");
}

/*
 * <case> ::= "sCASE" "(" <expr> ")" {<cmd> ";"}
 *            ["sOTHERWISE" {<cmd> ";"}] "sEND"
 */
static void cmd_case(void) {
    entrar("case");
    consumir(sCASE);
    consumir(sABREPAR);
    expr();
    consumir(sFECHAPAR);

    while (inicia_comando(atual.categoria)) { cmd(); consumir(sPONTOVIRGULA); }

    if (aceitar(sOTHERWISE))
        while (inicia_comando(atual.categoria)) { cmd(); consumir(sPONTOVIRGULA); }

    consumir(sEND);
    sair("case");
}

/*
 * <chse>  ::= "sCHOOSE" "(" <expr> ")" <mlst> "sEND"
 * <mlst>  ::= <mtch> {<mtch>} [<othr>]
 * <mtch>  ::= "sMATCH" <mvlrs> "sIMPLIC" <cmd> ";"
 * <mvlrs> ::= "sCTEINT" {"," "sCTEINT"}
 * <othr>  ::= "sOTHERS" "sIMPLIC" <cmd> ";"
 */
static void cmd_choose(void) {
    entrar("chse");
    consumir(sCHOOSE);
    consumir(sABREPAR);
    expr();
    consumir(sFECHAPAR);

    if (atual.categoria != sMATCH)
        diag_error(atual.linha, "o comando choose exige ao menos uma clausula match");

    while (atual.categoria == sMATCH) {
        entrar("mtch");
        consumir(sMATCH);
        do {
            aceitar(sSUBRAT); /*admite valores negativos nas alternativas*/
            consumir(sCTEINT);
        } while (aceitar(sVIRGULA));
        consumir(sIMPLIC);
        cmd();
        consumir(sPONTOVIRGULA);
        sair("mtch");
    }

    if (aceitar(sOTHERS)) {
        entrar("othr");
        consumir(sIMPLIC);
        cmd();
        consumir(sPONTOVIRGULA);
        sair("othr");
    }

    consumir(sEND);
    sair("chse");
}

/*valor usado nos limites do for: um identificador ou uma constante inteira*/
static void valor_for(void) {
    if (atual.categoria == sIDENTIF) { avancar(); return; }
    aceitar(sSUBRAT);
    consumir(sCTEINT);
}

/*
 * <for> ::= "sFOR" <id> "sFROM" (<id>|"sCTEINT") "sTO" (<id>|"sCTEINT")
 *           ["sBY" "sCTEINT"] "sDO" {<cmd> ";"} "sEND"
 */
static void cmd_for(void) {
    entrar("for");
    consumir(sFOR);
    consumir(sIDENTIF);
    consumir(sFROM);
    valor_for();
    consumir(sTO);
    valor_for();
    if (aceitar(sBY)) { aceitar(sSUBRAT); consumir(sCTEINT); }
    consumir(sDO);
    while (inicia_comando(atual.categoria)) { cmd(); consumir(sPONTOVIRGULA); }
    consumir(sEND);
    sair("for");
}

/* <whle> ::= "sWHILE" "(" <expr> ")" "sDO" {<cmd> ";"} "sEND" */
static void cmd_while(void) {
    entrar("whle");
    consumir(sWHILE);
    consumir(sABREPAR);
    expr();
    consumir(sFECHAPAR);
    consumir(sDO);
    while (inicia_comando(atual.categoria)) { cmd(); consumir(sPONTOVIRGULA); }
    consumir(sEND);
    sair("whle");
}

/* <rept> ::= "sREPEAT" {<cmd> ";"} "sUNTIL" "(" <expr> ")" */
static void cmd_repeat(void) {
    entrar("rept");
    consumir(sREPEAT);
    while (inicia_comando(atual.categoria)) { cmd(); consumir(sPONTOVIRGULA); }
    consumir(sUNTIL);
    consumir(sABREPAR);
    expr();
    consumir(sFECHAPAR);
    sair("rept");
}

/* <ret> ::= "sRETURN" <elem> */
static void cmd_return(void) {
    entrar("ret");
    consumir(sRETURN);
    expr();
    sair("ret");
}

/*
 * Comando iniciado por identificador. Um unico token de lookahead
 * basta para distinguir os dois casos:
 *   <call> ::= <id> "(" [<expr> {"," <expr>}] ")"
 *   <atr>  ::= (<id> | <vetr>) "sATRIB" <elem>
 */
static void cmd_id(void) {
    consumir(sIDENTIF);
    if (atual.categoria == sABREPAR) {
        entrar("call");
        args_chamada();
        sair("call");
        return;
    }
    entrar("atr");
    if (atual.categoria == sABRECOL) indice_vetor();
    consumir(sATRIB);
    expr();
    sair("atr");
}

/*lista de argumentos de uma chamada, a partir do parentese de abertura*/
static void args_chamada(void) {
    consumir(sABREPAR);
    if (atual.categoria != sFECHAPAR)
        do { expr(); } while (aceitar(sVIRGULA));
    consumir(sFECHAPAR);
}

/* <vetr> ::= <id> "[" ("sCTEINT" | <id>) "]" (a partir do colchete) */
static void indice_vetor(void) {
    consumir(sABRECOL);
    if (atual.categoria == sCTEINT || atual.categoria == sIDENTIF) avancar();
    else
        diag_error(atual.linha, "esperado sCTEINT ou sIDENTIF como indice, encontrado %s \"%s\"",
                   lex_nome_categoria(atual.categoria), atual.lexema);
    consumir(sFECHACOL);
}

/* ------------------------------------------------------------------ */
/* Hierarquia de expressoes (da menor para a maior precedencia)        */
/* ------------------------------------------------------------------ */

/* <expr> ::= <elogc> {"sOR" <elogc>} */
static void expr(void) {
    entrar("expr");
    elogc();
    while (aceitar(sOR)) elogc();
    sair("expr");
}

/* <elogc> ::= <erlac> {"sAND" <erlac>} */
static void elogc(void) {
    entrar("elogc");
    erlac();
    while (aceitar(sAND)) erlac();
    sair("elogc");
}

/* <erlac> ::= <earit> {<oprel> <earit>} */
static void erlac(void) {
    entrar("erlac");
    earit();
    while (atual.categoria == sMAIOR || atual.categoria == sMAIORIGUAL ||
           atual.categoria == sMENOR || atual.categoria == sMENORIGUAL ||
           atual.categoria == sIGUAL || atual.categoria == sDIFERENTE) {
        avancar();
        earit();
    }
    sair("erlac");
}

/* <earit> ::= <earip> {<opari> <earip>} */
static void earit(void) {
    entrar("earit");
    earip();
    while (atual.categoria == sSOMA || atual.categoria == sSUBRAT) {
        avancar();
        earip();
    }
    sair("earit");
}

/* <earip> ::= <fact> {<oparp> <fact>} */
static void earip(void) {
    entrar("earip");
    fact();
    while (atual.categoria == sMULT || atual.categoria == sDIV) {
        avancar();
        fact();
    }
    sair("earip");
}

/*
 * <fact> ::= <elem> | "sNEG" <fact> | "sSUBRAT" <fact> | "(" <expr> ")"
 *
 * <elem> engloba literais, identificadores, elementos de vetor, chamadas de
 * sub-rotina e expressoes entre parenteses. Os operadores unarios sao
 * associativos a direita, o que a recursao abaixo reproduz naturalmente.
 */
static void fact(void) {
    entrar("fact");
    switch (atual.categoria) {
        case sNEG:
        case sSUBRAT:
            avancar();
            fact();
            break;
        case sABREPAR:
            avancar();
            expr();
            consumir(sFECHAPAR);
            break;
        case sCTEINT:
        case sCTECHAR:
        case sSTRING:
            avancar();
            break;
        case sIDENTIF:
            avancar();
            if (atual.categoria == sABREPAR) args_chamada();      /*<call>*/
            else if (atual.categoria == sABRECOL) indice_vetor(); /*<vetr>*/
            break;
        default:
            diag_error(atual.linha, "esperado um operando, encontrado %s \"%s\"",
                       lex_nome_categoria(atual.categoria), atual.lexema);
    }
    sair("fact");
}
