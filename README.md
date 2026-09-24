# Análise de Algoritmos — Busca Sequencial Iterativa e Recursiva

## Descrição


Programa em C que insere registros de um CSV em outro, sem duplicatas,
usando **busca sequencial** como chave de unicidade. Compara duas
implementações: **iterativa** e **recursiva**.

- **Arquivo:** `bolsistas-2025.csv` (Portal de Dados Abertos)
- **Coluna-chave:** `Nome`
- **Fórmula:** `T(N, Q) = N·Q + N(N−1)/2`
- **Complexidade:** Θ(N²) no total

## Compilação

```bash
make
# ou
gcc -O2 -Wall -Wextra -std=c99 -o src/copiador src/copiador.c
