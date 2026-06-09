import subprocess, csv, os, sys, re
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

BIN = "build/ga_detector.exe"
IMGS = [
    "imgs/C01.bmp", "imgs/C02.bmp", "imgs/C03.bmp",
    "imgs/C04.bmp", "imgs/C05.bmp", "imgs/auto.bmp",
    "imgs/lamp.bmp", "imgs/1uno.bmp", "imgs/2dos.bmp"
]
POB, PC, PM, SEL, CRUZA = 100, 0.90, 0.05, 2, 1
ITERS = 100

def ejecutar_imagen(img_path, gens):
    nombre = os.path.basename(img_path)
    cmd = [BIN, img_path, str(POB), str(gens), str(PC), str(PM), str(SEL), str(CRUZA), str(ITERS)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    lines = proc.stdout.strip().split('\n')
    data = []
    stats = None
    for line in lines:
        if line.startswith("STATS"):
            parts = line.split(',')
            stats = list(map(float, parts[1:]))
        else:
            try:
                cx, cy, r, fit = map(float, line.split(','))
                data.append([nombre, cx, cy, r, fit])
            except ValueError:
                continue
    return nombre, data, stats

# FASE 1: Correr 100 iteraciones por imagen (gens=500) para resultados y graficas 1 y 2
print("=== FASE 1: 100 iteraciones por imagen ===")
todas = []
stats_dict = {}
for i, img in enumerate(IMGS):
    print(f"  [{i+1}/{len(IMGS)}] {os.path.basename(img)}...", end=" ", flush=True)
    nombre, data, stats = ejecutar_imagen(img, 500)
    todas.extend(data)
    if stats:
        stats_dict[nombre] = stats
    print(f"ok (fit_avg={stats[6]:.0f})" if stats else "fallo")

os.makedirs("resultados", exist_ok=True)
csv_path = "resultados/resultados_crudos.csv"
with open(csv_path, "w", newline='') as f:
    w = csv.writer(f)
    w.writerow(["Imagen", "Centro_X", "Centro_Y", "Radio", "Fitness"])
    w.writerows(todas)
print(f"CSV guardado: {csv_path}")

# === GRAFICA 1: Comparativa de Fitness ===
print("\nGenerando grafica 1...")
fig, ax = plt.subplots(figsize=(10, 5))
nombres = list(stats_dict.keys())
fit_avgs = [stats_dict[n][6] for n in nombres]
fit_stds = [stats_dict[n][7] for n in nombres]
colores = plt.cm.Set2(np.linspace(0, 1, len(nombres)))
bars = ax.bar(nombres, fit_avgs, yerr=fit_stds, color=colores, capsize=5, edgecolor='gray')
ax.set_xlabel("Imagen")
ax.set_ylabel("Fitness promedio")
ax.set_title("Comparativa de Fitness promedio por imagen (100 ejecuciones)")
ax.grid(axis='y', alpha=0.3)
for bar, avg, std in zip(bars, fit_avgs, fit_stds):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + std + 2,
            f'{avg:.0f}', ha='center', va='bottom', fontsize=9)
plt.tight_layout()
out1 = "outputs/comparativa_fitness.png"
plt.savefig(out1, dpi=150)
plt.close()
print(f"  {out1}")

# === GRAFICA 2: Dispersion de circulos ===
print("Generando grafica 2...")
fig, axes = plt.subplots(3, 3, figsize=(12, 10))
axes = axes.flatten()
for idx, nombre in enumerate(nombres):
    ax = axes[idx]
    runs = [r for r in todas if r[0] == nombre]
    xs = [r[1] for r in runs]
    ys = [r[2] for r in runs]
    ax.scatter(xs, ys, s=8, alpha=0.5, c='steelblue')
    ax.set_title(nombre.replace('.bmp',''), fontsize=9)
    ax.set_xlabel("X"); ax.set_ylabel("Y")
    ax.grid(alpha=0.2)
    ax.set_aspect('equal', adjustable='box')
for idx in range(len(nombres), 9):
    axes[idx].set_visible(False)
fig.suptitle("Dispersion de centros (x,y) por imagen (100 ejecuciones)", fontsize=13)
plt.tight_layout()
out2 = "outputs/dispersion_circulos.png"
plt.savefig(out2, dpi=150)
plt.close()
print(f"  {out2}")

# === GRAFICA 3: Curva de convergencia REAL ===
def ejecutar_convergencia(img_path, gens, iters_internos=20):
    nombre = os.path.basename(img_path)
    cmd = [BIN, img_path, str(POB), str(gens), str(PC), str(PM), str(SEL), str(CRUZA), str(iters_internos)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    for line in proc.stdout.strip().split('\n'):
        if line.startswith("STATS"):
            return float(line.split(',')[7])
    return None

print("Generando grafica 3 (convergencia real)...")
GEN_SAMPLES = [10, 25, 50, 75, 100, 150, 200, 300, 400, 500]
convergencia = {os.path.basename(img).replace('.bmp',''): [] for img in IMGS}

for gens in GEN_SAMPLES:
    print(f"  gens={gens}...", end=" ", flush=True)
    for img in IMGS:
        nom = os.path.basename(img).replace('.bmp','')
        fit = ejecutar_convergencia(img, gens)
        if fit is not None:
            convergencia[nom].append((gens, fit))
    print("ok")

fig, ax = plt.subplots(figsize=(10, 5))
for nom_corto, puntos in convergencia.items():
    if not puntos:
        continue
    gs = [p[0] for p in puntos]
    fits = [p[1] for p in puntos]
    ax.plot(gs, fits, marker='o', markersize=3, lw=1.5, label=nom_corto)
ax.set_xlabel("Generaciones")
ax.set_ylabel("Fitness promedio")
ax.set_title("Curva de convergencia real del fitness (promedio de 100 ejecuciones por punto)")
ax.legend(fontsize=8, ncol=3)
ax.grid(alpha=0.3)
plt.tight_layout()
out3 = "outputs/convergencia_simulada.png"
plt.savefig(out3, dpi=150)
plt.close()
print(f"  {out3}")

print("\n¡Listo! Las 3 graficas estan en outputs/")
