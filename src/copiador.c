#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================
 * Medição de tempo portátil (Windows + Linux/macOS)
 * ------------------------------------------------------------
 * - Windows: usa QueryPerformanceCounter (alta resolução).
 * - Linux/macOS: usa clock_gettime(CLOCK_MONOTONIC).
 * ============================================================ */
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    static double agora_segundos(void) {
        LARGE_INTEGER freq, cont;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&cont);
        return (double)cont.QuadPart / (double)freq.QuadPart;
    }
#else
    static double agora_segundos(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    }
#endif

#define MAX_LINHA 1024
#define MAX_NOME  512
#define CABECALHO "Nome,Modalidade,Nivel,Agencia"

typedef struct {
    char **nomes;
    size_t qtd;
    size_t capacidade;
} Cadastro;

static unsigned long long comparacoes = 0;
static int usar_recursiva = 0; /* 0 = iterativa, 1 = recursiva */

/* ---------- Cadastro ---------- */

void cadastro_iniciar(Cadastro *c) {
    c->nomes = NULL;
    c->qtd = 0;
    c->capacidade = 0;
}

int cadastro_inserir(Cadastro *c, const char *nome) {
    if (c->qtd == c->capacidade) {
        size_t nova = (c->capacidade == 0) ? 16 : c->capacidade * 2;
        char **bloco = realloc(c->nomes, nova * sizeof(char *));
        if (!bloco) return -1;
        c->nomes = bloco;
        c->capacidade = nova;
    }
    c->nomes[c->qtd] = malloc(strlen(nome) + 1);
    if (!c->nomes[c->qtd]) return -1;
    strcpy(c->nomes[c->qtd], nome);
    c->qtd++;
    return 0;
}

void cadastro_liberar(Cadastro *c) {
    size_t i;
    for (i = 0; i < c->qtd; i++) free(c->nomes[i]);
    free(c->nomes);
    cadastro_iniciar(c);
}

/* ---------- Busca sequencial iterativa ---------- */

int busca_sequencial_iterativa(const Cadastro *c, const char *nome) {
    size_t i;
    for (i = 0; i < c->qtd; i++) {
        comparacoes++;
        if (strcmp(c->nomes[i], nome) == 0) return 1;
    }
    return 0;
}

/* ---------- Busca sequencial recursiva ---------- */

int busca_sequencial_recursiva_aux(const Cadastro *c, const char *nome, size_t i) {
    if (i >= c->qtd) return 0;
    comparacoes++;
    if (strcmp(c->nomes[i], nome) == 0) return 1;
    return busca_sequencial_recursiva_aux(c, nome, i + 1);
}

int busca_sequencial_recursiva(const Cadastro *c, const char *nome) {
    return busca_sequencial_recursiva_aux(c, nome, 0);
}

/* ---------- Despacho ---------- */

int busca_sequencial(const Cadastro *c, const char *nome) {
    if (usar_recursiva)
        return busca_sequencial_recursiva(c, nome);
    return busca_sequencial_iterativa(c, nome);
}

/* ---------- Utilidades de linha ---------- */

void remover_fim_de_linha(char *linha) {
    size_t n = strlen(linha);
    while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r')) {
        linha[n - 1] = '\0';
        n--;
    }
}

void extrair_nome(const char *linha, char *nome) {
    size_t i = 0;
    while (linha[i] != '\0' && linha[i] != ',' && i < MAX_NOME - 1) {
        nome[i] = linha[i];
        i++;
    }
    nome[i] = '\0';
}

int linha_e_cabecalho(const char *linha) {
    return strncmp(linha, "Nome,", 5) == 0;
}

int linha_vazia(const char *linha) {
    size_t i;
    for (i = 0; linha[i] != '\0'; i++)
        if (linha[i] != ' ' && linha[i] != '\t') return 0;
    return 1;
}

/* ---------- Arquivos ---------- */

size_t carregar_destino(const char *caminho, Cadastro *c) {
    FILE *fp = fopen(caminho, "r");
    char linha[MAX_LINHA], nome[MAX_NOME];
    if (!fp) return 0;
    while (fgets(linha, sizeof(linha), fp)) {
        remover_fim_de_linha(linha);
        if (linha_vazia(linha) || linha_e_cabecalho(linha)) continue;
        extrair_nome(linha, nome);
        if (cadastro_inserir(c, nome) != 0) {
            fprintf(stderr, "ERRO: memoria insuficiente ao carregar o destino.\n");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
    return c->qtd;
}

void garantir_cabecalho(const char *caminho) {
    FILE *fp = fopen(caminho, "a+");
    if (!fp) { perror("ERRO ao abrir destino"); exit(EXIT_FAILURE); }
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) fprintf(fp, "%s\n", CABECALHO);
    fclose(fp);
}

int gravar_estatisticas(const char *caminho, size_t inseridos,
                        unsigned long long total, double tempo_s) {
    FILE *fp = fopen(caminho, "a+");
    if (!fp) { perror("ERRO ao abrir estatisticas"); return -1; }
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0)
        fprintf(fp, "nomes_inseridos,comparacoes,tempo_s\n");
    fprintf(fp, "%zu,%llu,%.6f\n", inseridos, total, tempo_s);
    fclose(fp);
    return 0;
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    Cadastro cadastro;
    FILE *origem, *destino;
    char linha[MAX_LINHA], nome[MAX_NOME];
    size_t q_inicial, inseridos = 0, redundantes = 0, lidos = 0;
    double t0, t1, tempo_s;

    if (argc < 4 || argc > 5) {
        fprintf(stderr,
            "Uso: %s <origem.csv> <destino.csv> <estatisticas.csv> [iter|rec]\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 5) {
        if (strcmp(argv[4], "rec") == 0) usar_recursiva = 1;
        else if (strcmp(argv[4], "iter") != 0) {
            fprintf(stderr, "Modo invalido: use 'iter' ou 'rec'.\n");
            return EXIT_FAILURE;
        }
    }

    cadastro_iniciar(&cadastro);
    garantir_cabecalho(argv[2]);
    q_inicial = carregar_destino(argv[2], &cadastro);

    origem = fopen(argv[1], "r");
    if (!origem) {
        perror("ERRO ao abrir origem");
        cadastro_liberar(&cadastro);
        return EXIT_FAILURE;
    }

    destino = fopen(argv[2], "a");
    if (!destino) {
        perror("ERRO ao abrir destino");
        fclose(origem);
        cadastro_liberar(&cadastro);
        return EXIT_FAILURE;
    }

    t0 = agora_segundos();

    while (fgets(linha, sizeof(linha), origem)) {
        remover_fim_de_linha(linha);
        if (linha_vazia(linha) || linha_e_cabecalho(linha)) continue;
        lidos++;
        extrair_nome(linha, nome);

        if (busca_sequencial(&cadastro, nome)) {
            printf("ERRO: nome redundante encontrado: %s\n", nome);
            redundantes++;
            continue;
        }

        fprintf(destino, "%s\n", linha);

        if (cadastro_inserir(&cadastro, nome) != 0) {
            fprintf(stderr, "ERRO: memoria insuficiente ao inserir o nome.\n");
            fclose(origem);
            fclose(destino);
            cadastro_liberar(&cadastro);
            return EXIT_FAILURE;
        }
        inseridos++;
    }

    t1 = agora_segundos();
    tempo_s = t1 - t0;

    fclose(origem);
    fclose(destino);

    printf("\n");
    printf("Modo de busca               : %s\n", usar_recursiva ? "recursiva" : "iterativa");
    printf("Registros lidos na origem   : %zu\n", lidos);
    printf("Cadastros previos no destino: %zu\n", q_inicial);
    printf("Nomes inseridos             : %zu\n", inseridos);
    printf("Nomes redundantes           : %zu\n", redundantes);
    printf("Comparacoes entre nomes     : %llu\n", comparacoes);
    printf("Tempo de execucao           : %.6f s\n", tempo_s);

    if (gravar_estatisticas(argv[3], inseridos, comparacoes, tempo_s) != 0) {
        cadastro_liberar(&cadastro);
        return EXIT_FAILURE;
    }

    cadastro_liberar(&cadastro);
    return EXIT_SUCCESS;
}
