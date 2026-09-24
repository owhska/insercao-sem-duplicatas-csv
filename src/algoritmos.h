#ifndef ALGORITMOS_H
#define ALGORITMOS_H

#include <stddef.h>

extern unsigned long long comp_ordenacao;
extern unsigned long long comp_busca;

int busca_sequencial_iterativa(char **v, size_t n, const char *chave);
int busca_sequencial_recursiva(char **v, size_t n, const char *chave);

void mergesort_iterativo(char **v, size_t n);
void mergesort_recursivo(char **v, size_t n);

size_t busca_binaria_iterativa(char **v, size_t n, const char *chave);
size_t busca_binaria_recursiva(char **v, size_t n, const char *chave);

double agora_segundos(void);

#endif
