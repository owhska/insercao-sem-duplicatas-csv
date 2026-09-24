#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algoritmos.h"

#define MAX_LINHA 4096
#define MAX_NOMES 100000

static char *nomes_base[MAX_NOMES];
static size_t qtd_base = 0;
static unsigned long long semente = 20260923ULL;

static unsigned long long aleatorio(void) {
    semente = semente * 6364136223846793005ULL + 1442695040888963407ULL;
    return semente >> 33;
}

static void carregar_base(const char *caminho) {
    FILE *fp = fopen(caminho, "r");
    char linha[MAX_LINHA];
    int primeira = 1;
    size_t i;
    if (!fp) { perror("ERRO ao abrir base"); exit(EXIT_FAILURE); }
    while (fgets(linha, sizeof(linha), fp) && qtd_base < MAX_NOMES) {
        char *p = strchr(linha, ',');
        int repetido = 0;
        if (primeira) { primeira = 0; continue; }
        if (p) *p = '\0';
        linha[strcspn(linha, "\r\n")] = '\0';
        if (!linha[0]) continue;
        for (i = 0; i < qtd_base; i++)
            if (strcmp(nomes_base[i], linha) == 0) { repetido = 1; break; }
        if (repetido) continue;
        nomes_base[qtd_base] = malloc(strlen(linha) + 1);
        strcpy(nomes_base[qtd_base++], linha);
    }
    fclose(fp);
}

static char **gerar_chaves(size_t n) {
    char **v = malloc(n * sizeof(char *));
    size_t i, j;
    for (i = 0; i < n; i++) {
        char buf[MAX_LINHA];
        snprintf(buf, sizeof(buf), "%s #%zu", nomes_base[i % qtd_base], i / qtd_base);
        v[i] = malloc(strlen(buf) + 1);
        strcpy(v[i], buf);
    }
    for (i = n - 1; i > 0; i--) {
        char *t;
        j = (size_t)(aleatorio() % (i + 1));
        t = v[i]; v[i] = v[j]; v[j] = t;
    }
    return v;
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void rodar_sequencial(char **chaves, size_t n, int recursiva, double *t, unsigned long long *cb) {
    char **cad = malloc(n * sizeof(char *));
    size_t qtd = 0, i;
    double t0 = agora_segundos();
    comp_busca = 0;
    for (i = 0; i < n; i++) {
        int existe = recursiva ? busca_sequencial_recursiva(cad, qtd, chaves[i])
                               : busca_sequencial_iterativa(cad, qtd, chaves[i]);
        if (!existe) cad[qtd++] = chaves[i];
    }
    *t = agora_segundos() - t0;
    *cb = comp_busca;
    free(cad);
}

static void rodar_binaria(char **chaves, size_t n, int recursiva, double *t_ord, double *t_bus,
                          unsigned long long *co, unsigned long long *cb) {
    char **ord = malloc(n * sizeof(char *));
    size_t i, inseridos = 0, pos;
    double t0, t1, t2;
    memcpy(ord, chaves, n * sizeof(char *));
    comp_ordenacao = 0;
    comp_busca = 0;
    t0 = agora_segundos();
    if (recursiva) mergesort_recursivo(ord, n); else mergesort_iterativo(ord, n);
    t1 = agora_segundos();
    for (i = 0; i < n; i++) {
        pos = recursiva ? busca_binaria_recursiva(ord, n, chaves[i])
                        : busca_binaria_iterativa(ord, n, chaves[i]);
        if (ord[pos] == chaves[i]) inseridos++;
    }
    t2 = agora_segundos();
    if (inseridos != n) fprintf(stderr, "AVISO: inseridos=%zu n=%zu\n", inseridos, n);
    *t_ord = t1 - t0;
    *t_bus = t2 - t1;
    *co = comp_ordenacao;
    *cb = comp_busca;
    free(ord);
}

int main(int argc, char *argv[]) {
    size_t tam_seq[] = { 1000, 2000, 4000, 8000 };
    size_t tam_bin[] = { 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000, 512000 };
    int reps = 15, r, modo;
    size_t k, i;
    double tempos[64], tempos_b[64];
    if (argc < 2) { fprintf(stderr, "Uso: %s <base.csv> [repeticoes]\n", argv[0]); return EXIT_FAILURE; }
    if (argc >= 3) reps = atoi(argv[2]);
    if (reps < 1 || reps > 64) reps = 15;
    carregar_base(argv[1]);
    printf("solucao,modo,n,comp_ordenacao,comp_busca,tempo_ordenacao_s,tempo_busca_s,tempo_total_s\n");
    for (k = 0; k < sizeof(tam_seq) / sizeof(tam_seq[0]); k++) {
        size_t n = tam_seq[k];
        char **chaves = gerar_chaves(n);
        for (modo = 0; modo < 2; modo++) {
            unsigned long long cb = 0;
            for (r = 0; r < reps; r++) rodar_sequencial(chaves, n, modo, &tempos[r], &cb);
            qsort(tempos, (size_t)reps, sizeof(double), cmp_double);
            printf("1,%s,%zu,0,%llu,0,%.9f,%.9f\n", modo ? "seq-rec" : "seq-iter", n, cb,
                   tempos[reps / 2], tempos[reps / 2]);
            fflush(stdout);
        }
        for (i = 0; i < n; i++) free(chaves[i]);
        free(chaves);
    }
    for (k = 0; k < sizeof(tam_bin) / sizeof(tam_bin[0]); k++) {
        size_t n = tam_bin[k];
        char **chaves = gerar_chaves(n);
        for (modo = 0; modo < 2; modo++) {
            unsigned long long co = 0, cb = 0;
            double tt[64];
            for (r = 0; r < reps; r++) {
                rodar_binaria(chaves, n, modo, &tempos[r], &tempos_b[r], &co, &cb);
                tt[r] = tempos[r] + tempos_b[r];
            }
            qsort(tempos, (size_t)reps, sizeof(double), cmp_double);
            qsort(tempos_b, (size_t)reps, sizeof(double), cmp_double);
            qsort(tt, (size_t)reps, sizeof(double), cmp_double);
            printf("2,%s,%zu,%llu,%llu,%.9f,%.9f,%.9f\n", modo ? "bin-rec" : "bin-iter", n, co, cb,
                   tempos[reps / 2], tempos_b[reps / 2], tt[reps / 2]);
            fflush(stdout);
        }
        for (i = 0; i < n; i++) free(chaves[i]);
        free(chaves);
    }
    return EXIT_SUCCESS;
}
