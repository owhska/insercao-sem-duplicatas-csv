#!/usr/bin/env python3
"""Compara as comparacoes medidas pelo copiador com o modelo teorico
T(N, Q) = N*Q + N*(N-1)/2 e gera os graficos do relatorio.

Agora le dois arquivos por experimento:
    <nome>_iter.csv  -> modo iterativo
    <nome>_rec.csv   -> modo recursivo
e concatena em um unico DataFrame com a coluna 'modo'.
"""

import os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

BASE = os.path.dirname(os.path.abspath(__file__))
EST = os.path.join(BASE, "experimentos", "estatisticas")
SAIDA = os.path.join(BASE, "graficos")
os.makedirs(SAIDA, exist_ok=True)

OBS = "#2a78d6"   # azul - iterativa
REC = "#8e44ad"   # roxo - recursiva
TEO = "#eb6834"   # laranja - teoria
SURFACE = "#fcfcfb"
INK = "#0b0b0b"
MUTED = "#898781"
GRID = "#e1e0d9"
AXIS = "#c3c2b7"

plt.rcParams.update({
    "figure.facecolor": SURFACE,
    "axes.facecolor": SURFACE,
    "font.family": "DejaVu Sans",
    "font.size": 10,
    "axes.titlesize": 12,
    "axes.titleweight": "bold",
    "axes.labelcolor": INK,
    "axes.edgecolor": AXIS,
    "text.color": INK,
    "xtick.color": MUTED,
    "ytick.color": MUTED,
    "grid.color": GRID,
    "grid.linewidth": 0.8,
    "legend.frameon": False,
    "figure.dpi": 160,
})


def modelo(n, q):
    return n * q + n * (n - 1) // 2


def moldura(ax, titulo, xlabel, ylabel, subtitulo=None):
    ax.set_title(titulo, loc="left", pad=18 if subtitulo else 10)
    if subtitulo:
        ax.text(0, 1.02, subtitulo, transform=ax.transAxes,
                fontsize=9, color="#52514e", va="bottom")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True, axis="y", linewidth=0.8)
    ax.set_axisbelow(True)
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    ax.spines["left"].set_color(AXIS)
    ax.spines["bottom"].set_color(AXIS)


def milhar(x, _):
    return f"{int(x):,}".replace(",", ".")


def salvar(fig, nome):
    caminho = os.path.join(SAIDA, nome)
    fig.tight_layout()
    fig.savefig(caminho, bbox_inches="tight")
    plt.close(fig)
    print("gerado:", caminho)


# ------------------------------------------------------------------
# Carregamento: le os dois modos e concatena
# ------------------------------------------------------------------
def carregar_par(nome_base):
    """Le <nome_base>_iter.csv e <nome_base>_rec.csv e devolve um
    DataFrame concatenado com coluna 'modo' em {'iter', 'rec'}."""
    it = pd.read_csv(os.path.join(EST, f"{nome_base}_iter.csv"))
    rc = pd.read_csv(os.path.join(EST, f"{nome_base}_rec.csv"))
    it["modo"] = "iter"
    rc["modo"] = "rec"
    return pd.concat([it, rc], ignore_index=True)


a = carregar_par("A_destino_vazio")
b = carregar_par("B_q1")
c = carregar_par("C_q_variavel")

# Separa por modo para os graficos
a_iter = a[a.modo == "iter"].sort_values("nomes_inseridos").reset_index(drop=True)
a_rec  = a[a.modo == "rec"].sort_values("nomes_inseridos").reset_index(drop=True)
b_iter = b[b.modo == "iter"].sort_values("nomes_inseridos").reset_index(drop=True)
b_rec  = b[b.modo == "rec"].sort_values("nomes_inseridos").reset_index(drop=True)
c_iter = c[c.modo == "iter"].reset_index(drop=True)
c_rec  = c[c.modo == "rec"].reset_index(drop=True)

# Colunas de previsao e diferenca (por modo)
for df, q in [(a_iter, 0), (a_rec, 0)]:
    df["previsto"]  = [modelo(n, q) for n in df.nomes_inseridos]
    df["diferenca"] = df.comparacoes - df.previsto
for df, q in [(b_iter, 1), (b_rec, 1)]:
    df["previsto"]  = [modelo(n, q) for n in df.nomes_inseridos]
    df["diferenca"] = df.comparacoes - df.previsto

# Experimento C: a coluna Q nao vem do CSV, reconstruimos pela ordem
qs_c = [0, 10, 20, 40, 80, 160]
for df in (c_iter, c_rec):
    df["q"] = qs_c
    df["previsto"]  = [modelo(160, q) for q in df.q]
    df["diferenca"] = df.comparacoes - df.previsto


# ------------------------------------------------------------------
# Grafico 1 - observado x previsto, Q = 0 (iter e rec)
# ------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7.2, 4.2))
grade = np.linspace(1, a_iter.nomes_inseridos.max(), 400)
ax.plot(grade, grade * (grade - 1) / 2, color=TEO, lw=2,
        label="Previsto: N(N−1)/2")
ax.plot(a_iter.nomes_inseridos, a_iter.comparacoes, "o", color=OBS, ms=8,
        markeredgecolor=SURFACE, markeredgewidth=2, label="Observado (iterativa)")
ax.plot(a_rec.nomes_inseridos, a_rec.comparacoes, "s", color=REC, ms=6,
        markeredgecolor=SURFACE, markeredgewidth=1.5, label="Observado (recursiva)")
moldura(ax, "Comparações: observado × previsto (Q = 0)", "N (nomes inseridos)",
        "Comparações entre nomes",
        "Os pontos medidos caem exatamente sobre a curva teórica — diferença nula em ambos os modos.")
ax.xaxis.set_major_formatter(FuncFormatter(milhar))
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.legend()
salvar(fig, "g1_observado_previsto.png")


# ------------------------------------------------------------------
# Grafico 2 - log-log, inclinacao 2
# ------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7.2, 4.2))
ax.plot(a_iter.nomes_inseridos, a_iter.comparacoes, "o-", color=OBS, lw=2, ms=7,
        markeredgecolor=SURFACE, markeredgewidth=1.5, label="Observado (iterativa)")
ax.plot(a_rec.nomes_inseridos, a_rec.comparacoes, "s--", color=REC, lw=1.5, ms=5,
        markeredgecolor=SURFACE, markeredgewidth=1.2, label="Observado (recursiva)")
ref = a_iter.comparacoes.iloc[0] * (a_iter.nomes_inseridos / a_iter.nomes_inseridos.iloc[0]) ** 2
ax.plot(a_iter.nomes_inseridos, ref, "--", color=TEO, lw=2, label="Referência N²")
ax.set_xscale("log")
ax.set_yscale("log")
moldura(ax, "Verificação de O(N²) em escala log-log", "N (escala log)",
        "Comparações (escala log)",
        "Em log-log a reta medida tem inclinação 2: duplicar N multiplica as comparações por 4.")
ax.legend()
salvar(fig, "g2_loglog.png")


# ------------------------------------------------------------------
# Grafico 3 - razao T(2N)/T(N) (usando o modo iterativo como referencia,
#             pois a contagem e identica nos dois modos)
# ------------------------------------------------------------------
dobras = [(10, 20), (20, 40), (40, 80), (80, 160), (160, 320), (320, 640)]
rotulos, razoes = [], []
for n1, n2 in dobras:
    t1 = int(a_iter.loc[a_iter.nomes_inseridos == n1, "comparacoes"].iloc[0])
    t2 = int(a_iter.loc[a_iter.nomes_inseridos == n2, "comparacoes"].iloc[0])
    rotulos.append(f"{n1}→{n2}")
    razoes.append(t2 / t1)

fig, ax = plt.subplots(figsize=(7.2, 4.2))
ax.bar(rotulos, razoes, color=OBS, width=0.55)
ax.axhline(4, color=TEO, lw=2, ls="--", label="Limite assintótico = 4")
for i, r in enumerate(razoes):
    ax.text(i, r + 0.06, f"{r:.3f}", ha="center", fontsize=9, color="#52514e")
ax.set_ylim(0, 4.8)
moldura(ax, "Fator de crescimento ao dobrar N", "Transição de N",
        "T(2N) / T(N)",
        "A razão sobe monotonicamente em direção a 4, como prevê um algoritmo quadrático.")
ax.legend(loc="lower right")
salvar(fig, "g3_dobrar_n.png")


# ------------------------------------------------------------------
# Grafico 4 - termo N*Q: N = 160 fixo, Q variavel
# ------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7.2, 4.2))
grade_q = np.linspace(0, c_iter.q.max(), 200)
ax.plot(grade_q, 160 * grade_q + 160 * 159 / 2, color=TEO, lw=2,
        label="Previsto: 160·Q + 12.720")
ax.plot(c_iter.q, c_iter.comparacoes, "o", color=OBS, ms=8,
        markeredgecolor=SURFACE, markeredgewidth=2, label="Observado (iterativa)")
ax.plot(c_rec.q, c_rec.comparacoes, "s", color=REC, ms=6,
        markeredgecolor=SURFACE, markeredgewidth=1.5, label="Observado (recursiva)")
moldura(ax, "Efeito do cadastro prévio Q (N = 160 fixo)", "Q (cadastros já existentes no destino)",
        "Comparações entre nomes",
        "Com N fixo, a dependência em Q é uma reta de inclinação 160 — o termo N·Q da fórmula.")
ax.xaxis.set_major_formatter(FuncFormatter(milhar))
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.legend()
salvar(fig, "g4_termo_nq.png")


# ------------------------------------------------------------------
# Grafico 5 - tempo de execucao: iterativa x recursiva
# ------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7.2, 4.2))
ax.plot(a_iter.nomes_inseridos, a_iter.tempo_s * 1e6, "o-", color=OBS, lw=2, ms=7,
        markeredgecolor=SURFACE, markeredgewidth=1.5, label="Iterativa")
ax.plot(a_rec.nomes_inseridos, a_rec.tempo_s * 1e6, "s--", color=REC, lw=2, ms=6,
        markeredgecolor=SURFACE, markeredgewidth=1.5, label="Recursiva")
moldura(ax, "Tempo de execução: iterativa × recursiva (Q = 0)",
        "N (nomes inseridos)", "Tempo (µs)",
        "A recursiva é consistentemente mais lenta por causa do overhead de chamadas e do uso de pilha.")
ax.xaxis.set_major_formatter(FuncFormatter(milhar))
ax.legend()
salvar(fig, "g5_tempo_iter_rec.png")


# ------------------------------------------------------------------
# Grafico 6 - comparacoes: iterativa x recursiva (devem coincidir)
# ------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7.2, 4.2))
ax.plot(a_iter.nomes_inseridos, a_iter.comparacoes, "o", color=OBS, ms=8,
        markeredgecolor=SURFACE, markeredgewidth=2, label="Iterativa")
ax.plot(a_rec.nomes_inseridos, a_rec.comparacoes, "x", color=REC, ms=8,
        markeredgewidth=2, label="Recursiva")
moldura(ax, "Comparações: iterativa × recursiva (Q = 0)",
        "N (nomes inseridos)", "Comparações entre nomes",
        "As duas implementações produzem o mesmo número de comparações — os pontos se sobrepõem.")
ax.xaxis.set_major_formatter(FuncFormatter(milhar))
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.legend()
salvar(fig, "g6_comparacoes_iter_rec.png")


# ------------------------------------------------------------------
# Tabelas para o relatorio
# ------------------------------------------------------------------
print("\n=== Experimento A (Q = 0) — iterativa ===")
print(a_iter[["nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))
print("\n=== Experimento A (Q = 0) — recursiva ===")
print(a_rec[["nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))

print("\n=== Experimento B (Q = 1) — iterativa ===")
print(b_iter[["nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))
print("\n=== Experimento B (Q = 1) — recursiva ===")
print(b_rec[["nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))

print("\n=== Experimento C (N = 160, Q variavel) — iterativa ===")
print(c_iter[["q", "nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))
print("\n=== Experimento C (N = 160, Q variavel) — recursiva ===")
print(c_rec[["q", "nomes_inseridos", "comparacoes", "previsto", "diferenca", "tempo_s"]].to_string(index=False))

print("\n=== razoes T(2N)/T(N) (iterativa) ===")
for rot, r in zip(rotulos, razoes):
    print(f"{rot}: {r:.4f}")

logn = np.log(a_iter.nomes_inseridos.astype(float))
logt = np.log(a_iter.comparacoes.astype(float))
inclinacao, intercepto = np.polyfit(logn, logt, 1)
print(f"\ninclinacao log-log (iterativa): {inclinacao:.4f}  (esperado 2 para O(N^2))")

logn_r = np.log(a_rec.nomes_inseridos.astype(float))
logt_r = np.log(a_rec.comparacoes.astype(float))
inclinacao_r, _ = np.polyfit(logn_r, logt_r, 1)
print(f"inclinacao log-log (recursiva): {inclinacao_r:.4f}")

print(f"\nerro absoluto maximo A (iter): {a_iter.diferenca.abs().max()}")
print(f"erro absoluto maximo A (rec) : {a_rec.diferenca.abs().max()}")
print(f"erro absoluto maximo B (iter): {b_iter.diferenca.abs().max()}")
print(f"erro absoluto maximo B (rec) : {b_rec.diferenca.abs().max()}")
print(f"erro absoluto maximo C (iter): {c_iter.diferenca.abs().max()}")
print(f"erro absoluto maximo C (rec) : {c_rec.diferenca.abs().max()}")

# Comparacao direta de tempo
print("\n=== Tempo medio (µs) por N — iterativa vs recursiva ===")
for n in sorted(a_iter.nomes_inseridos.unique()):
    ti = a_iter.loc[a_iter.nomes_inseridos == n, "tempo_s"].iloc[0] * 1e6
    tr = a_rec.loc[a_rec.nomes_inseridos == n, "tempo_s"].iloc[0] * 1e6
    razao = tr / ti if ti > 0 else float("inf")
    print(f"N={n:4d}  iter={ti:8.2f} µs  rec={tr:8.2f} µs  razao={razao:.2f}")
