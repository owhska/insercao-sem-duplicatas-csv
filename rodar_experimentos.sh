#!/bin/bash
set -e

BASE="$(cd "$(dirname "$0")" && pwd)"
cd "$BASE"

BIN="$BASE/src/copiador"
BENCH="$BASE/src/bench"
[ -f "${BIN}.exe" ] && BIN="${BIN}.exe"
[ -f "${BENCH}.exe" ] && BENCH="${BENCH}.exe"

ORIGEM="$BASE/dados/bolsistas-2025.csv"
EXP="$BASE/experimentos"
ENT="$EXP/entradas"
DST="$EXP/destinos"
EST="$EXP/estatisticas"
SAI="$EXP/saidas"
MODOS="seq-iter seq-rec bin-iter bin-rec"
REPS="${REPS:-15}"

if python3 --version >/dev/null 2>&1; then PY=python3
elif python --version >/dev/null 2>&1; then PY=python
elif py --version >/dev/null 2>&1; then PY=py
else echo "ERRO: Python nao encontrado." >&2; exit 1; fi

echo ">> Compilando"
make -C "$BASE" --no-print-directory

rm -rf "$EXP"
mkdir -p "$ENT" "$DST" "$EST" "$SAI"

echo ">> Gerando entradas"
"$PY" "$BASE/preparar_entradas.py"

NS="10 20 40 80 160 320 640 718"

rodar() {
    local rotulo="$1" novos="$2" previo="$3" est="$4"
    for M in $MODOS; do
        local D="$DST/${rotulo}_${M}.csv"
        rm -f "$D"
        [ -n "$previo" ] && cp "$previo" "$D"
        "$BIN" "$novos" "$D" "$M" "$est" > "$SAI/${rotulo}_${M}.txt"
    done
}

echo ">> Experimento A (Q = 0, N variavel)"
for N in $NS; do
    rodar "A_n${N}" "$ENT/novos_${N}.csv" "" "$EST/A.csv"
done

echo ">> Experimento C (N = 160, Q variavel)"
for Q in 0 10 20 40 80 160; do
    P=""; [ "$Q" -gt 0 ] && P="$ENT/previos_${Q}.csv"
    rodar "C_q${Q}" "$ENT/novos_160.csv" "$P" "$EST/C.csv"
done

echo ">> Experimento D (arquivo original completo, Q = 0)"
rodar "D_completo" "$ORIGEM" "" "$EST/D.csv"

echo ">> Experimento E (reexecucao sobre o destino do Experimento D)"
for M in $MODOS; do
    cp "$DST/D_completo_${M}.csv" "$DST/E_reexecucao_${M}.csv"
    "$BIN" "$ORIGEM" "$DST/E_reexecucao_${M}.csv" "$M" "$EST/E.csv" > "$SAI/E_reexecucao_${M}.txt"
done

echo ">> Tempos com dados reais ($REPS repeticoes por ponto)"
for N in $NS; do
    for M in $MODOS; do
        for R in $(seq "$REPS"); do
            rm -f "$DST/tmp.csv"
            "$BIN" "$ENT/novos_${N}.csv" "$DST/tmp.csv" "$M" "$EST/tempos_reais.csv" > /dev/null
        done
    done
done
rm -f "$DST/tmp.csv"

echo ">> Benchmark em memoria (mediana de $REPS repeticoes)"
"$BENCH" "$ORIGEM" "$REPS" > "$EST/bench.csv"

echo ">> Concluido. Rode: $PY analisar.py"
