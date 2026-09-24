import os
import random
import sys

base = os.path.dirname(os.path.abspath(__file__))
origem = os.path.join(base, "dados", "bolsistas-2025.csv")
saida = os.path.join(base, "experimentos", "entradas")
os.makedirs(saida, exist_ok=True)

linhas = [l for l in open(origem, encoding="latin-1").read().splitlines() if l.strip()]
cabecalho, dados = linhas[0], linhas[1:]

vistos, unicas = set(), []
for linha in dados:
    nome = linha.split(",")[0]
    if nome not in vistos:
        vistos.add(nome)
        unicas.append(linha)

random.Random(2026).shuffle(unicas)

def gravar(nome, registros):
    with open(os.path.join(saida, nome), "w", encoding="latin-1", newline="\n") as f:
        f.write(cabecalho + "\n")
        for r in registros:
            f.write(r + "\n")

for n in [10, 20, 40, 80, 160, 320, 640, len(unicas)]:
    gravar(f"novos_{n}.csv", unicas[:n])

for q in [10, 20, 40, 80, 160]:
    gravar(f"previos_{q}.csv", unicas[160:160 + q])

print(f"registros: {len(dados)}  nomes distintos: {len(unicas)}")
