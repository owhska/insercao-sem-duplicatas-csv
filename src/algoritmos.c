#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "algoritmos.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
double agora_segundos(void) {
    LARGE_INTEGER freq, cont;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cont);
    return (double)cont.QuadPart / (double)freq.QuadPart;
}
#else
double agora_segundos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif

unsigned long long comp_ordenacao = 0;
unsigned long long comp_busca = 0;

int busca_sequencial_iterativa(char **v, size_t n, const char *chave) {
    size_t i;
    for (i = 0; i < n; i++) {
        comp_busca++;
        if (strcmp(v[i], chave) == 0) return 1;
    }
    return 0;
}

static int busca_sequencial_rec_aux(char **v, size_t n, const char *chave, size_t i) {
    if (i >= n) return 0;
    comp_busca++;
    if (strcmp(v[i], chave) == 0) return 1;
    return busca_sequencial_rec_aux(v, n, chave, i + 1);
}

int busca_sequencial_recursiva(char **v, size_t n, const char *chave) {
    return busca_sequencial_rec_aux(v, n, chave, 0);
}

static void intercalar(char **v, char **aux, size_t ini, size_t meio, size_t fim) {
    size_t i = ini, j = meio, k = ini;
    while (i < meio && j < fim) {
        comp_ordenacao++;
        if (strcmp(v[i], v[j]) <= 0) aux[k++] = v[i++];
        else aux[k++] = v[j++];
    }
    while (i < meio) aux[k++] = v[i++];
    while (j < fim) aux[k++] = v[j++];
    memcpy(v + ini, aux + ini, (fim - ini) * sizeof(char *));
}

static void mergesort_rec_aux(char **v, char **aux, size_t ini, size_t fim) {
    size_t meio;
    if (fim - ini < 2) return;
    meio = ini + (fim - ini) / 2;
    mergesort_rec_aux(v, aux, ini, meio);
    mergesort_rec_aux(v, aux, meio, fim);
    intercalar(v, aux, ini, meio, fim);
}

static char **alocar_auxiliar(size_t n) {
    char **aux = malloc((n ? n : 1) * sizeof(char *));
    if (!aux) {
        fprintf(stderr, "ERRO: memoria insuficiente para o MergeSort.\n");
        exit(EXIT_FAILURE);
    }
    return aux;
}

void mergesort_recursivo(char **v, size_t n) {
    char **aux;
    if (n < 2) return;
    aux = alocar_auxiliar(n);
    mergesort_rec_aux(v, aux, 0, n);
    free(aux);
}

void mergesort_iterativo(char **v, size_t n) {
    char **aux;
    size_t largura, ini, meio, fim;
    if (n < 2) return;
    aux = alocar_auxiliar(n);
    for (largura = 1; largura < n; largura *= 2) {
        for (ini = 0; ini + largura < n; ini += 2 * largura) {
            meio = ini + largura;
            fim = (ini + 2 * largura < n) ? ini + 2 * largura : n;
            intercalar(v, aux, ini, meio, fim);
        }
    }
    free(aux);
}

size_t busca_binaria_iterativa(char **v, size_t n, const char *chave) {
    size_t lo = 0, hi = n, meio;
    while (lo < hi) {
        meio = lo + (hi - lo) / 2;
        comp_busca++;
        if (strcmp(v[meio], chave) < 0) lo = meio + 1;
        else hi = meio;
    }
    return lo;
}

static size_t busca_binaria_rec_aux(char **v, const char *chave, size_t lo, size_t hi) {
    size_t meio;
    if (lo >= hi) return lo;
    meio = lo + (hi - lo) / 2;
    comp_busca++;
    if (strcmp(v[meio], chave) < 0) return busca_binaria_rec_aux(v, chave, meio + 1, hi);
    return busca_binaria_rec_aux(v, chave, lo, meio);
}

size_t busca_binaria_recursiva(char **v, size_t n, const char *chave) {
    return busca_binaria_rec_aux(v, chave, 0, n);
}
