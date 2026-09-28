#ifndef LOG_H
#define LOG_H

/*
 * Modulo log - registro dos artefatos intermediarios da analise.
 * Gera, conforme as opcoes da CLI, os arquivos:
 *   <fonte>.tk   lista de tokens        -> <L>  <CAT>  "<LEX>"
 *   <fonte>.ts   tabela de simbolos     -> SCOPE=...  id="..."  cat=...  tipo=...  extra=...
 *   <fonte>.trc  rastreamento do parser -> entrada/saida dos nao-terminais
 */

/*abre os arquivos de log habilitados; devolve 1 em sucesso*/
int log_abrir(const char *arquivo_fonte, int tokens, int symtab, int trace);

/*registra um token no arquivo .tk*/
void log_token(int linha, const char *categoria, const char *lexema);

/*registra uma linha de rastreamento no arquivo .trc*/
void log_trace(const char *texto);

/*registra uma entrada da tabela de simbolos no arquivo .ts*/
void log_simbolo(const char *escopo, const char *lexema,
                 const char *categoria, const char *tipo, int extra);

/*informa se o arquivo .ts foi habilitado (evita trabalho desnecessario)*/
int log_symtab_ativo(void);

/*fecha todos os arquivos abertos; pode ser chamado mais de uma vez*/
void log_fechar(void);

#endif
