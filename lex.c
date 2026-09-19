#include "lex.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct { const char *texto; CategoriaToken categoria; } PalavraReservada;

/*tabela das palavras reservadas da linguagem*/
static const PalavraReservada palavras[] = {
    {"globvars", sGLOBVARS}, {"locvars", sLOCVARS}, {"is", sKIND},
    {"int", sINT}, {"logic", sLOGIC}, {"chr", sCHR}, {"proc", sPROC},
    {"func", sFUNC}, {"ref", sREF}, {"start", sSTART}, {"end", sEND},
    {"echo", sECHO}, {"get", sGET}, {"case", sCASE}, {"otherwise", sOTHERWISE},
    {"choose", sCHOOSE}, {"match", sMATCH}, {"others", sOTHERS},
    {"for", sFOR}, {"from", sFROM}, {"to", sTO}, {"by", sBY}, {"do", sDO},
    {"while", sWHILE}, {"repeat", sREPEAT}, {"until", sUNTIL}, {"return", sRETURN}
};

static int ler(Lex *lex) {
    /*consome o caractere atual e avanca a leitura*/
    int c = lex->proximo;
    lex->proximo = fgetc(lex->fonte);
    if (c == '\n') lex->linha++;
    return c;
}

static Token token(CategoriaToken categoria, const char *texto, int linha) {
    /*monta um token para devolver ao parser*/
    Token t;
    t.categoria = categoria;
    t.linha = linha;
    snprintf(t.lexema, sizeof(t.lexema), "%s", texto);
    return t;
}

static Token erro(Lex *lex, int linha, const char *formato, ...) {
    /*guarda a mensagem e devolve um token de erro*/
    va_list args;
    va_start(args, formato);
    vsnprintf(lex->mensagem_erro, sizeof(lex->mensagem_erro), formato, args);
    va_end(args);
    return token(sERRO, lex->mensagem_erro, linha);
}

static void ignorar_espacos_e_comentarios(Lex *lex) {
    /*descarta espacos e comentarios de uma linha*/
    for (;;) {
        while (isspace((unsigned char)lex->proximo)) ler(lex);
        if (lex->proximo != '#') return;
        ler(lex);
        while (lex->proximo != EOF && lex->proximo != '\n') ler(lex);
    }
}

static CategoriaToken palavra_reservada(const char *texto) {
    /*verifica se o identificador e uma palavra reservada*/
    size_t i;
    for (i = 0; i < sizeof(palavras) / sizeof(palavras[0]); i++)
        if (strcmp(texto, palavras[i].texto) == 0) return palavras[i].categoria;
    return sIDENTIF;
}

int lex_init(Lex *lex, FILE *fonte) {
    /*prepara a leitura do arquivo fonte*/
    if (lex == NULL || fonte == NULL) return 0;
    lex->fonte = fonte;
    lex->linha = 1;
    lex->proximo = fgetc(fonte);
    lex->mensagem_erro[0] = '\0';
    return 1;
}

Token lex_next(Lex *lex) {
    char buf[TOKEN_LEXEMA_MAX];
    size_t n = 0;
    int c, linha;

    /*primeiro ignora o que nao gera token*/
    ignorar_espacos_e_comentarios(lex);
    linha = lex->linha;
    c = lex->proximo;
    if (c == EOF) return token(sEOF, "", linha);

    if (isalpha((unsigned char)c) || c == '_') {
        /*le identificadores e palavras reservadas*/
        do {
            if (n + 1 < sizeof(buf)) buf[n++] = (char)ler(lex);
            else { ler(lex); return erro(lex, linha, "identificador muito longo na linha %d", linha); }
        } while (isalnum((unsigned char)lex->proximo) || lex->proximo == '_');
        buf[n] = '\0';
        return token(palavra_reservada(buf), buf, linha);
    }

    if (isdigit((unsigned char)c)) {
        /*le constantes inteiras*/
        do {
            if (n + 1 < sizeof(buf)) buf[n++] = (char)ler(lex);
            else { ler(lex); return erro(lex, linha, "inteiro muito longo na linha %d", linha); }
        } while (isdigit((unsigned char)lex->proximo));
        if (isalpha((unsigned char)lex->proximo) || lex->proximo == '_')
            return erro(lex, linha, "numero invalido na linha %d", linha);
        buf[n] = '\0';
        return token(sCTEINT, buf, linha);
    }

    if (c == '"') {
        /*le uma string entre aspas duplas*/
        ler(lex);
        while (lex->proximo != EOF && lex->proximo != '"' && lex->proximo != '\n') {
            if (n + 1 >= sizeof(buf)) return erro(lex, linha, "string muito longa na linha %d", linha);
            buf[n++] = (char)ler(lex);
        }
        if (lex->proximo != '"') return erro(lex, linha, "string sem fechamento na linha %d", linha);
        ler(lex);
        buf[n] = '\0';
        return token(sSTRING, buf, linha);
    }

    if (c == '\'') {
        /*le um unico caractere entre aspas simples*/
        ler(lex);
        if (lex->proximo == EOF || lex->proximo == '\n' || lex->proximo == '\'')
            return erro(lex, linha, "constante caractere invalida na linha %d", linha);
        buf[0] = (char)ler(lex);
        buf[1] = '\0';
        if (lex->proximo != '\'') return erro(lex, linha, "constante caractere invalida na linha %d", linha);
        ler(lex);
        return token(sCTECHAR, buf, linha);
    }

    ler(lex);
    switch (c) {
        case '(': return token(sABREPAR, "(", linha);
        case ')': return token(sFECHAPAR, ")", linha);
        case '[': return token(sABRECOL, "[", linha);
        case ']': return token(sFECHACOL, "]", linha);
        case ',': return token(sVIRGULA, ",", linha);
        case ';': return token(sPONTOVIRGULA, ";", linha);
        case ':': return token(sDOISPONTOS, ":", linha);
        case '+': return token(sSOMA, "+", linha);
        case '-':
            if (lex->proximo == '>') { ler(lex); return token(sIMPLIC, "->", linha); }
            return token(sSUBRAT, "-", linha);
        case '*': return token(sMULT, "*", linha);
        case '&': return token(sAND, "&", linha);
        case '|': return token(sOR, "|", linha);
        case '~':
            if (lex->proximo == '=') { ler(lex); return token(sDIFERENTE, "~=", linha); }
            return token(sNEG, "~", linha);
        case '=': return token(sIGUAL, "=", linha);
        case '>':
            if (lex->proximo == '=') { ler(lex); return token(sMAIORIGUAL, ">=", linha); }
            return token(sMAIOR, ">", linha);
        case '<':
            if (lex->proximo == '<') { ler(lex); return token(sATRIB, "<<", linha); }
            if (lex->proximo == '=') { ler(lex); return token(sMENORIGUAL, "<=", linha); }
            return token(sMENOR, "<", linha);
        case '/':
            if (lex->proximo == '/') { ler(lex); return token(sDIV, "//", linha); }
            if (lex->proximo == '#') {
                /*descarta comentario de bloco*/
                ler(lex);
                while (lex->proximo != EOF) {
                    if (lex->proximo == '#') { ler(lex); if (lex->proximo == '/') { ler(lex); return lex_next(lex); } }
                    else ler(lex);
                }
                return erro(lex, linha, "comentario de bloco sem fechamento na linha %d", linha);
            }
            return erro(lex, linha, "caractere '/' invalido na linha %d", linha);
        default:
            snprintf(buf, sizeof(buf), "%c", c);
            return erro(lex, linha, "caractere invalido '%s' na linha %d", buf, linha);
    }
}

const char *lex_erro(const Lex *lex) { return lex->mensagem_erro; }

const char *lex_nome_categoria(CategoriaToken c) {
    /*converte a categoria para texto no arquivo de log*/
    static const char *nomes[] = {
        "sEOF", "sERRO", "sIDENTIF", "sCTEINT", "sSTRING", "sCTECHAR",
        "sGLOBVARS", "sLOCVARS", "sKIND", "sINT", "sLOGIC", "sCHR",
        "sPROC", "sFUNC", "sREF", "sSTART", "sEND", "sECHO", "sGET",
        "sCASE", "sOTHERWISE", "sCHOOSE", "sMATCH", "sOTHERS", "sFOR",
        "sFROM", "sTO", "sBY", "sDO", "sWHILE", "sREPEAT", "sUNTIL", "sRETURN",
        "sATRIB", "sSOMA", "sSUBRAT", "sMULT", "sDIV", "sIGUAL", "sDIFERENTE",
        "sMAIOR", "sMENOR", "sMAIORIGUAL", "sMENORIGUAL", "sAND", "sOR", "sNEG",
        "sIMPLIC", "sABREPAR", "sFECHAPAR", "sABRECOL", "sFECHACOL", "sVIRGULA",
        "sPONTOVIRGULA", "sDOISPONTOS"
    };
    if (c < sEOF || c > sDOISPONTOS) return "sDESCONHECIDO";
    return nomes[c];
}
