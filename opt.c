#include "opt.h"

#include <stdio.h>
#include <string.h>

/*estado interno do modulo: so e acessivel por opts_get*/
static Opcoes opcoes = { NULL, 0, 0, 0 };

void opts_uso(const char *programa) {
    fprintf(stderr, "uso: %s <arquivo.slac> [--tokens] [--symtab] [--trace]\n", programa);
}

int opts_parse(int argc, char *argv[]) {
    int i;

    opcoes.arquivo_fonte = NULL;
    opcoes.gerar_tokens = 0;
    opcoes.gerar_symtab = 0;
    opcoes.gerar_trace = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) {
            opcoes.gerar_tokens = 1;
        } else if (strcmp(argv[i], "--symtab") == 0) {
            opcoes.gerar_symtab = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            opcoes.gerar_trace = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "opcao desconhecida: %s\n", argv[i]);
            return 0;
        } else if (opcoes.arquivo_fonte == NULL) {
            opcoes.arquivo_fonte = argv[i];
        } else {
            fprintf(stderr, "apenas um arquivo fonte pode ser informado\n");
            return 0;
        }
    }

    /*o fonte e o unico parametro obrigatorio*/
    if (opcoes.arquivo_fonte == NULL) return 0;
    return 1;
}

const Opcoes *opts_get(void) { return &opcoes; }
