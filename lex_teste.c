#include "lex.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *fonte;
    Lex lex;
    Token t;

    /*recebe o arquivo slac pelo terminal*/
    if (argc != 2) {
        fprintf(stderr, "uso: %s <arquivo.slac>\n", argv[0]);
        return EXIT_FAILURE;
    }
    fonte = fopen(argv[1], "r");
    if (fonte == NULL) { perror("erro ao abrir arquivo"); return EXIT_FAILURE; }
    lex_init(&lex, fonte);
    /*mostra todos os tokens encontrados*/
    do {
        t = lex_next(&lex);
        printf("%d  %s  \"%s\"\n", t.linha, lex_nome_categoria(t.categoria), t.lexema);
    } while (t.categoria != sEOF && t.categoria != sERRO);
    fclose(fonte);
    return t.categoria == sERRO ? EXIT_FAILURE : EXIT_SUCCESS;
}
