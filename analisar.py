import math
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import FuncFormatter

BASE = os.path.dirname(os.path.abspath(__file__))
EST = os.path.join(BASE, "experimentos", "estatisticas")
SAIDA = os.path.join(BASE, "graficos")
os.makedirs(SAIDA, exist_ok=True)

COR = {"seq-iter": "#2a78d6", "seq-rec": "#8e44ad", "bin-iter": "#1b9e77", "bin-rec": "#d95f02"}
MARCA = {"seq-iter": "o", "seq-rec": "s", "bin-iter": "^", "bin-rec": "D"}
ROTULO = {"seq-iter": "Seq. iterativa", "seq-rec": "Seq. recursiva",
          "bin-iter": "MergeSort + binária (iterativas)", "bin-rec": "MergeSort + binária (recursivas)"}
TEO = "#555555"

plt.rcParams.update({
    "figure.facecolor": "#fcfcfb", "axes.facecolor": "#fcfcfb", "font.size": 10,
    "axes.titlesize": 12, "axes.titleweight": "bold", "axes.edgecolor": "#c3c2b7",
    "grid.color": "#e1e0d9", "legend.frameon": False, "figure.dpi": 160,
    "axes.spines.top": False, "axes.spines.right": False,
})


def milhar(x, _):
    return f"{int(round(x)):,}".replace(",", ".")


def salvar(fig, nome):
    fig.tight_layout()
    fig.savefig(os.path.join(SAIDA, nome), bbox_inches="tight")
    plt.close(fig)


def lg(n):
    return math.log2(n)


def t_seq(n, q):
    return n * q + n * (n - 1) // 2


def ms_pior_topdown(n):
    if n < 2:
        return 0
    k = math.ceil(lg(n))
    return n * k - 2 ** k + 1


def ms_melhor_topdown(n):
    if n < 2:
        return 0
    return ms_melhor_topdown(n // 2) + ms_melhor_topdown(n - n // 2) + n // 2


def ms_pior_bottomup(n):
    total, largura = 0, 1
    while largura < n:
        ini = 0
        while ini + largura < n:
            fim = min(ini + 2 * largura, n)
            total += fim - ini - 1
            ini += 2 * largura
        largura *= 2
    return total


def ms_melhor_bottomup(n):
    total, largura = 0, 1
    while largura < n:
        ini = 0
        while ini + largura < n:
            fim = min(ini + 2 * largura, n)
            total += min(largura, fim - ini - largura)
            ini += 2 * largura
        largura *= 2
    return total


def bb_min(n):
    return 0 if n == 0 else int(math.floor(lg(n + 1)))


def bb_max(n):
    return 0 if n == 0 else int(math.floor(lg(n))) + 1


a = pd.read_csv(os.path.join(EST, "A.csv"))
c = pd.read_csv(os.path.join(EST, "C.csv"))
d = pd.read_csv(os.path.join(EST, "D.csv"))
e = pd.read_csv(os.path.join(EST, "E.csv"))
tr = pd.read_csv(os.path.join(EST, "tempos_reais.csv"))
bench = pd.read_csv(os.path.join(EST, "bench.csv"))

print("=== Solucao 1: previsto x medido ===")
for nome, df in [("A", a), ("C", c)]:
    s = df[df.modo.str.startswith("seq")].copy()
    s["previsto"] = [t_seq(n, q) for n, q in zip(s.inseridos, s.previos)]
    s["dif"] = s.comp_total - s.previsto
    print(nome, "diferenca maxima:", s.dif.abs().max())
    print(s[["modo", "inseridos", "previos", "comp_total", "previsto", "dif"]].to_string(index=False))

print("\n=== Solucao 2: limites x medido ===")
for nome, df in [("A", a), ("C", c)]:
    b = df[df.modo.str.startswith("bin")].copy()
    linhas = []
    for _, r in b.iterrows():
        n, q = int(r.lidos), int(r.previos)
        if r.modo == "bin-rec":
            ol, oh = ms_melhor_topdown(q) + ms_melhor_topdown(n), ms_pior_topdown(q) + ms_pior_topdown(n)
        else:
            ol, oh = ms_melhor_bottomup(q) + ms_melhor_bottomup(n), ms_pior_bottomup(q) + ms_pior_bottomup(n)
        bl = n * (bb_min(q) + bb_min(n))
        bh = n * (bb_max(q) + (1 if q else 0) + bb_max(n))
        ok = ol <= r.comp_ordenacao <= oh and bl <= r.comp_busca <= bh
        linhas.append((r.modo, n, q, ol, int(r.comp_ordenacao), oh, bl, int(r.comp_busca), bh, ok))
    for l in linhas:
        print(nome, *l)

print("\nMergeSort n=718 (D, ordem original) iter/rec:", d[d.modo.str.startswith("bin")][["modo", "comp_ordenacao"]].values.tolist())
print("limites 718 top-down:", ms_melhor_topdown(718), ms_pior_topdown(718), " bottom-up:", ms_melhor_bottomup(718), ms_pior_bottomup(718))
print("D:\n", d.to_string(index=False))
print("E:\n", e.to_string(index=False))

med = tr.groupby(["modo", "lidos"]).tempo_total_s.median().unstack(0) * 1e3
print("\n=== tempos reais (mediana, ms) ===")
print(med.to_string(float_format=lambda x: f"{x:.4f}"))

print("\n=== benchmark ===")
print(bench.to_string(index=False))
for modo in ["seq-iter", "seq-rec"]:
    s = bench[bench.modo == modo]
    incl = np.polyfit(np.log(s.n), np.log(s.tempo_total_s), 1)[0]
    raz = (s.tempo_total_s.values[1:] / s.tempo_total_s.values[:-1]).round(3)
    print(modo, "inclinacao log-log do tempo:", round(incl, 4), "razoes:", raz)
for modo in ["bin-iter", "bin-rec"]:
    s = bench[bench.modo == modo]
    incl = np.polyfit(np.log(s.n), np.log(s.tempo_total_s), 1)[0]
    raz = (s.tempo_total_s.values[1:] / s.tempo_total_s.values[:-1]).round(3)
    k = (s.tempo_total_s / (s.n * np.log2(s.n)) * 1e9).round(3)
    print(modo, "inclinacao log-log do tempo:", round(incl, 4), "razoes:", raz, "ns por n lg n:", k.values)
    kc = (s.comp_ordenacao + s.comp_busca) / (s.n * np.log2(s.n))
    print(modo, "comparacoes / (n lg n):", kc.round(4).values)
for n in [1000, 2000, 4000, 8000]:
    si = bench[(bench.modo == "seq-iter") & (bench.n == n)].tempo_total_s.iloc[0]
    sr = bench[(bench.modo == "seq-rec") & (bench.n == n)].tempo_total_s.iloc[0]
    bi = bench[(bench.modo == "bin-iter") & (bench.n == n)].tempo_total_s.iloc[0]
    br = bench[(bench.modo == "bin-rec") & (bench.n == n)].tempo_total_s.iloc[0]
    print(f"n={n}: rec/iter seq={sr/si:.2f} bin={br/bi:.2f}  seq-iter/bin-iter={si/bi:.1f}")

aq = a.copy()
fig, ax = plt.subplots(figsize=(7.2, 4.2))
g = np.linspace(1, 718, 400)
ax.plot(g, g * (g - 1) / 2, color=TEO, lw=1.8, label="Previsto: N(N−1)/2")
for m in ["seq-iter", "seq-rec"]:
    s = aq[aq.modo == m]
    ax.plot(s.inseridos, s.comp_total, MARCA[m], color=COR[m], ms=7 if m == "seq-iter" else 4, label=ROTULO[m])
ax.set_title("Solução 1: comparações medidas × previstas (Q = 0)", loc="left")
ax.set_xlabel("N (registros inseridos)")
ax.set_ylabel("Comparações entre chaves")
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.grid(True, axis="y")
ax.legend()
salvar(fig, "g1_sol1_observado_previsto.png")

fig, ax = plt.subplots(figsize=(7.2, 4.2))
gq = np.linspace(0, 160, 100)
ax.plot(gq, 160 * gq + 12720, color=TEO, lw=1.8, label="Previsto: 160·Q + 12.720")
for m in ["seq-iter", "seq-rec"]:
    s = c[c.modo == m]
    ax.plot(s.previos, s.comp_total, MARCA[m], color=COR[m], ms=7 if m == "seq-iter" else 4, label=ROTULO[m])
ax.set_title("Solução 1: efeito de Q com N = 160 fixo", loc="left")
ax.set_xlabel("Q (registros já existentes no destino)")
ax.set_ylabel("Comparações entre chaves")
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.grid(True, axis="y")
ax.legend()
salvar(fig, "g2_sol1_termo_nq.png")

fig, ax = plt.subplots(figsize=(7.2, 4.2))
ns = np.array(sorted(a.lidos.unique()))
low = [ms_melhor_topdown(n) + n * bb_min(n) for n in ns]
high = [ms_pior_topdown(n) + n * bb_max(n) for n in ns]
ax.fill_between(ns, low, high, color="#dddddd", label="Faixa teórica (MergeSort + busca binária)")
ax.plot(ns, ns * np.log2(ns) * 2, "--", color=TEO, lw=1.2, label="Referência 2·N lg N")
for m in ["bin-iter", "bin-rec"]:
    s = a[a.modo == m]
    ax.plot(s.lidos, s.comp_total, MARCA[m], color=COR[m], ms=6, label=ROTULO[m])
ax.set_title("Solução 2: comparações medidas (Q = 0)", loc="left")
ax.set_xlabel("N (registros novos)")
ax.set_ylabel("Comparações entre chaves")
ax.yaxis.set_major_formatter(FuncFormatter(milhar))
ax.grid(True, axis="y")
ax.legend(fontsize=8)
salvar(fig, "g3_sol2_comparacoes.png")

fig, ax = plt.subplots(figsize=(7.2, 4.2))
for m in ["seq-iter", "bin-iter", "bin-rec"]:
    s = a[a.modo == m]
    ax.plot(s.lidos, s.comp_total, MARCA[m] + "-", color=COR[m], ms=6, lw=1.5, label=ROTULO[m])
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_title("Comparações: Solução 1 × Solução 2 (Q = 0)", loc="left")
ax.set_xlabel("N (escala log)")
ax.set_ylabel("Comparações (escala log)")
ax.grid(True, which="major")
ax.legend(fontsize=8)
salvar(fig, "g4_sol1_x_sol2_comparacoes.png")

fig, ax = plt.subplots(figsize=(7.2, 4.4))
for m in ["seq-iter", "seq-rec", "bin-iter", "bin-rec"]:
    s = bench[bench.modo == m]
    ax.plot(s.n, s.tempo_total_s * 1e3, MARCA[m] + "-", color=COR[m], ms=5, lw=1.5, label=ROTULO[m])
s = bench[bench.modo == "seq-iter"]
ref = s.tempo_total_s.iloc[-1] * 1e3 * (s.n / s.n.iloc[-1]) ** 2
ax.plot(s.n, ref, ":", color=TEO, lw=1.5, label="Referência ∝ N²")
s = bench[bench.modo == "bin-iter"]
nn = s.n.values.astype(float)
ref = s.tempo_total_s.iloc[-1] * 1e3 * (nn * np.log2(nn)) / (nn[-1] * np.log2(nn[-1]))
ax.plot(nn, ref, "--", color=TEO, lw=1.2, label="Referência ∝ N lg N")
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_title("Tempo de execução medido (benchmark em memória)", loc="left")
ax.set_xlabel("N (escala log)")
ax.set_ylabel("Tempo em ms (escala log, mediana de 15)")
ax.grid(True, which="major")
ax.legend(fontsize=8)
salvar(fig, "g5_tempos_benchmark.png")

fig, ax = plt.subplots(figsize=(7.2, 4.2))
for m in ["seq-iter", "seq-rec", "bin-iter", "bin-rec"]:
    ax.plot(med.index, med[m], MARCA[m] + "-", color=COR[m], ms=5, lw=1.5, label=ROTULO[m])
ax.set_title("Tempo do programa com os dados reais (Q = 0)", loc="left")
ax.set_xlabel("N (registros novos)")
ax.set_ylabel("Tempo em ms (mediana de 15)")
ax.grid(True, axis="y")
ax.legend(fontsize=8)
salvar(fig, "g6_tempos_dados_reais.png")

print("\ngraficos gerados em", SAIDA)
