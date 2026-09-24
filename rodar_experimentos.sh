#!/bin/bash
set -e

# ------------------------------------------------------------------
# Caminhos base
# ------------------------------------------------------------------
BASE="$(cd "$(dirname "$0")" && pwd)"
cd "$BASE"

BIN="$BASE/src/copiador"
DADOS="$BASE/dados"
EXP="$BASE/experimentos"
ORIGEM="$DADOS/bolsistas-2025.csv"

# No Windows o executável sai como .exe
if [ -f "${BIN}.exe" ]; then
    BIN="${BIN}.exe"
fi

# ------------------------------------------------------------------
# Conversao de caminho Unix -> Windows quando cygpath existe
# (Git Bash usa /c/Users/..., Python do Windows precisa C:\Users\...)
# ------------------------------------------------------------------
converter() {
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -w "$1"
    else
        echo "$1"
    fi
}

ORIGEM_PY=$(converter "$ORIGEM")
ENTRADAS_PY=$(converter "$EXP/entradas")

# ------------------------------------------------------------------
# Cria pastas e limpa execucoes anteriores
# ------------------------------------------------------------------
mkdir -p "$EXP/entradas" "$EXP/destinos" "$EXP/estatisticas" "$EXP/saidas"
rm -f "$EXP"/entradas/*.csv "$EXP"/destinos/*.csv "$EXP"/estatisticas/*.csv "$EXP"/saidas/*.txt

# ------------------------------------------------------------------
# Compila
# ------------------------------------------------------------------
echo ">> Compilando"
make -C "$BASE" --no-print-directory

# ------------------------------------------------------------------
# Detecta Python (testa de fato com --version para ignorar o alias
# fantasma da Microsoft Store)
# ------------------------------------------------------------------
if python --version >/dev/null 2>&1; then
    PY=python
elif python3 --version >/dev/null 2>&1; then
    PY=python3
elif py --version >/dev/null 2>&1; then
    PY=py
else
    echo "ERRO: Python nao encontrado no PATH." >&2
    echo "Instale em https://www.python.org/downloads/ marcando 'Add Python to PATH'." >&2
    exit 1
fi

echo ">> Python detectado: $PY ($($PY --version 2>&1))"

CABECALHO="Nome,Modalidade,Nivel,Agencia"
TAMANHOS="10 20 40 80 160 320 640"

# ------------------------------------------------------------------
# Gera os recortes de entrada com nomes distintos
# ------------------------------------------------------------------
echo ">> Gerando os recortes de entrada com nomes distintos"
"$PY" - "$ORIGEM_PY" "$ENTRADAS_PY" $TAMANHOS <<'PY'
import sys, os

origem, saida = sys.argv[1], sys.argv[2]
tamanhos = [int(x) for x in sys.argv[3:]]

linhas = open(origem, encoding="latin-1").read().splitlines()
linhas = [l for l in linhas if l.strip()]
dados = linhas[1:]

vistos, unicas = set(), []
for linha in dados:
    nome = linha.split(",")[0]
    if nome not in vistos:
        vistos.add(nome)
        unicas.append(linha)

for n in tamanhos + [len(unicas)]:
    if n > len(unicas):
        continue
    caminho = os.path.join(saida, f"origem_{n}.csv")
    with open(caminho, "w", encoding="latin-1") as f:
        f.write("Nome,Modalidade,Nivel,Agencia\n")
        for linha in unicas[:n]:
            f.write(linha + "\n")

for q in [10, 20, 40, 80, 160]:
    caminho = os.path.join(saida, f"previos_{q}.csv")
    with open(caminho, "w", encoding="latin-1") as f:
        f.write("Nome,Modalidade,Nivel,Agencia\n")
        for i in range(q):
            f.write(f"CADASTRO PREVIO {i:04d},PQ,C,CNPq\n")

print(f"nomes distintos no arquivo original: {len(unicas)}")
PY

# ------------------------------------------------------------------
# Calcula total de nomes distintos com Python
# ------------------------------------------------------------------
TOTAL_UNICOS=$("$PY" -c "
linhas=[l for l in open(r'$ORIGEM_PY',encoding='latin-1').read().splitlines() if l.strip()][1:]
print(len({l.split(',')[0] for l in linhas}))")

echo ">> Total de nomes distintos: $TOTAL_UNICOS"

# ------------------------------------------------------------------
# Funcao auxiliar: executa um experimento em ambos os modos
#   $1 = rotulo curto (ex: A_q0_n10)
#   $2 = arquivo de origem
#   $3 = arquivo de destino base (sem extensao de modo)
#   $4 = arquivo de estatisticas base (sem extensao de modo)
#   $5 = (opcional) arquivo de previos, copiado para o destino antes
# ------------------------------------------------------------------
rodar_ambos_modos() {
    local rotulo="$1"
    local origem="$2"
    local dest_base="$3"
    local est_base="$4"
    local previo="${5:-}"

    for MODO in iter rec; do
        local DEST="${dest_base}_${MODO}.csv"
        local EST="${est_base}_${MODO}.csv"
        local SAI="$EXP/saidas/${rotulo}_${MODO}.txt"

        rm -f "$DEST"
        if [ -n "$previo" ]; then
            cp "$previo" "$DEST"
        fi

        "$BIN" "$origem" "$DEST" "$EST" "$MODO" > "$SAI"
    done
}

# ------------------------------------------------------------------
# Experimento A - destino vazio (Q = 0)
# ------------------------------------------------------------------
echo ">> Experimento A - destino vazio (Q = 0)"
for N in $TAMANHOS $TOTAL_UNICOS; do
    rodar_ambos_modos "A_q0_n${N}" \
        "$EXP/entradas/origem_${N}.csv" \
        "$EXP/destinos/A_q0_n${N}" \
        "$EXP/estatisticas/A_destino_vazio"
done

# ------------------------------------------------------------------
# Experimento B - destino com 1 cadastro previo (Q = 1)
# ------------------------------------------------------------------
echo ">> Experimento B - destino com 1 cadastro previo (Q = 1)"
for N in $TAMANHOS $TOTAL_UNICOS; do
    PREVIO="$EXP/entradas/previo_q1.csv"
    printf '%s\nCADASTRO PREVIO 0000,PQ,C,CNPq\n' "$CABECALHO" > "$PREVIO"
    rodar_ambos_modos "B_q1_n${N}" \
        "$EXP/entradas/origem_${N}.csv" \
        "$EXP/destinos/B_q1_n${N}" \
        "$EXP/estatisticas/B_q1" \
        "$PREVIO"
done

# ------------------------------------------------------------------
# Experimento C - N = 160 fixo, Q variavel
# ------------------------------------------------------------------
echo ">> Experimento C - N = 160 fixo, Q variavel"
for Q in 0 10 20 40 80 160; do
    if [ "$Q" -eq 0 ]; then
        PREVIO=""
    else
        PREVIO="$EXP/entradas/previos_${Q}.csv"
    fi
    rodar_ambos_modos "C_n160_q${Q}" \
        "$EXP/entradas/origem_160.csv" \
        "$EXP/destinos/C_n160_q${Q}" \
        "$EXP/estatisticas/C_q_variavel" \
        "$PREVIO"
done

# ------------------------------------------------------------------
# Experimento D - arquivo original completo (com redundancias reais)
# ------------------------------------------------------------------
echo ">> Experimento D - arquivo original completo"
for MODO in iter rec; do
    DEST="$EXP/destinos/D_arquivo_completo_${MODO}.csv"
    EST="$EXP/estatisticas/D_arquivo_completo_${MODO}.csv"
    rm -f "$DEST"
    "$BIN" "$ORIGEM" "$DEST" "$EST" "$MODO" \
        > "$EXP/saidas/D_arquivo_completo_${MODO}.txt"
done

# ------------------------------------------------------------------
# Experimento E - reexecucao sobre destino ja preenchido
# ------------------------------------------------------------------
echo ">> Experimento E - reexecucao sobre destino ja preenchido"
for MODO in iter rec; do
    DEST="$EXP/destinos/E_reexecucao_${MODO}.csv"
    EST="$EXP/estatisticas/E_reexecucao_${MODO}.csv"
    cp "$EXP/destinos/D_arquivo_completo_${MODO}.csv" "$DEST"
    "$BIN" "$ORIGEM" "$DEST" "$EST" "$MODO" \
        > "$EXP/saidas/E_reexecucao_${MODO}.txt"
done

# ------------------------------------------------------------------
# Resumo final
# ------------------------------------------------------------------
echo ""
echo ">> Concluido"
echo ">> Estatisticas geradas:"
for f in "$EXP"/estatisticas/*.csv; do
    echo "--- $(basename "$f")"
    cat "$f"
done
