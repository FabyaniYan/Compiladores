#ifndef OPT_H
#define OPT_H

/*
 * Modulo opt - interpretacao da linha de comando.
 * Uso: complac <arquivo.slac> [--tokens] [--symtab] [--trace]
 */

typedef struct {
    const char *arquivo_fonte; /*path do fonte informado na CLI*/
    int gerar_tokens;          /*--tokens: gera arquivo .tk*/
    int gerar_symtab;          /*--symtab: gera arquivo .ts*/
    int gerar_trace;           /*--trace : gera arquivo .trc*/
} Opcoes;

/*le argv e preenche as opcoes; devolve 1 em sucesso e 0 em uso invalido*/
int opts_parse(int argc, char *argv[]);

/*devolve as opcoes ja interpretadas (nunca NULL apos opts_parse)*/
const Opcoes *opts_get(void);

/*imprime a forma correta de chamada do binario*/
void opts_uso(const char *programa);

#endif
