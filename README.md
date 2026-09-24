# Inserção sem duplicatas em CSV — Análise de Algoritmos

Trabalho da disciplina de Análise de Algoritmos (Relatório 4).

Programa em C que insere, ao final de um CSV de destino, os registros de um CSV de novos registros, sem introduzir valores repetidos na coluna-chave `Nome`. A verificação de duplicidade é implementada de duas formas:

- **Solução 1:** busca sequencial, nas versões iterativa e recursiva;
- **Solução 2:** MergeSort (iterativo e recursivo) sobre as chaves em memória + busca binária (iterativa e recursiva).

Base de dados: `dados/bolsistas-2025.csv` (Portal de Dados Abertos, dados.gov.br), 719 registros, colunas `Nome,Modalidade,Nivel,Agência`, codificação Latin-1.

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `src/algoritmos.c`, `src/algoritmos.h` | buscas sequenciais, MergeSorts, buscas binárias e contadores de comparações |
| `src/copiador.c` | programa principal |
| `src/bench.c` | benchmark de tempo em memória (sem E/S), com entradas maiores |
| `dados/bolsistas-2025.csv` | arquivo original |
| `preparar_entradas.py` | gera os arquivos de teste em `experimentos/entradas/` |
| `rodar_experimentos.sh` | compila e executa toda a bateria de experimentos |
| `analisar.py` | confronta medições e teoria e gera os gráficos |
| `experimentos/` | entradas, destinos, saídas e estatísticas usados no relatório |
| `graficos/` | gráficos do relatório |
| `relatorio/` | fonte LaTeX e PDF do relatório |

## Compilação

```bash
make
```

ou, manualmente:

```bash
gcc -O2 -fno-optimize-sibling-calls -Wall -Wextra -std=c99 -o src/copiador src/copiador.c src/algoritmos.c
gcc -O2 -fno-optimize-sibling-calls -Wall -Wextra -std=c99 -o src/bench src/bench.c src/algoritmos.c
```

A opção `-fno-optimize-sibling-calls` impede que o GCC transforme as chamadas recursivas de cauda em laços; sem ela, as versões recursivas das buscas seriam compiladas como iterativas e a comparação entre as implementações perderia o sentido.

Funciona em Linux e no Windows (MSYS2/MinGW ou Git Bash com GCC).

## Execução

```bash
./src/copiador <novos.csv> <destino.csv> [modo] [estatisticas.csv]
```

- `novos.csv`: arquivo com os registros a inserir (primeira linha = cabeçalho);
- `destino.csv`: arquivo de destino; se não existir, é criado com o cabeçalho do arquivo de novos registros;
- `modo` (opcional): `seq-iter` (padrão), `seq-rec`, `bin-iter` ou `bin-rec`;
- `estatisticas.csv` (opcional): acrescenta uma linha com contagens de comparações e tempos.

Exemplos:

```bash
./src/copiador dados/bolsistas-2025.csv cadastro.csv
./src/copiador dados/bolsistas-2025.csv cadastro.csv bin-rec estatisticas.csv
```

Registros cuja chave já existe no destino (ou que se repetem no próprio arquivo de novos registros) não são inseridos, e o programa imprime `ERRO: valor duplicado na chave, registro ignorado: <nome>`.

## Reproduzir os experimentos

Requer `bash`, `make`, GCC e Python 3 com `pandas`, `numpy` e `matplotlib`.

```bash
./rodar_experimentos.sh
python3 analisar.py
```
