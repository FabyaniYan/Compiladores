#ifndef LEX_H
#define LEX_H

#include <stdio.h>

#define TOKEN_LEXEMA_MAX 256

typedef enum {
    sEOF, sERRO,
    sIDENTIF, sCTEINT, sSTRING, sCTECHAR,
    sGLOBVARS, sLOCVARS, sKIND, sINT, sLOGIC, sCHR,
    sPROC, sFUNC, sREF, sSTART, sEND, sECHO, sGET,
    sCASE, sOTHERWISE, sCHOOSE, sMATCH, sOTHERS,
    sFOR, sFROM, sTO, sBY, sDO, sWHILE, sREPEAT, sUNTIL, sRETURN,
    sATRIB, sSOMA, sSUBRAT, sMULT, sDIV,
    sIGUAL, sDIFERENTE, sMAIOR, sMENOR, sMAIORIGUAL, sMENORIGUAL,
    sAND, sOR, sNEG, sIMPLIC,
    sABREPAR, sFECHAPAR, sABRECOL, sFECHACOL, sVIRGULA, sPONTOVIRGULA, sDOISPONTOS
} CategoriaToken;

typedef struct {
    CategoriaToken categoria;
    char lexema[TOKEN_LEXEMA_MAX];
    int linha;
} Token;

typedef struct {
    FILE *fonte;
    int linha;
    int proximo;
    char mensagem_erro[256];
} Lex;

int lex_init(Lex *lex, FILE *fonte);
Token lex_next(Lex *lex);
const char *lex_nome_categoria(CategoriaToken categoria);
const char *lex_erro(const Lex *lex);

#endif
