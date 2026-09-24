#!/bin/bash
set -e

BASE="$(cd "$(dirname "$0")" && pwd)"
BIN="$BASE/src/copiador"
DADOS="$BASE/dados"
EXP="$BASE/experimentos"
ORIGEM="$DADOS/bolsistas-2025.csv"

mkdir -p "$EXP/entradas" "$EXP/destinos" "$EXP/estatisticas" "$EXP/saidas"
rm -f "$EXP"/entradas/*.csv "$EXP"/destinos/*.csv "$EXP"/estatisticas/*.csv "$EXP"/saidas/*.txt

echo ">> Compilando"
make -C "$BASE" --no-print-directory

CABECALHO="Nome,Modalidade,Nivel,Agencia"
TAMANHOS="10 20 40 80 160 320 640"

echo ">> Gerando os recortes de entrada com nomes distintos"
python3 - "$ORIGEM" "$EXP/entradas" $TAMANHOS <<'PY'
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

TOTAL_UNICOS=$(python3 -c "
linhas=[l for l in open('$ORIGEM',encoding='latin-1').read().splitlines() if l.strip()][1:]
print(len({l.split(',')[0] for l in linhas}))")

# ------------------------------------------------------------------
# Função auxiliar: executa um experimento em ambos os modos
#   $1 = rótulo curto (ex: A_q0_n10)
#   $2 = arquivo de origem
#   $3 = arquivo de destino base (sem extensão de modo)
#   $4 = arquivo de estatísticas base (sem extensão de modo)
# ------------------------------------------------------------------
rodar_ambos_modos() {
    local rotulo="$1"
    local origem="$2"
    local dest_base="$3"
    local est_base="$4"

    for MODO in iter rec; do
        local DEST="${dest_base}_${MODO}.csv"
        local EST="${est_base}_${MODO}.csv"
        local SAI="$EXP/saidas/${rotulo}_${MODO}.txt"

        # recria o destino do zero
        rm -f "$DEST"
        if [ -n "$5" ]; then
            # se foi passado um arquivo de prévios, copia-o
            cp "$5" "$DEST"
        fi

        "$BIN" "$origem" "$DEST" "$EST" "$MODO" > "$SAI"
    done
}

echo ">> Experimento A - destino vazio (Q = 0)"
for N in $TAMANHOS $TOTAL_UNICOS; do
    rodar_ambos_modos "A_q0_n${N}" \
        "$EXP/entradas/origem_${N}.csv" \
        "$EXP/destinos/A_q0_n${N}" \
        "$EXP/estatisticas/A_destino_vazio"
done

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

echo ">> Experimento D - arquivo original completo, com as redundancias reais"
for MODO in iter rec; do
    DEST="$EXP/destinos/D_arquivo_completo_${MODO}.csv"
    EST="$EXP/estatisticas/D_arquivo_completo_${MODO}.csv"
    rm -f "$DEST"
    "$BIN" "$ORIGEM" "$DEST" "$EST" "$MODO" \
        > "$EXP/saidas/D_arquivo_completo_${MODO}.txt"
done

echo ">> Experimento E - reexecucao sobre um destino ja preenchido (tudo redundante)"
for MODO in iter rec; do
    DEST="$EXP/destinos/E_reexecucao_${MODO}.csv"
    EST="$EXP/estatisticas/E_reexecucao_${MODO}.csv"
    cp "$EXP/destinos/D_arquivo_completo_${MODO}.csv" "$DEST"
    "$BIN" "$ORIGEM" "$DEST" "$EST" "$MODO" \
        > "$EXP/saidas/E_reexecucao_${MODO}.txt"
done

echo ">> Concluido"
for f in "$EXP"/estatisticas/*.csv; do
    echo "--- $(basename "$f")"
    cat "$f"
done
