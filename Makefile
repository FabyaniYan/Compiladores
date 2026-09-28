# Makefile do COMPLAC - compilador para a linguagem SLAC2
# Compilacao completa do projeto com um unico comando: make

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99
ALVO    = complac
FONTES  = main.c opt.c log.c diag.c lex.c symtab.c parser.c
OBJETOS = $(FONTES:.c=.o)

all: $(ALVO)

$(ALVO): $(OBJETOS)
	$(CC) $(CFLAGS) -o $@ $(OBJETOS)

# dependencias de cabecalho
main.o:   main.c diag.h lex.h log.h opt.h parser.h symtab.h
opt.o:    opt.c opt.h
log.o:    log.c log.h
diag.o:   diag.c diag.h log.h
lex.o:    lex.c lex.h
symtab.o: symtab.c symtab.h lex.h diag.h log.h
parser.o: parser.c parser.h lex.h diag.h log.h symtab.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# executa os programas de teste incluidos no repositorio
teste: $(ALVO)
	./$(ALVO) testes/exemplo1.slac --tokens --symtab --trace
	./$(ALVO) testes/exemplo2.slac --tokens --symtab --trace
	./$(ALVO) testes/exemplo3.slac --tokens --symtab --trace
	-./$(ALVO) testes/erro_sintatico.slac
	-./$(ALVO) testes/erro_lexico.slac
	-./$(ALVO) testes/erro_duplicado.slac

clean:
	rm -f $(OBJETOS) $(ALVO) testes/*.tk testes/*.ts testes/*.trc

.PHONY: all teste clean
