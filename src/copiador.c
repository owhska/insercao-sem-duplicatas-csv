#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algoritmos.h"

#define MAX_LINHA 4096
#define COLUNA_CHAVE 0

typedef struct {
    char **itens;
    size_t qtd;
    size_t capacidade;
} Vetor;

typedef enum { SEQ_ITER, SEQ_REC, BIN_ITER, BIN_REC } Modo;

static const char *nomes_modo[] = { "seq-iter", "seq-rec", "bin-iter", "bin-rec" };

static void vetor_iniciar(Vetor *v) {
    v->itens = NULL;
    v->qtd = 0;
    v->capacidade = 0;
}

static void vetor_inserir(Vetor *v, char *s) {
    if (v->qtd == v->capacidade) {
        size_t nova = v->capacidade ? v->capacidade * 2 : 16;
        char **bloco = realloc(v->itens, nova * sizeof(char *));
        if (!bloco) {
            fprintf(stderr, "ERRO: memoria insuficiente.\n");
            exit(EXIT_FAILURE);
        }
        v->itens = bloco;
        v->capacidade = nova;
    }
    v->itens[v->qtd++] = s;
}

static void vetor_liberar(Vetor *v, int liberar_itens) {
    size_t i;
    if (liberar_itens)
        for (i = 0; i < v->qtd; i++) free(v->itens[i]);
    free(v->itens);
    vetor_iniciar(v);
}

static char *duplicar(const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (!d) {
        fprintf(stderr, "ERRO: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(d, s);
    return d;
}

static void remover_fim_de_linha(char *linha) {
    size_t n = strlen(linha);
    while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r')) linha[--n] = '\0';
}

static int linha_vazia(const char *linha) {
    for (; *linha; linha++)
        if (*linha != ' ' && *linha != '\t') return 0;
    return 1;
}

static char *extrair_chave(const char *linha) {
    int coluna = 0;
    const char *ini = linha, *p;
    char *chave;
    size_t tam;
    while (coluna < COLUNA_CHAVE && *ini) {
        if (*ini == ',') coluna++;
        ini++;
    }
    for (p = ini; *p && *p != ','; p++);
    tam = (size_t)(p - ini);
    chave = malloc(tam + 1);
    if (!chave) {
        fprintf(stderr, "ERRO: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(chave, ini, tam);
    chave[tam] = '\0';
    return chave;
}

static size_t carregar_chaves_destino(const char *caminho, Vetor *chaves) {
    FILE *fp = fopen(caminho, "r");
    char linha[MAX_LINHA];
    int primeira = 1;
    if (!fp) return 0;
    while (fgets(linha, sizeof(linha), fp)) {
        remover_fim_de_linha(linha);
        if (primeira) { primeira = 0; continue; }
        if (linha_vazia(linha)) continue;
        vetor_inserir(chaves, extrair_chave(linha));
    }
    fclose(fp);
    return chaves->qtd;
}

static int destino_vazio(const char *caminho) {
    FILE *fp = fopen(caminho, "r");
    long tam;
    if (!fp) return 1;
    fseek(fp, 0, SEEK_END);
    tam = ftell(fp);
    fclose(fp);
    return tam <= 0;
}

static void garantir_quebra_final(const char *caminho) {
    FILE *fp = fopen(caminho, "rb");
    int ultimo;
    if (!fp) return;
    if (fseek(fp, -1, SEEK_END) != 0) { fclose(fp); return; }
    ultimo = fgetc(fp);
    fclose(fp);
    if (ultimo != '\n') {
        fp = fopen(caminho, "a");
        if (fp) { fputc('\n', fp); fclose(fp); }
    }
}

static void gravar_estatisticas(const char *caminho, Modo modo, size_t lidos, size_t previos,
                                size_t inseridos, size_t redundantes,
                                double t_ord, double t_busca, double t_total) {
    FILE *fp = fopen(caminho, "a+");
    if (!fp) { perror("ERRO ao abrir estatisticas"); return; }
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0)
        fprintf(fp, "modo,lidos,previos,inseridos,redundantes,comp_ordenacao,comp_busca,comp_total,tempo_ordenacao_s,tempo_verificacao_s,tempo_total_s\n");
    fprintf(fp, "%s,%zu,%zu,%zu,%zu,%llu,%llu,%llu,%.9f,%.9f,%.9f\n",
            nomes_modo[modo], lidos, previos, inseridos, redundantes,
            comp_ordenacao, comp_busca, comp_ordenacao + comp_busca, t_ord, t_busca, t_total);
    fclose(fp);
}

static void uso(const char *prog) {
    fprintf(stderr, "Uso: %s <novos.csv> <destino.csv> [seq-iter|seq-rec|bin-iter|bin-rec] [estatisticas.csv]\n", prog);
}

int main(int argc, char *argv[]) {
    Modo modo = SEQ_ITER;
    Vetor destino_chaves, linhas, chaves_novas, ordenadas;
    FILE *origem, *destino;
    char linha[MAX_LINHA];
    char *cabecalho = NULL;
    int primeira = 1;
    size_t previos, lidos = 0, inseridos = 0, redundantes = 0, i;
    double t0, t1, t2;
    int m;

    if (argc < 3 || argc > 5) { uso(argv[0]); return EXIT_FAILURE; }
    if (argc >= 4) {
        for (m = 0; m < 4; m++)
            if (strcmp(argv[3], nomes_modo[m]) == 0) break;
        if (m == 4) { uso(argv[0]); return EXIT_FAILURE; }
        modo = (Modo)m;
    }

    vetor_iniciar(&destino_chaves);
    vetor_iniciar(&linhas);
    vetor_iniciar(&chaves_novas);
    vetor_iniciar(&ordenadas);

    origem = fopen(argv[1], "r");
    if (!origem) { perror("ERRO ao abrir arquivo de novos registros"); return EXIT_FAILURE; }
    while (fgets(linha, sizeof(linha), origem)) {
        remover_fim_de_linha(linha);
        if (primeira) { primeira = 0; cabecalho = duplicar(linha); continue; }
        if (linha_vazia(linha)) continue;
        vetor_inserir(&linhas, duplicar(linha));
        vetor_inserir(&chaves_novas, extrair_chave(linha));
    }
    fclose(origem);
    lidos = linhas.qtd;

    if (destino_vazio(argv[2])) {
        destino = fopen(argv[2], "w");
        if (!destino) { perror("ERRO ao criar destino"); return EXIT_FAILURE; }
        fprintf(destino, "%s\n", cabecalho ? cabecalho : "");
        fclose(destino);
    } else {
        garantir_quebra_final(argv[2]);
    }
    previos = carregar_chaves_destino(argv[2], &destino_chaves);

    destino = fopen(argv[2], "a");
    if (!destino) { perror("ERRO ao abrir destino"); return EXIT_FAILURE; }

    t0 = agora_segundos();
    t1 = t0;

    if (modo == SEQ_ITER || modo == SEQ_REC) {
        for (i = 0; i < lidos; i++) {
            char *chave = chaves_novas.itens[i];
            int existe = (modo == SEQ_ITER)
                ? busca_sequencial_iterativa(destino_chaves.itens, destino_chaves.qtd, chave)
                : busca_sequencial_recursiva(destino_chaves.itens, destino_chaves.qtd, chave);
            if (existe) {
                printf("ERRO: valor duplicado na chave, registro ignorado: %s\n", chave);
                redundantes++;
                continue;
            }
            fprintf(destino, "%s\n", linhas.itens[i]);
            vetor_inserir(&destino_chaves, duplicar(chave));
            inseridos++;
        }
    } else {
        for (i = 0; i < lidos; i++) vetor_inserir(&ordenadas, chaves_novas.itens[i]);
        if (modo == BIN_ITER) {
            mergesort_iterativo(destino_chaves.itens, destino_chaves.qtd);
            mergesort_iterativo(ordenadas.itens, ordenadas.qtd);
        } else {
            mergesort_recursivo(destino_chaves.itens, destino_chaves.qtd);
            mergesort_recursivo(ordenadas.itens, ordenadas.qtd);
        }
        t1 = agora_segundos();
        for (i = 0; i < lidos; i++) {
            char *chave = chaves_novas.itens[i];
            size_t q = destino_chaves.qtd, pos;
            int existe = 0;
            if (q > 0) {
                pos = (modo == BIN_ITER)
                    ? busca_binaria_iterativa(destino_chaves.itens, q, chave)
                    : busca_binaria_recursiva(destino_chaves.itens, q, chave);
                if (pos < q) {
                    comp_busca++;
                    existe = (strcmp(destino_chaves.itens[pos], chave) == 0);
                }
            }
            if (!existe) {
                pos = (modo == BIN_ITER)
                    ? busca_binaria_iterativa(ordenadas.itens, ordenadas.qtd, chave)
                    : busca_binaria_recursiva(ordenadas.itens, ordenadas.qtd, chave);
                existe = (ordenadas.itens[pos] != chave);
            }
            if (existe) {
                printf("ERRO: valor duplicado na chave, registro ignorado: %s\n", chave);
                redundantes++;
                continue;
            }
            fprintf(destino, "%s\n", linhas.itens[i]);
            inseridos++;
        }
    }

    t2 = agora_segundos();
    fclose(destino);

    printf("\n");
    printf("Modo                        : %s\n", nomes_modo[modo]);
    printf("Registros lidos             : %zu\n", lidos);
    printf("Registros previos no destino: %zu\n", previos);
    printf("Registros inseridos         : %zu\n", inseridos);
    printf("Registros duplicados        : %zu\n", redundantes);
    printf("Comparacoes na ordenacao    : %llu\n", comp_ordenacao);
    printf("Comparacoes na busca        : %llu\n", comp_busca);
    printf("Comparacoes totais          : %llu\n", comp_ordenacao + comp_busca);
    printf("Tempo total                 : %.6f s\n", t2 - t0);

    if (argc == 5)
        gravar_estatisticas(argv[4], modo, lidos, previos, inseridos, redundantes,
                            t1 - t0, t2 - t1, t2 - t0);

    free(cabecalho);
    vetor_liberar(&destino_chaves, 1);
    vetor_liberar(&linhas, 1);
    vetor_liberar(&chaves_novas, 1);
    vetor_liberar(&ordenadas, 0);
    return EXIT_SUCCESS;
}
