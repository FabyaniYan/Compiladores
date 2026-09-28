#ifndef DIAG_H
#define DIAG_H

/*
 * Modulo diag - centraliza toda mensagem emitida pelo compilador.
 * lex e parser nunca escrevem na saida diretamente: chamam estas funcoes.
 */

/*guarda o nome do fonte, usado no cabecalho das mensagens*/
void diag_init(const char *arquivo_fonte);

/*
 * Relata um erro e encerra o processo de compilacao.
 * Formato: <fonte>:<linha>: erro: <mensagem>
 * Fecha os logs antes de sair, garantindo o encerramento ordenado.
 */
void diag_error(int linha, const char *formato, ...);

/*registra informacao de rastreamento (so aparece se --trace estiver ativo)*/
void diag_info(const char *formato, ...);

/*mensagem informativa impressa no stdout, usada ao final da analise*/
void diag_ok(const char *formato, ...);

#endif
