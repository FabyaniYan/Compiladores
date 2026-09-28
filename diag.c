#include "diag.h"
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static const char *fonte_atual = "<fonte>";

void diag_init(const char *arquivo_fonte) {
    if (arquivo_fonte != NULL) fonte_atual = arquivo_fonte;
}

void diag_error(int linha, const char *formato, ...) {
    va_list args;

    fprintf(stderr, "%s:%d: erro: ", fonte_atual, linha);
    va_start(args, formato);
    vfprintf(stderr, formato, args);
    va_end(args);
    fprintf(stderr, "\n");

    /*erro interrompe o processamento: fecha os logs e encerra*/
    log_fechar();
    exit(EXIT_FAILURE);
}

void diag_info(const char *formato, ...) {
    char buffer[512];
    va_list args;

    va_start(args, formato);
    vsnprintf(buffer, sizeof(buffer), formato, args);
    va_end(args);
    log_trace(buffer);
}

void diag_ok(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vfprintf(stdout, formato, args);
    va_end(args);
    fprintf(stdout, "\n");
}
