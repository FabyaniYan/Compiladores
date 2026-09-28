#include "log.h"

#include <stdio.h>
#include <string.h>

#define PATH_MAX_LOG 512

static FILE *arq_tokens = NULL;
static FILE *arq_symtab = NULL;
static FILE *arq_trace = NULL;

static void troca_extensao(char *destino, size_t tam, const char *fonte, const char *ext) {
    /*monta <fonte-sem-extensao>.<ext>; se nao houver extensao, apenas concatena*/
    const char *ponto = strrchr(fonte, '.');
    const char *barra = strrchr(fonte, '/');
    size_t base;

    if (ponto == NULL || (barra != NULL && ponto < barra)) base = strlen(fonte);
    else base = (size_t)(ponto - fonte);

    if (base >= tam) base = tam - 1;
    memcpy(destino, fonte, base);
    destino[base] = '\0';
    snprintf(destino + base, tam - base, ".%s", ext);
}

static FILE *abrir(const char *fonte, const char *ext) {
    char caminho[PATH_MAX_LOG];
    troca_extensao(caminho, sizeof(caminho), fonte, ext);
    return fopen(caminho, "w");
}

int log_abrir(const char *arquivo_fonte, int tokens, int symtab, int trace) {
    if (arquivo_fonte == NULL) return 0;

    if (tokens) {
        arq_tokens = abrir(arquivo_fonte, "tk");
        if (arq_tokens == NULL) return 0;
    }
    if (symtab) {
        arq_symtab = abrir(arquivo_fonte, "ts");
        if (arq_symtab == NULL) return 0;
    }
    if (trace) {
        arq_trace = abrir(arquivo_fonte, "trc");
        if (arq_trace == NULL) return 0;
    }
    return 1;
}

void log_token(int linha, const char *categoria, const char *lexema) {
    if (arq_tokens == NULL) return;
    fprintf(arq_tokens, "%d  %s  \"%s\"\n", linha, categoria, lexema);
}

void log_trace(const char *texto) {
    if (arq_trace == NULL) return;
    fprintf(arq_trace, "%s\n", texto);
}

void log_simbolo(const char *escopo, const char *lexema,
                 const char *categoria, const char *tipo, int extra) {
    if (arq_symtab == NULL) return;
    fprintf(arq_symtab, "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d\n",
            escopo, lexema, categoria, tipo, extra);
}

int log_symtab_ativo(void) { return arq_symtab != NULL; }

void log_fechar(void) {
    /*fecha o que estiver aberto e zera os ponteiros para permitir nova chamada*/
    if (arq_tokens != NULL) { fclose(arq_tokens); arq_tokens = NULL; }
    if (arq_symtab != NULL) { fclose(arq_symtab); arq_symtab = NULL; }
    if (arq_trace != NULL) { fclose(arq_trace); arq_trace = NULL; }
}
