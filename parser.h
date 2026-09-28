#ifndef PARSER_H
#define PARSER_H

#include "lex.h"

/*
 * Modulo parser - Analisador Sintatico Descendente Recursivo (ASDR).
 *
 * Segue diretamente a EBNF do Apendice A da especificacao da SLAC2:
 * cada nao-terminal da gramatica possui uma funcao propria neste modulo.
 * Consome os tokens do lex sob demanda (um token de lookahead) e e o
 * responsavel por abrir e fechar os escopos da tabela de simbolos.
 */

/*
 * Analisa o programa completo a partir do analisador lexico informado.
 * Qualquer erro interrompe a execucao atraves de diag_error.
 */
void parse_program(Lex *lex);

#endif
